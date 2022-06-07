#include	<stdio.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<pthread.h>
#include	<unistd.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <sys/socket.h>
#include 	<time.h>


#include "controlador.h"
#include "console.h"
#include "acionamento.h"

#define NSEC_POR_SEC	(1000000000)	// Numero de nanosegundos em um segundo (1 bilhao)
#define USEC_POR_SEC	(1000000)	// Numero de microssegundos em um segundo (1 milhao)
#define NSEC_POR_USEC	(1000)		// Numero de nanosegundos em um microsegundo (1 mil)


controlador statusSistema;


int socket_local, socket_nuvem;
struct sockaddr_in endereco_destino, endereco_nuvem;


pthread_mutex_t em_dados = PTHREAD_MUTEX_INITIALIZER;
pthread_t updateData, controlLoop, console, nuvem;


void recebeDados(void){
	struct timespec t;				// Hora atual
	struct timespec tp;				// Hora de inicio para o periodo de interesse
	int periodo_ns = 30000000;			// 30 ms = 30 000 000 ns

	clock_gettime(CLOCK_MONOTONIC, &t);
	tp.tv_sec = t.tv_sec;
	tp.tv_nsec = t.tv_nsec;

	while (statusSistema.termina == 0)
	{
		clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL);
		pthread_mutex_lock(&em_dados);
		statusSistema.nivelBoiler = recebeSensor(socket_local, endereco_destino, NIVELBOILER);
		statusSistema.tempBoiler = recebeSensor(socket_local, endereco_destino, TEMPBOILER);
		statusSistema.tempColetor = recebeSensor(socket_local, endereco_destino, TEMPCOLETOR);
		statusSistema.tempCanos = recebeSensor(socket_local, endereco_destino, TEMPCANOS);
		pthread_mutex_unlock(&em_dados);

		// Calcula a proxima hora de acordar
		tp.tv_nsec += periodo_ns;

		while (tp.tv_nsec >= NSEC_POR_SEC) {
			tp.tv_nsec -= NSEC_POR_SEC;
			tp.tv_sec++;
		}
	}

}

void recebeNuvem(void){
	char bufferIn[TAM_BUFFER];
	int tam_c;
    int bytes_recebidos;
	while(statusSistema.termina == 0){
		bytes_recebidos = recebe_nuvem(socket_nuvem, bufferIn, TAM_BUFFER, &endereco_nuvem, &tam_c);
		int data = atoi(bufferIn);
		if(data >= 30 && data <= 60){
			envia_mensagem(socket_nuvem, endereco_nuvem, "ok");
			statusSistema.tempMinimaBoiler = (float)data;
		}else{
			envia_mensagem(socket_nuvem, endereco_nuvem, "error");
	}
	}
	
}

void planta_dadosCompletos(double *dados)
{
	pthread_mutex_lock(&em_dados);
	dados[0] = statusSistema.nivelBoiler*2000;
	dados[1] = statusSistema.nivelBoiler;
	dados[2] = statusSistema.tempBoiler;
	dados[3] = statusSistema.tempMinimaBoiler;
	dados[4] = statusSistema.tempColetor;
	dados[5] = statusSistema.tempCanos;
	dados[6] = statusSistema.bombaColetor;
	dados[7] = statusSistema.bombaCirculacao; 
	dados[8] = statusSistema.aquecedor;
	dados[9] = statusSistema.valvulaEntrada; 
	dados[10] = statusSistema.valvulaEsgoto;
	pthread_mutex_unlock(&em_dados); 
}

void malhaControle(void){
	struct timespec t;				// Hora atual
	struct timespec tp;				// Hora de inicio para o periodo de interesse
	int periodo_ns = 30000000;			// 30 ms = 30 000 000 ns

	clock_gettime(CLOCK_MONOTONIC, &t);
	tp.tv_sec = t.tv_sec + 2;
	tp.tv_nsec = t.tv_nsec;

	while (statusSistema.termina == 0)
	{
		clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL);
		if(statusSistema.nivelBoiler < 0.500){
			if(statusSistema.valvulaEntrada == DESLIGA){
				pthread_mutex_lock(&em_dados);
				statusSistema.valvulaEntrada = acionaPeriferico(socket_local, endereco_destino, VALVULAENTRADA, LIGA);
				pthread_mutex_unlock(&em_dados);
			}
		}
		if(statusSistema.nivelBoiler >= 0.505){
			if(statusSistema.valvulaEntrada == LIGA){
				pthread_mutex_lock(&em_dados);
				statusSistema.valvulaEntrada = acionaPeriferico(socket_local, endereco_destino, VALVULAENTRADA, DESLIGA);
				pthread_mutex_unlock(&em_dados);
			}
		}
		if(statusSistema.tempBoiler < statusSistema.tempMinimaBoiler){
			if(statusSistema.tempColetor > statusSistema.tempMinimaBoiler+10){
				if(statusSistema.aquecedor == LIGA){
					pthread_mutex_lock(&em_dados);
					statusSistema.aquecedor = acionaPeriferico(socket_local, endereco_destino, AQUECEDOR, DESLIGA);
					pthread_mutex_unlock(&em_dados);
				}
				if(statusSistema.bombaColetor == DESLIGA){
					pthread_mutex_lock(&em_dados);
					statusSistema.bombaColetor = acionaPeriferico(socket_local, endereco_destino, BOMBACOLETOR, LIGA);
					pthread_mutex_unlock(&em_dados);
				}
			}else{
				if(statusSistema.bombaColetor == LIGA){
					pthread_mutex_lock(&em_dados);
					statusSistema.bombaColetor = acionaPeriferico(socket_local, endereco_destino, BOMBACOLETOR, DESLIGA);
					pthread_mutex_unlock(&em_dados);
				}
				if(statusSistema.aquecedor == DESLIGA){
					pthread_mutex_lock(&em_dados);
					statusSistema.aquecedor = acionaPeriferico(socket_local, endereco_destino, AQUECEDOR, LIGA);
					pthread_mutex_unlock(&em_dados);
				}
			}
		}
		if(statusSistema.tempBoiler > statusSistema.tempMinimaBoiler+5){
			if(statusSistema.bombaColetor == LIGA){
				pthread_mutex_lock(&em_dados);
				statusSistema.bombaColetor = acionaPeriferico(socket_local, endereco_destino, BOMBACOLETOR, DESLIGA);
				pthread_mutex_unlock(&em_dados);
			}
			if(statusSistema.aquecedor == LIGA){
				pthread_mutex_lock(&em_dados);
				statusSistema.aquecedor = acionaPeriferico(socket_local, endereco_destino, AQUECEDOR, DESLIGA);
				pthread_mutex_unlock(&em_dados);
			}
		}
		if(statusSistema.tempCanos < statusSistema.tempBoiler-5){
			if(statusSistema.bombaCirculacao == DESLIGA){
				if(statusSistema.tempBoiler > statusSistema.tempMinimaBoiler){
					pthread_mutex_lock(&em_dados);
					statusSistema.bombaCirculacao = acionaPeriferico(socket_local, endereco_destino, BOMBACIRCULACAO, LIGA);
					pthread_mutex_unlock(&em_dados);
				}
			}
		}
		if(statusSistema.tempCanos >= statusSistema.tempBoiler){
			if(statusSistema.bombaCirculacao == LIGA){
				pthread_mutex_lock(&em_dados);
				statusSistema.bombaCirculacao = acionaPeriferico(socket_local, endereco_destino, BOMBACIRCULACAO, DESLIGA);
				pthread_mutex_unlock(&em_dados);
			}
		}

		// Calcula a proxima hora de acordar
		tp.tv_nsec += periodo_ns;

		while (tp.tv_nsec >= NSEC_POR_SEC) {
			tp.tv_nsec -= NSEC_POR_SEC;
			tp.tv_sec++;
		}
	}
}

int main(int argc, char *argv[]){
    if (argc < 3) {
		fprintf(stderr,"Uso: ./controlador <endereco> <porta> \n");
		fprintf(stderr,"<endereco> eh o DNS ou IP do servidor \n");
		fprintf(stderr,"<porta> eh o numero da porta do servidor \n");
		exit(FALHA);
	}
	//struct param_t socket_param;
	int porta_destino = atoi( argv[2]);
    socket_local = cria_socket_local();
	socket_nuvem = cria_socket_local();
	define_porta_escutada(socket_nuvem,porta_destino+1);
    endereco_destino = cria_endereco_destino(argv[1], porta_destino);

	struct timespec t;				// Hora atual
	struct timespec tp;				// Hora de inicio para o periodo de interesse
	int periodo_ns = 1000000000;			// 1 s = 1 000 000 000 ns

	clock_gettime(CLOCK_MONOTONIC, &t);
	tp.tv_sec = t.tv_sec;
	tp.tv_nsec = t.tv_nsec;

	pthread_mutex_lock(&em_dados);
	statusSistema.tempMinimaBoiler = (float)45.0;
	statusSistema.bombaColetor = acionaPeriferico(socket_local, endereco_destino, BOMBACOLETOR, DESLIGA);
	statusSistema.bombaCirculacao = acionaPeriferico(socket_local, endereco_destino, BOMBACIRCULACAO, DESLIGA);
	statusSistema.aquecedor = acionaPeriferico(socket_local, endereco_destino, AQUECEDOR, DESLIGA);
	statusSistema.valvulaEntrada = acionaPeriferico(socket_local, endereco_destino, VALVULAENTRADA, DESLIGA);
	statusSistema.valvulaEsgoto = acionaPeriferico(socket_local, endereco_destino, VALVULAESGOTO, DESLIGA);
	pthread_mutex_unlock(&em_dados);

	pthread_create(&updateData, 0, (void *) recebeDados, NULL);
	pthread_create(&controlLoop, 0, (void *) malhaControle, NULL);
	pthread_create(&nuvem, 0, (void *) recebeNuvem, NULL);

	if(console_modoJanela() < 0){
		exit(-1);
	}
	pthread_create(&console, 0, (void *) console_threadConsole, NULL);
	int i;
	while (statusSistema.termina == 0)
	{
		clock_nanosleep( CLOCK_MONOTONIC, TIMER_ABSTIME, &tp, NULL);
		console_mostraDados();

		// Calcula a proxima hora de acordar
		tp.tv_nsec += periodo_ns;

		while (tp.tv_nsec >= NSEC_POR_SEC) {
			tp.tv_nsec -= NSEC_POR_SEC;
			tp.tv_sec++;
		}
	}
	console_modoNormal();
	exit(1);
	

}
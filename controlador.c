#include	<stdio.h>
#include	<stdlib.h>
#include	<assert.h>
#include	<pthread.h>
#include	<unistd.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <sys/socket.h>


#include "controlador.h"
#include "console.h"
#include "acionamento.h"

controlador statusSistema;


int main(int argc, char *argv[]){
    if (argc < 3) {
		fprintf(stderr,"Uso: ./controlador <endereco> <porta> \n");
		fprintf(stderr,"<endereco> eh o DNS ou IP do servidor \n");
		fprintf(stderr,"<porta> eh o numero da porta do servidor \n");
		exit(FALHA);
	}

	int porta_destino = atoi( argv[2]);
    int socket_local = cria_socket_local();
    struct sockaddr_in endereco_destino = cria_endereco_destino(argv[1], porta_destino);

    acionaPeriferico(socket_local, endereco_destino, AQUECEDOR, LIGA);
	acionaPeriferico(socket_local, endereco_destino, BOMBACOLETOR, LIGA);
	acionaPeriferico(socket_local, endereco_destino, BOMBACIRCULACAO, LIGA);
	acionaPeriferico(socket_local, endereco_destino, VALVULAENTRADA, LIGA);

	while (1)
	{
		printf("Nivel: %.2f\n",recebeSensor(socket_local, endereco_destino, NIVELBOILER)*2000);
		printf("Temp boiler: %.2f\n",recebeSensor(socket_local, endereco_destino, TEMPBOILER));
		sleep(1);
	}
	

}
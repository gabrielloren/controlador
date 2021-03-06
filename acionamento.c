#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#include "acionamento.h"
#include "controlador.h"
#include "console.h"

pthread_mutex_t socketx = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t socketNuvem = PTHREAD_MUTEX_INITIALIZER;

int cria_socket_local(void)
{
	int socket_local;		/* Socket usado na comunicacao */

	socket_local = socket( PF_INET, SOCK_DGRAM, 0);
	if (socket_local < 0) {
		perror("socket");
		return -1;
	}
	return socket_local;
}

void define_porta_escutada(int socket_local, int porta_escutada) // Porta escutada nuvem
{
	struct sockaddr_in servidor; 	/* Endereco completo do servidor e do cliente */
	int tam_s;						

	memset((char *) &servidor, 0, sizeof(servidor));
	servidor.sin_family = AF_INET;
	servidor.sin_addr.s_addr = htonl(INADDR_ANY);
	servidor.sin_port = htons(porta_escutada);

	tam_s = sizeof(servidor);
	if (bind(socket_local, (struct sockaddr *) &servidor, tam_s) < 0) {
		perror("bind");
		exit(FALHA);
	}
}


struct sockaddr_in cria_endereco_destino(char *destino, int porta_destino)
{
	struct sockaddr_in servidor; 	/* Endereco do servidor incluindo ip e porta */
	struct hostent *dest_internet;	/* Endereco destino em formato proprio */
	struct in_addr dest_ip;		/* Endereco destino em formato ip numerico */

	if (inet_aton ( destino, &dest_ip ))
		dest_internet = gethostbyaddr((char *)&dest_ip, sizeof(dest_ip), AF_INET);
	else
		dest_internet = gethostbyname(destino);

	if (dest_internet == NULL) {
		fprintf(stderr,"Endereco de rede invalido\n");
		exit(FALHA);
	}

	memset((char *) &servidor, 0, sizeof(servidor));
	memcpy(&servidor.sin_addr, dest_internet->h_addr_list[0], sizeof(servidor.sin_addr));
	servidor.sin_family = AF_INET;
	servidor.sin_port = htons(porta_destino);

	return servidor;
}


void envia_mensagem(int socket_local, struct sockaddr_in endereco_destino, char *mensagem)
{
	/* Envia msg ao servidor */
    pthread_mutex_lock(&socketx);
	if (sendto(socket_local, mensagem, strlen(mensagem)+1, 0, (struct sockaddr *) &endereco_destino, sizeof(endereco_destino)) < 0 )
	{
		perror("sendto");
		return;
	}
    pthread_mutex_unlock(&socketx);
}


int recebe_mensagem(int socket_local, char *buffer, int tam_buffer, struct sockaddr_in *endereco_cliente)
{
    pthread_mutex_lock(&socketx);
	int bytes_recebidos;		/* Numero de bytes recebidos */

	/* Espera pela msg de resposta do servidor */
	bytes_recebidos = recvfrom(socket_local, buffer, tam_buffer, 0, (struct sockaddr *) endereco_cliente, 0);
	if (bytes_recebidos < 0)
	{
		perror("recvfrom");
	}
    pthread_mutex_unlock(&socketx);
	return bytes_recebidos;
}

int recebe_nuvem(int socket_local, char *buffer, int tam_buffer, struct sockaddr_in *endereco_cliente, int *tam_c)
{
    pthread_mutex_lock(&socketNuvem);
	int bytes_recebidos;		/* Numero de bytes recebidos */
    memset( (char *)endereco_cliente, 0, sizeof(struct sockaddr_in));
	endereco_cliente->sin_family = AF_INET;

    *tam_c = sizeof(struct sockaddr_in);
	/* Espera pela msg de resposta do servidor */
	bytes_recebidos = recvfrom(socket_local, buffer, tam_buffer, 0, (struct sockaddr *) endereco_cliente, tam_c);
	if (bytes_recebidos < 0)
	{
		perror("recvfrom");
	}
    pthread_mutex_unlock(&socketNuvem);
	return bytes_recebidos;
}

static int separaCampos( char *buffer, char *campos[], int maxCampos)
{
	char *token;
	int i = 0;
	int j;

	// Pega o primeiro token
	token = strtok(buffer," ");
	campos[i++] = token;

	while( token != NULL  &&  i<maxCampos ) {
		token = strtok(NULL, " ");		// Pega o pr??ximo token
		campos[i++] = token;
	}
	j = i-1;

	while( i < maxCampos )
		campos[i++] = NULL;			// Completa com NULL

	return j;
}


char acionaPeriferico(int socket_local, struct sockaddr_in endereco_destino, char dispositivo, char comando){
    
    char bufferIn[TAM_BUFFER];
	char bufferOut[TAM_BUFFER];
    char device[50];
    // printf("dispositivo: %d - comando: %d\n",dispositivo, comando);
    switch (dispositivo)
    {
    case BOMBACOLETOR:
        /* code */
        sprintf(device, "bombacoletor");
        break;
    case BOMBACIRCULACAO:
        /* code */
        sprintf(device, "bombacirculacao");
        break;
    case AQUECEDOR:
        /* code */
        sprintf(device, "aquecedor");
        break;
    case VALVULAENTRADA:
        /* code */
        sprintf(device, "valvulaentrada");
        break;
    case VALVULAESGOTO:
        /* code */
        sprintf(device, "valvulaesgoto");
        break;    
    default:
        return -1;
        break;
    }
    if(comando < 0 || comando > 1){
        return -1;      // erro, comando invalido
    }
    sprintf(bufferOut, "%s %d", device, comando);

    envia_mensagem(socket_local, endereco_destino, bufferOut);
    recebe_mensagem(socket_local, bufferIn, TAM_BUFFER, NULL);

    sprintf(device, "z");
    strcat(device, bufferOut);
    // printf("%s %d", bufferIn, strcmp(bufferIn, device));
    if(strcmp(bufferIn, device) == 0){  // Verifica se comando recebido corresponde ao enviado.
        return comando;
    }else{
        return -1;
    }

}

float recebeSensor(int socket_local, struct sockaddr_in endereco_destino, char dispositivo){
    
    char bufferIn[TAM_BUFFER];
	char bufferOut[TAM_BUFFER];
    char *campos[10];
    char temp[50];

    switch (dispositivo)
    {
    case NIVELBOILER:
        /* code */
        sprintf(bufferOut, "nivelboiler");
        break;
    case TEMPBOILER:
        /* code */
        sprintf(bufferOut, "tempboiler");
        break;
    case TEMPCOLETOR:
        /* code */
        sprintf(bufferOut, "tempcoletor");
        break;
    case TEMPCANOS:
        /* code */
        sprintf(bufferOut, "tempcanos");
        break;  
    default:
        return -1;
        break;
    }

    envia_mensagem(socket_local, endereco_destino, bufferOut);
    
    recebe_mensagem(socket_local, bufferIn, TAM_BUFFER, NULL);

    int nCampos = separaCampos( bufferIn, campos, 10);

    strcpy(temp, "z");
    strcat(temp,bufferOut);

    if( strcmp( campos[0], temp) == 0 ) {
        if( nCampos != 2 ) {
            return -1;
        }
        float valorSensor = atof( campos[1] );
        return valorSensor;
        
    }else{
        return -1;
    }

}
#ifndef ACIONAMENTO_H
#define ACIONAMENTO_H

#define FALHA               1
#define TAM_BUFFER          200

#define LIGA                1
#define DESLIGA             0

#define BOMBACOLETOR        0
#define BOMBACIRCULACAO     1
#define AQUECEDOR           2
#define VALVULAENTRADA      3
#define VALVULAESGOTO       4

#define NIVELBOILER         5
#define TEMPBOILER          6
#define TEMPCOLETOR         7
#define TEMPCANOS           8

int cria_socket_local(void);

struct sockaddr_in cria_endereco_destino(char *destino, int porta_destino);

void envia_mensagem(int socket_local, struct sockaddr_in endereco_destino, char *mensagem);

int recebe_mensagem(int socket_local, char *buffer, int tam_buffer);

static int separaCampos( char *buffer, char *campos[], int maxCampos);

int acionaPeriferico(int socket_local, struct sockaddr_in endereco_destino, char dispositivo, char comando);

float recebeSensor(int socket_local, struct sockaddr_in endereco_destino, char dispositivo);

#endif  //ACIONAMENTO_H
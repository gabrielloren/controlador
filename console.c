/*	******************
	* RESUMO NCURSES *
	******************

	LINES					// Numero de linhas disponiveis
	COLS					// Numero de colunas disponiveis

	initscr();				// Inicializa o terminal em modo curses

	cbreak();				// Desativa o buffer de linha, CTRL-Z e CTRL-C disparam sinais
	nocbreak();				// Ativa o buffer de linha

	raw();					// Desativa o buffer de linha, CTRL-Z e CTRL-C não disparam sinais
	noraw();				// Ativa o buffer de linha

	echo();					// Teclado ecoa no display
	noecho();				// Teclado nao ecoa no display

	keypad(stdscr, TRUE);	// Permite leitura de teclas de função
	keypad(stdscr, FALSE);	// Teclas de função geram sequencias de escape


	start_color();			// Inicializa as cores
		COLOR_BLACK
		COLOR_RED
		COLOR_GREEN
		COLOR_YELLOW
		COLOR_BLUE
		COLOR_MAGENTA
		COLOR_CYAN
		COLOR_WHITE

	init_pair(n, COLOR_RED, COLOR_BLACK);	// Define par de cores 'n', ordem eh 'foreground','background'


	attron(atribs);			// Ativa atributos
	attroff(atribs);		// Desativa atributos
			A_NORMAL 		// Exibicao normal (sem destaque)
			A_STANDOUT 		// Melhor modo de destaque do terminal
			A_UNDERLINE		// Sublinhado
			A_REVERSE		// Video reverso
			A_BLINK			// Piscando
			A_DIM			// Meio brilhante
			A_BOLD			// Extra brilhante ou negrito
			A_ALTCHARSET 	// Conjunto de caracteres alternativos
			COLOR_PAIR(n)	// Par de cores n que foi pre-definido


	printw("Texto");		// Escreve string nas coordenadas atuais
	printw("%c %d %f", c, d, f);	// Semelhante ao printf

	addch(c);					// Escreve caracter nas coordenadas atuais
	addch(ch | A_BOLD | A_UNDERLINE);


	move(linha,coluna);				// Move o cursor para linha,coluna

	mvprintw(linha,coluna,"Texto");		// Junta o move com o printw
	mvprintw(linha,coluna,"%c %d %f", c, d, f);	// Semelhante ao printf


	refresh();				// Atualiza a tela com as partes modificadas

	getch();				// Le uma tecla (buffer? eco? keypad?)

	getstr(str);			// Le um string

	endwin();				// Encerra o modo curses

*/




/*





	// Testa o conjunto de caracteres alternativos
	move(3,0);				// Move o cursor para inicio da linha 3
	attron(A_ALTCHARSET);
	printw("abcdefghijklmnopqrstuvxz059AM");
	attroff(A_ALTCHARSET);

????????????????????????????????????????????????????????????????????????????????
*/


#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<time.h>
#include	<assert.h>
#include	<pthread.h>
#include	<string.h>
#include 	<ncurses.h>


#include	"console.h"
#include 	"controlador.h"


// Linhas na tela
#define LINHA_MENSAGEM	(PLANTA_DADOS+0)
#define LINHA_ENTRADA	(PLANTA_DADOS+1)
#define LINHA_NUVEM		(PLANTA_DADOS+2)


// Pares de cores
#define AMARELO_PRETO		1
#define VERDE_PRETO			2
#define VERMELHO_PRETO		3
#define AZUL_PRETO			4


/* Texto para os dados completos
*/
static char *texto_dados[PLANTA_DADOS] = {
	"Volume de agua no boiler (l)                  ",
	"Nivel de agua no boiler (m)                   ",
	"Temperatura da agua no boiler (Celsius)       ",
	"Temperatura minima agua no boiler (Celsius)   ",
	"Temperatura da agua no coletor solar (Celsius)",
	"Temperatura da agua nos canos (Celsius)       ",
	"Indica bomba do coletor ligada ou nao         ",
	"Indica bomba de recirculação ligada ou nao    ",
	"Indica aquecedor ligado ou nao                ",
	"Indica entrada de agua aberta ou nao          ",
	"Indica saida de agua para esgoto aberta ou nao"
	};



// Mutex para compartilhar a tela entre várias threads
static pthread_mutex_t tela = PTHREAD_MUTEX_INITIALIZER;


/* Texto para os dados completos
*/
char **textosCompletos(void)
{
	return texto_dados;
}


/** Inicializa o console
* 	Retorna -1 se erro
*/
int console_modoJanela(void)
{
	initscr();				// Inicializa o terminal em modo curses
	start_color();			// Inicializa as cores
	raw();					// Desativa o buffer de linha, CTRL-Z e CTRL-C não disparam sinais
	noecho();				// Teclado nao ecoa no display
	keypad(stdscr, TRUE);	// Permite leitura de teclas de função
	
	if( LINES < PLANTA_DADOS+3 ) {
		console_modoNormal();
		printf("Tamanho (altura) do terminal não é suficiente\n");
		statusSistema.termina = 1;
		return -1;
	}
	if( COLS < 80 ) {
		console_modoNormal();
		printf("Tamanho (largura) do terminal não é suficiente\n");
		statusSistema.termina = 1;
		return -1;
	}

	init_pair(AMARELO_PRETO, COLOR_YELLOW, COLOR_BLACK);
	init_pair(VERDE_PRETO, COLOR_GREEN, COLOR_BLACK);
	init_pair(VERMELHO_PRETO, COLOR_RED, COLOR_BLACK);
	init_pair(AZUL_PRETO, COLOR_BLUE, COLOR_BLACK);

	return 0;	
}


/** Retorna o terminal ao modo normal
*/
void console_modoNormal(void)
{
	keypad(stdscr, FALSE);	// Retorna ao default
	noraw();				// Retorna ao default
	echo();					// Retorna ao default
	endwin();				// Encerra o modo curses
}


/** Move o cursor para posição de descanso
*	Não obtem mutex da tela
*/
static void descansaCursor(void)
{
	move(LINHA_ENTRADA,59);
}


/** Coloca mensagem na tela
*	Não obtem mutex da tela
*/
static void mensagem(char *s)
{
	move(LINHA_MENSAGEM,0);
	printw(s);	
}


/** Coloca string recebido do controlador na tela
*	Obtem mutex da tela
*/
static void recebidoControlador(char *s)
{
	char texto[100];
	int tam = strlen(s);

	// Trunca string para caber tudo em 80 colunas
	if(tam > 80-23 )
		s[80-23] = '\0';
	
	sprintf( texto, "[N] RECEBIDO %3d BYTES=%s", tam, s);
	pthread_mutex_lock(&tela);	
	move(LINHA_NUVEM,0);
	printw(texto);
	refresh();
	pthread_mutex_unlock(&tela);
}


/** Atualiza o terminal com os dados da planta, em modo janela
*/
void console_mostraDados(void)
{
	char **textos = textosCompletos();
	double dados[PLANTA_DADOS];
	int atrib;

	planta_dadosCompletos(dados);

	pthread_mutex_lock(&tela);

	// Linhas apenas para leitura
	for( int i=0; i<PLANTA_DADOS-5-1; ++i) {
		if( i%2==0 )
			atrib = A_NORMAL;
		else
			atrib = COLOR_PAIR(AMARELO_PRETO);
		attron(atrib);
		mvprintw(i,0,"%s: %8.3lf", textos[i], dados[i]);
		attroff(atrib);		// Desativa atributos
	}

	// Linha para +/-
	if( PLANTA_DADOS-5-3-1%2==0 )
		atrib = A_NORMAL;
	else
		atrib = COLOR_PAIR(AMARELO_PRETO);
	attron(atrib);
	mvprintw(PLANTA_DADOS-5-3-1,0,"%s: %8.3lf      [+|-]", textos[PLANTA_DADOS-5-3-1], dados[PLANTA_DADOS-5-3-1]);
	attroff(atrib);		// Desativa atributos


	// Linhas para liga/desliga
	for( int i=PLANTA_DADOS-5-1; i<PLANTA_DADOS-1; ++i) {
		if( dados[i] != 0 )
			atrib = COLOR_PAIR(VERDE_PRETO);
		else
			atrib = COLOR_PAIR(VERMELHO_PRETO);
		attron(atrib);
		mvprintw(i,0,"%s:  %3d", textos[i], (int)dados[i]);
		attroff(atrib);		// Desativa atributos
	}

	// // Linha de erro
	// if( dados[PLANTA_DADOS-1] != 0 )
	// 	atrib = COLOR_PAIR(VERMELHO_PRETO) | A_BLINK;
	// else
	// 	atrib = COLOR_PAIR(VERDE_PRETO);
	// attron(atrib);
	// mvprintw(PLANTA_DADOS-1, 0, "%s:   %s", textos[PLANTA_DADOS-1], dados[PLANTA_DADOS-1]!=0?"ERRO":"OK");
	// attroff(atrib);		// Desativa atributos
	move(LINHA_NUVEM+2,0);
	refresh();
	pthread_mutex_unlock(&tela);
}


/** Código da thread para interface via console
*/
void console_threadConsole(void)
{
	int c;
	double v;
	char *m;
	
	pthread_mutex_lock(&tela);
	move(LINHA_ENTRADA,0);
	printw("Digite + - p/comandos  ou  X p/terminar: ");
	move(LINHA_NUVEM,0);
	printw("[N] RECEBIDO     BYTES= ");
	refresh();
	pthread_mutex_unlock(&tela);

	do {
		c = getch();		// Espera por uma tecla
		switch(c) {
			// case 'n':
			// 		pthread_mutex_lock(&tela);
			// 		move(LINHA_NUVEM,0);
			// 		printw("[N] RECEBIDO --- BYTES=                                               ");
			// 		refresh();
			// 		pthread_mutex_unlock(&tela);
			// 		m = instrumentacao_leControlador();
			// 		recebidoControlador(m);
			// 		console_showPlantaTudo();
			// 		break;
			case '+':
					statusSistema.tempMinimaBoiler += 0.5;
					console_mostraDados();
					break;
			case '-':
					statusSistema.tempMinimaBoiler -= 0.5;
					console_mostraDados();
					break;
			// case KEY_F(1):
			// 		planta_acionaBombaColetor( (planta_leBombaColetor()+1) % 2 );
			// 		console_showPlantaTudo();
			// 		break;
			// case KEY_F(2):
			// 		planta_acionaBombaCirculacao( (planta_leBombaCirculacao()+1) % 2 );
			// 		console_showPlantaTudo();
			// 		break;
			// case KEY_F(3):
			// 		planta_acionaAquecedor( (planta_leAquecedor()+1) % 2 );
			// 		console_showPlantaTudo();
			// 		break;
			// case KEY_F(4):
			// 		planta_acionaValvulaEntrada( (planta_leValvulaEntrada()+1) % 2);
			// 		console_showPlantaTudo();
			// 		break;
			// case KEY_F(5):
			// 		planta_acionaValvulaEsgoto( (planta_leValvulaEsgoto()+1) % 2 );
			// 		console_showPlantaTudo();
			// 		break;
			case 'X':
			case 'x':
					pthread_mutex_lock(&tela);
					printw("TERMINAR !!!");
					refresh();
					pthread_mutex_unlock(&tela);
					statusSistema.termina = 1;
					break;
			default:
					break;
		}
	} while( statusSistema.termina == 0 );
	

}



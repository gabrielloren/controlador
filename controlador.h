#ifndef CONTROLADOR_H
#define CONTROLADOR_H

typedef struct controlador
{
    float nivelBoiler;
    float tempBoiler;
    float tempMinimaBoiler;
    float tempColetor;
    float tempCanos;
    char bombaColetor;
    char bombaCirculacao;
    char aquecedor;
    char valvulaEntrada;
    char valvulaEsgoto;
    char termina;
} controlador;

extern controlador statusSistema;

void planta_dadosCompletos(double *dados);

#endif //CONTROLADOR_H
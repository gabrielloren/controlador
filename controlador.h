#ifndef CONTROLADOR_H
#define CONTROLADOR_H

typedef struct controlador
{
    float nivelBoiler;
    float tempBoiler;
    float tempColetor;
    float tempCanos;
    char bombaColetor:1;
    char bombaCirculacao:1;
    char aquecedor:1;
    char valvulaEntrada:1;
    char valvulaEsgoto:1;
    char termina:1;
} controlador;

extern controlador statusSistema;

#endif //CONTROLADOR_H
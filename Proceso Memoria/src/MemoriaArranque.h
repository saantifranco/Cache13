/*
 * MemoriaArranque.h
 *
 *  Created on: 3/9/2015
 *      Author: utnso
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>

#ifndef MEMORIAARRANQUE_H_
#define MEMORIAARRANQUE_H_

typedef struct{
	int PUERTO_ESCUCHA;
	char* IP_SWAP;
	int PUERTO_SWAP;
	int MAXIMO_MARCOS_POR_PROCESO;
	int CANTIDAD_MARCOS;
	int TAMANIO_MARCO;
	int ENTRADAS_TLB;
	char* TLB_HABILITADA;
	double RETARDO;
	char* ALGORITMO_REEMPLAZO;
}MemoriaConfig;

MemoriaConfig* leerArchivoConfiguracion(char *ruta_archivo);

t_log* crearArchivoLog();

//variables config
int PUERTO_ESCUCHA, PUERTO_SWAP, MAXIMO_MARCOS_POR_PROCESO, CANTIDAD_MARCOS, TAMANIO_MARCO, ENTRADAS_TLB;
double RETARDO;
char* IP_SWAP;
char* TLB_HABILITADA;
char* ALGORITMO_REEMPLAZO;

#endif /* MEMORIAARRANQUE_H_ */

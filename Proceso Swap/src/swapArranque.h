/*
 * swapArranque.h
 *
 *  Created on: 3/9/2015
 *      Author: utnso
 */

#ifndef PROCESO_SWAP_SRC_SWAPARRANQUE_H_
#define PROCESO_SWAP_SRC_SWAPARRANQUE_H_

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include"/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"
#include <pthread.h>



typedef struct {
	int PUERTO_ESCUCHA;
	int TAMANIO_PAGINA;
	int CANTIDAD_PAGINAS;
	int RETARDO_COMPACTACION;
	double RETARDO_SWAP;
	char* NOMBRE_SWAP;
} swapConfig;


typedef struct {
	t_list* listaLibres;
	t_list* listaOcupados;
	t_queue* cola;
	t_log* logs;
	swapConfig* config;
	int socket;
} infoHilos;

pthread_mutex_t mutex;



swapConfig* leerArchivoConfiguracion(char *ruta_archivo);
t_log* crearArchivoLog();
void* atenderConexionesDeMemoria(void*);
void inicializarParticion(swapConfig*);



#endif /* PROCESO_SWAP_SRC_SWAPARRANQUE_H_ */

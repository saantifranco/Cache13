/*
 * PlanificadorArranque.h
 *
 *  Created on: 2/9/2015
 *      Author: utnso
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <sockets.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>


#ifndef PLANIFICADORARRANQUE_H_
#define PLANIFICADORARRANQUE_H_

typedef struct{
	int PUERTO_ESCUCHA;
	int QUANTUM;
	char* ALGORITMO_PLANIF;
}t_planificadorConfig;

t_planificadorConfig* leerArchivoConfiguracion(char *ruta_archivo);

t_log* crearArchivoLog();

#endif /* PLANIFICADORARRANQUE_H_ */

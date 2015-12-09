#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "sockets.h>
#include <commons/config.h>
#include <commons/log.h>


#ifndef CPUARRANQUE_H_
#define CPUARRANQUE_H_

//variable config
typedef struct{
	char* IP_PLANIFICADOR;
	int PUERTO_PLANIFICADOR;
	char* IP_MEMORIA;
	int PUERTO_MEMORIA;
	int CANTIDAD_HILOS;
	double RETARDO;
}CPUConfig;

CPUConfig* leerArchivoConfiguracion(char *ruta_archivo);

t_log* crearArchivoLog();

//recursos
//t_config* config;
//t_log* logs;

#endif /* CPUARRANQUE_H_ */

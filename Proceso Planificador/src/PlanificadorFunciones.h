/*
 * PlanificadorFunciones.h
 *
 *  Created on: 7/9/2015
 *      Author: utnso
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/temporal.h>
#include "PlanificadorArranque.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"


#ifndef PLANIFICADORFUNCIONES_H_
#define PLANIFICADORFUNCIONES_H_

/*-----Estructuras y Enums -----*/
typedef enum { NUEVO, LISTO, EJECUTANDO, BLOQUEADO, FINALIZADO, LISTO_SUSPENDIDO, BLOQUEADO_SUSPENDIDO } e_estado;

typedef struct{
	char* tiempoInicio;
	char* tiempoFin;
}t_periodo; //Refleja un ciclo de ejecución

typedef struct{
	char* path; //Ruta al "mCod"
	char* nombre; //Nombre del proceso
	int pid; //Process ID
	int proximaInstruccion; //Puntero a la prox instruccion
	int flagAbortado; //Flag para saber si fue finalizado
	e_estado estado;
	t_periodo* tiempoRespuesta;
	t_list* tiempoEjecucion;
	t_list* tiempoEspera;
}t_pcb;

typedef struct{
	int pid;
	int segundos;
}t_entradaSalida;

typedef struct{
	int socketCPU; //Socket para comunicar con la CPU
	int core; //Numero de CPU
	int porcentajeUso; //Cantidad de segundos que ejecutó en el minuto
	int estado; //0 para ocupada, 1 para libre
	int ultimoPidEjecutado; //Ultimo proceso que ejecuto, para manejar los retornos: 0 nunca ejecutó nada
}t_cpu;

typedef struct{
	t_list* cpuConectadas; //CPU conectadas al planificador
	t_queue* procesosListos; //Cola de procesos listos para empezar a ejecutarse (nuevos o que hayan vuelto de I/O)-->Guardo PIDs
	t_list* procesos; //Todos los procesos: Elementos del tipo t_pcb
	t_queue* entradaSalida; //Cola de pedidos de entrada/salida
	t_planificadorConfig* config;
	t_log* logs;
	int ultimoPidAsignado;
	t_list* finalizados; //Todos los procesos finlizados: Elementos del tipo t_pcb
	int cpuData; //Socket para pedir info acerca del esatod de las cpu's
}t_conexiones_solicitudes_recursos;

/*-----Semáforos -----*/
pthread_mutex_t mutex_cpus;
pthread_mutex_t mutex_procesos;
pthread_mutex_t mutex_colaReady;
pthread_mutex_t mutex_entradaSalida;
pthread_mutex_t mutex_logs;
pthread_mutex_t mutex_finalizados;
pthread_mutex_t mutex_envios;
pthread_mutex_t mutex_recepciones;

/*-----Funciones de Hilos -----*/
void* consola(void*);
void* solicitarEjecuciones(void*);
void* handlerSelect(void*);
void* dma(void*);

/*-----Funciones de Consola -----*/
void cpu_status(int);
void process_status_historico(t_list*, t_list*);
void process_status(t_list*, int);
void ps(void*);
void finalizar_PID(t_list*, int);
void finalizarProceso(t_pcb*);
void correr_PATH(t_list*, t_queue*, int*, char*, t_log*);
void core_status(t_list*, t_list*);

/*-----Funciones Auxiliares -----*/
t_pcb* get_proceso(t_list*, int );
void get_tiempo(int*, int*, int*, int*);
void convertir_tiempo(char*, int*, int*, int*, int*);
void loguear_metricas(t_log*, t_pcb*);

#endif /* PLANIFICADORFUNCIONES_H_ */

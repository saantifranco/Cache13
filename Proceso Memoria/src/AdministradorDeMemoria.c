/*
 * AdministradorDeMemoria.c
 *
 *  Created on: 3/9/2015
 *      Author: utnso
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <wait.h>
#include "MemoriaArranque.h"
#include "MemoriaFunciones.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>

void handler(int n);
pthread_t th_tlb;
pthread_t th_memoria;
t_list* memoriaPrincipal;
t_list* tlb;
t_list* listaProcesos;
t_log* logs;
MemoriaConfig* config;
int pid;
datosHandlerSelect* dataHilos;

int main(void) {

	config = leerArchivoConfiguracion("configMemoria");
	logs = crearArchivoLog();
	memoriaPrincipal = list_create();
	tlb = list_create();
	listaProcesos = list_create();
	inicializarMP(memoriaPrincipal, config);
	int punteroMP = 0;
//	int status;
	dataHilos = (datosHandlerSelect*)malloc(sizeof(datosHandlerSelect));
	dataHilos->memoriaPrincipal = memoriaPrincipal;
	dataHilos->tlb = tlb;
	dataHilos->log = logs;
	dataHilos->listaProcesos = listaProcesos;
	dataHilos->config = config;
	dataHilos->punteroMP = punteroMP;

	tlbStatus = (t_tlbStatus*) malloc(sizeof(t_tlbStatus));
	tlbStatus->aciertos=0;
	tlbStatus->ingresosTLB=0;
//	struct sigaction act;

	int socketSwap = Conectar(config->IP_SWAP, config->PUERTO_SWAP);
	if(socketSwap == -1){
		log_info(logs, "Memoria no pudo conectarse con el Swap\n");
		exit(1);
	}
	log_info(logs,"Memoria se ha conectado con el Swap\n");

	dataHilos->socketSwap = socketSwap;

//	crearFork();

	printf("MI PID ES %d\n", getpid());

	int listenningSocket = Escuchar(config->PUERTO_ESCUCHA);

	dataHilos->listenningSocket = listenningSocket;

	pthread_mutex_init(&mutex_memoria, NULL);
	pthread_mutex_init(&mutex_tlb, NULL);
	pthread_mutex_init(&mutex_logs, NULL);

	if (signal (SIGUSR1, handler) == SIG_ERR)
	{
		perror ("No se puede cambiar la señal SIGUSR1");
	}
	if (signal (SIGUSR2, handler) == SIG_ERR)
	{
		perror ("No se puede cambiar la señal SIGUSR2");
	}
	if (signal (SIGPOLL, handler) == SIG_ERR)
	{
		perror ("No se puede cambiar la señal SIGPOLL");
	}

	pthread_t th_principal;
	pthread_t th_status;

	pthread_create(&th_principal, NULL, handlerSelect, dataHilos);
	pthread_create(&th_status, NULL, temporizador, NULL);

	pthread_join(th_principal, NULL);
	pthread_join(th_status, NULL);

	Desconectar(listenningSocket);
	Desconectar(socketSwap);

	free(dataHilos);
	free(logs);
	free(config);

	return EXIT_SUCCESS;
}

void funcHiloTLB(void* data){
	datosHandlerSelect* dataHilos = (datosHandlerSelect*) data;
	t_list* tlb = dataHilos->tlb;
	t_list* listaProcesos = dataHilos->listaProcesos;
	limpiarTLB(tlb, listaProcesos);
	pthread_mutex_lock(&mutex_logs);
	log_info(logs, "Tratamiento de señal terminado");
	pthread_mutex_unlock(&mutex_logs);
	sleep(2);
}

void funcHiloMP(void* data){
	datosHandlerSelect* dataHilos = (datosHandlerSelect*) data;
	t_list* memoriaPrincipal = dataHilos->memoriaPrincipal;
	t_list* listaProcesos = dataHilos->listaProcesos;
	t_list* tlb = dataHilos->tlb;
	int socketSwap = dataHilos->socketSwap;
	limpiarMP(memoriaPrincipal, listaProcesos, tlb, socketSwap, config->CANTIDAD_MARCOS);
	pthread_mutex_lock(&mutex_logs);
	log_info(logs, "Tratamiento de señal terminado");
	pthread_mutex_unlock(&mutex_logs);
	sleep(2);
}

void handler(int n){
	switch (n) {
		case SIGUSR1:
			pthread_mutex_lock(&mutex_tlb);
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "Señal SIGUSR1 recibida, ejecutar acción: limpiar TLB");
			pthread_mutex_unlock(&mutex_logs);
			pthread_create(&th_tlb, NULL, (void*)funcHiloTLB, dataHilos);
			sleep(1);
			pthread_join(th_tlb, NULL);
			pthread_mutex_unlock(&mutex_tlb);
			break;
		case SIGUSR2:
			pthread_mutex_lock(&mutex_memoria);
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "Señal SIGUSR2 recibida, ejecutar acción: limpiar Memoria Principal");
			pthread_mutex_unlock(&mutex_logs);
			pthread_create(&th_memoria, NULL, (void*)funcHiloMP, dataHilos);
			sleep(1);
			pthread_join(th_memoria, NULL);
			pthread_mutex_unlock(&mutex_memoria);
			break;
		case SIGPOLL:
			pthread_mutex_lock(&mutex_memoria);
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "Señal SIGPOLL recibida, ejecutar acción: dump de Memoria Principal");
			pthread_mutex_unlock(&mutex_logs);
			crearFork(memoriaPrincipal, logs);
			pthread_mutex_unlock(&mutex_memoria);
			break;
	}
}

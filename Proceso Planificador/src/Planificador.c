/*
 ============================================================================
 Name        : Planificador.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
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
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <pthread.h>
#include "PlanificadorArranque.h"
#include "PlanificadorFunciones.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"

void codigoDePrueba(t_planificadorConfig*);

int main(void) {
	t_conexiones_solicitudes_recursos* dataHilos = (t_conexiones_solicitudes_recursos*) malloc(sizeof(t_conexiones_solicitudes_recursos));
	dataHilos->config = leerArchivoConfiguracion("configPlanificador");
	dataHilos->logs = crearArchivoLog();
	dataHilos->cpuConectadas = list_create();
	dataHilos->procesosListos = queue_create();
	dataHilos->procesos = list_create();
	dataHilos->entradaSalida = queue_create();
	dataHilos->ultimoPidAsignado = 0;
	dataHilos->finalizados = list_create();
	dataHilos->cpuData = 0;

	pthread_mutex_init(&mutex_cpus, NULL);
	pthread_mutex_init(&mutex_procesos, NULL);
	pthread_mutex_init(&mutex_colaReady, NULL);
	pthread_mutex_init(&mutex_entradaSalida, NULL);
	pthread_mutex_init(&mutex_logs, NULL);
	pthread_mutex_init(&mutex_finalizados, NULL);
	pthread_mutex_init(&mutex_envios, NULL);
	pthread_mutex_init(&mutex_recepciones, NULL);

	pthread_t th_consola;
	pthread_t th_callCenter;
	pthread_t th_jefesito;
	pthread_t th_secretaria;

	pthread_create(&th_consola, NULL, consola, dataHilos);
	pthread_create(&th_callCenter, NULL, handlerSelect, dataHilos);
	pthread_create(&th_jefesito, NULL, solicitarEjecuciones, dataHilos);
	pthread_create(&th_secretaria, NULL, dma, dataHilos);

	pthread_join(th_consola, NULL);
	pthread_join(th_callCenter, NULL);
	pthread_join(th_jefesito, NULL);
	pthread_join(th_secretaria, NULL);

//	codigoDePrueba(dataHilos->config);

	free(dataHilos->config->ALGORITMO_PLANIF);
	free(dataHilos->config);
	log_destroy(dataHilos->logs);

	return EXIT_SUCCESS;
}

void codigoDePrueba(t_planificadorConfig* config){
	int listenningSocket = Escuchar(config->PUERTO_ESCUCHA);
	int socketCPU = Aceptar(listenningSocket);

	t_cpu* cpu = malloc (sizeof(t_cpu));
	cpu->socketCPU = socketCPU;
	cpu->core = 1;
	cpu->estado = 1;
	cpu->porcentajeUso = 0;

	t_paquete_envio* mensaje = serializar_string("BIENVENIDO A PLANIFICADOR");
	SendAll(cpu->socketCPU, mensaje);
	free(mensaje->data);
	free(mensaje);

	t_paquete_ejecutar* paquete_ejecutar = malloc(sizeof(t_paquete_ejecutar));
	paquete_ejecutar->path = malloc(strlen("unPosiblePath.paraCpu"));
	strcpy(paquete_ejecutar->path, "unPosiblePath.paraCpu");
	paquete_ejecutar->pid = 107;
	paquete_ejecutar->proximaInstruccion = 64;
	paquete_ejecutar->rafaga = 65;
	printf("Estructura creada: %s, %d, %d, %d\n", paquete_ejecutar->path, paquete_ejecutar->pid, paquete_ejecutar->proximaInstruccion, paquete_ejecutar->rafaga);

	t_paquete_envio* paquete_envio = serializar_t_paquete_ejecutar(paquete_ejecutar);

	SendAll(cpu->socketCPU, paquete_envio);
	free(paquete_ejecutar->path);
	free(paquete_ejecutar);
	free(paquete_envio->data);
	free(paquete_envio);

	t_paquete_ejecutar* paquete_ejecutar2 = malloc(sizeof(t_paquete_ejecutar));
	paquete_ejecutar2->path = malloc(strlen("otroPosiblePath.paraCpu"));
	strcpy(paquete_ejecutar2->path, "otroPosiblePath.paraCpu");
	paquete_ejecutar2->pid = 204;
	paquete_ejecutar2->proximaInstruccion = 66;
	paquete_ejecutar2->rafaga = 67;
	printf("Estructura creada: %s, %d, %d, %d\n", paquete_ejecutar2->path, paquete_ejecutar2->pid, paquete_ejecutar2->proximaInstruccion, paquete_ejecutar2->rafaga);

	t_paquete_envio* paquete_envio2 = serializar_t_paquete_ejecutar(paquete_ejecutar2);

	SendAll(cpu->socketCPU, paquete_envio2);
	free(paquete_ejecutar2->path);
	free(paquete_ejecutar2);
	free(paquete_envio2->data);
	free(paquete_envio2);

	Enviar(cpu->socketCPU, "ADIOS AL PLANIFICADOR");

	consola(cpu);

	Desconectar(listenningSocket);
	Desconectar(cpu->socketCPU);
}

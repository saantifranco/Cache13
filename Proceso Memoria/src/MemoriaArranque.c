/*
 * MemoriaArranque.c
 *
 *  Created on: 3/9/2015
 *      Author: utnso
 */
#include "MemoriaArranque.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>

MemoriaConfig* leerArchivoConfiguracion(char *ruta_archivo) {

	t_config* config = malloc(sizeof(t_config));
	config = config_create(ruta_archivo);
	printf("Leyendo archivo de configuraciones: %s\n", ruta_archivo);

	MemoriaConfig* configProceso = (MemoriaConfig*) malloc(sizeof(MemoriaConfig));

	configProceso->PUERTO_ESCUCHA = config_get_int_value(config, "PUERTO_ESCUCHA");
	configProceso->IP_SWAP = config_get_string_value(config, "IP_SWAP");
	configProceso->PUERTO_SWAP = config_get_int_value(config, "PUERTO_SWAP");
	configProceso->MAXIMO_MARCOS_POR_PROCESO=config_get_int_value(config,"MAXIMO_MARCOS_POR_PROCESO");
	configProceso->CANTIDAD_MARCOS=config_get_int_value(config,"CANTIDAD_MARCOS");
	configProceso->TAMANIO_MARCO=config_get_int_value(config,"TAMANIO_MARCO");
	configProceso->ENTRADAS_TLB=config_get_int_value(config,"ENTRADAS_TLB");
	configProceso->TLB_HABILITADA=config_get_string_value(config,"TLB_HABILITADA");
	configProceso->RETARDO=config_get_double_value(config,"RETARDO_MEMORIA");
	configProceso->ALGORITMO_REEMPLAZO=config_get_string_value(config,"ALGORITMO_REEMPLAZO");

	free(config);

	printf("Puerto de la Memoria que escucha: %d\n", configProceso->PUERTO_ESCUCHA);
	printf("IP del Swap: %s\n", configProceso->IP_SWAP);
	printf("Puerto del Swap: %d\n", configProceso->PUERTO_SWAP);
	printf("Cantidad maxima de marcos por proceso: %d\n", configProceso->MAXIMO_MARCOS_POR_PROCESO);
	printf("Cantidad de Marcos: %d\n", configProceso->CANTIDAD_MARCOS);
	printf("Tamanio de cada marco: %d\n", configProceso->TAMANIO_MARCO);
	printf("Cantidad de entradas TLB: %d\n", configProceso->ENTRADAS_TLB);
	printf("TLB habilitada: %s\n", configProceso->TLB_HABILITADA);
	printf("Tiempo de retardo: %lf\n", configProceso->RETARDO);
	printf("Algoritmo de reemplazo: %s\n", configProceso->ALGORITMO_REEMPLAZO);
	puts("Listo.\n");

	return configProceso;
}

t_log* crearArchivoLog() {
	int devolver = 0;

	remove("logsAdministradorDeMemoria");

	puts("Creando archivo de logueo...\n");

	t_log* logs = malloc(sizeof(t_log));
	logs = log_create("logsAdministradorDeMemoria", "AdministradorDeMemoriaLog", 0, LOG_LEVEL_TRACE);

	if (logs == NULL) {
		puts("No se pudo generar el archivo de logueo\n");
		return NULL;
	}

	log_info(logs, "INICIALIZACION DEL ARCHIVO DE LOGUEO");

	puts("Listo.\n");

	return logs;
}


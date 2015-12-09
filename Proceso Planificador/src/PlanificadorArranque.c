/*
 * PlanificadorArranque.c
 *
 *  Created on: 2/9/2015
 *      Author: utnso
 */

#include "PlanificadorArranque.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>

t_planificadorConfig* leerArchivoConfiguracion(char *ruta_archivo) {
	t_config* config = config_create(ruta_archivo);
	printf("Leyendo archivo de configuraciones: %s\n", ruta_archivo);

	//printf("Tiene Algoritmo: %d \n", config_has_property(config, "ALGORITMO_PLANIF"));
	t_planificadorConfig* configProceso = (t_planificadorConfig*) malloc(sizeof(t_planificadorConfig));
	configProceso->PUERTO_ESCUCHA = config_get_int_value(config, "PUERTO_ESCUCHA");
	configProceso->ALGORITMO_PLANIF = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	configProceso->QUANTUM = config_get_int_value(config, "QUANTUM");

	//config_destroy(config);

	printf("Puerto de escucha de conexiones: %d\n", configProceso->PUERTO_ESCUCHA);
	printf("Algoritmo de planificacion a utilizar: %s\n", configProceso->ALGORITMO_PLANIF);
	printf("Quantum máximo a utilizar en los algoritmos: %d\n", configProceso->QUANTUM);

	puts("Listo.\n");
	return configProceso;
}

t_log* crearArchivoLog() {

	remove("logsPlanificador");

	puts("Creando archivo de logueo...");

	t_log* logs = log_create("logsPlanificador", "PlanificadorLog", 0, LOG_LEVEL_TRACE);

	if (logs == NULL) {
		puts("No se pudo generar el archivo de logueo\n");
		return NULL;
	}

	log_info(logs, "INICIALIZACION DEL ARCHIVO DE LOGUEO: PROCESO PLANIFICADOR");

	puts("Listo.\n");

	return logs;
}

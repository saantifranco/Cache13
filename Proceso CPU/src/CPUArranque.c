#include "CPUArranque.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>

CPUConfig* leerArchivoConfiguracion(char *ruta_archivo) {
	t_config* config = malloc(sizeof(t_config));
	config = config_create(ruta_archivo);
	printf("Leyendo archivo de configuraciones: %s\n", ruta_archivo);

	CPUConfig* configProceso = (CPUConfig*) malloc(sizeof(CPUConfig));

	configProceso->IP_PLANIFICADOR = config_get_string_value(config, "IP_PLANIFICADOR");
	configProceso->PUERTO_PLANIFICADOR = config_get_int_value(config, "PUERTO_PLANIFICADOR");
	configProceso->IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
	configProceso->PUERTO_MEMORIA = config_get_int_value(config, "PUERTO_MEMORIA");
	configProceso->CANTIDAD_HILOS = config_get_int_value(config, "CANTIDAD_HILOS");
	configProceso->RETARDO = config_get_double_value(config, "RETARDO");

	free(config);

	printf("IP del Planificador: %s\n", configProceso->IP_PLANIFICADOR);
	printf("Puerto del Planificador: %d\n", configProceso->PUERTO_PLANIFICADOR);
	printf("IP de la Memoria: %s\n", configProceso->IP_MEMORIA);
	printf("Puerto de la Memoria: %d\n", configProceso->PUERTO_MEMORIA);
	printf("Cantidad de Hilos de CPU a utilizar: %d\n", configProceso->CANTIDAD_HILOS);
	printf("Tiempo de retardo: %lf\n", configProceso->RETARDO);
	puts("Listo.\n");

	return configProceso;
}

t_log* crearArchivoLog() {

	remove("logsCPU");

	puts("Creando archivo de logueo...\n");

	t_log* logs = malloc(sizeof(t_log));
	logs = log_create("logsCPU", "CPULog", 0, LOG_LEVEL_TRACE);

	if (logs == NULL) {
		puts("No se pudo generar el archivo de logueo\n");
		return NULL;
	}

	log_info(logs, "INICIALIZACION DEL ARCHIVO DE LOGUEO");

	puts("Listo.\n");

	return logs;
}

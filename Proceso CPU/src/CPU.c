/*
 ============================================================================
 Name        : CPU.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <commons/process.h>
#include <commons/txt.h>
#include "CPUArranque.h"
#include "CPUFunciones.h"
#include <pthread.h>
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"

//

int main(void)
{

	//inicializar variable para cada hilo
	int idCPU=0;
	ejecucionIniciada = 0;
	int contadorHilosCores;
	t_conexiones_cores* dataHilos= (t_conexiones_cores*)malloc(sizeof(t_conexiones_cores));
	dataHilos->periodosEjecucion = list_create();

	pthread_mutex_init(&mutex_logs, NULL);
	pthread_mutex_init(&mutex_idCore,NULL);
	pthread_mutex_init(&mutex_Core,NULL);
	pthread_mutex_init(&mutex_envioMemoria,NULL);
	pthread_mutex_init(&mutex_listaStatus,NULL);
	pthread_t th_cores;
	pthread_t th_manager;
	pthread_t th_procesoPeriodico;

	dataHilos->configCore = leerArchivoConfiguracion("configCPU");
	retardoGlobal = dataHilos->configCore->RETARDO;
	instruccionesPorCpu = list_create();
	dataHilos->logs = crearArchivoLog();

	dataHilos->idCPU=idCPU;

	pthread_mutex_lock(&mutex_Core);
	pthread_create(&th_manager,NULL,cpu_status,dataHilos);
	//pthread_create(&th_manager,NULL,cpu_status,dataHilos);
	log_info(dataHilos->logs,"CPU activo,listo para ejecutar\n");

	for(contadorHilosCores=1;contadorHilosCores<=(dataHilos->configCore->CANTIDAD_HILOS);contadorHilosCores++)
	{

		pthread_mutex_lock(&mutex_Core);
		pthread_create(&th_cores,NULL,ejecucionCPU,dataHilos);
		t_instruccionesPorCpu* usoCore = (t_instruccionesPorCpu*) malloc(sizeof(t_instruccionesPorCpu));
		usoCore->core = idCPU;
		usoCore->instrucciones = 0;
		usoCore->uso = 0;
		list_add(instruccionesPorCpu, (void*) usoCore);
		pthread_mutex_lock(&mutex_Core);
		idCPU++;
		dataHilos->idCPU=idCPU;
		pthread_mutex_unlock(&mutex_Core);
		//pthread_mutex_lock(&mutex_idCore);
		//idCPU++;
		//pthread_mutex_unlock(&mutex_idCore);
	}
	pthread_mutex_lock(&mutex_Core);
	pthread_create(&th_procesoPeriodico,NULL,temporizador,dataHilos);

	pthread_join(th_cores,NULL);
	pthread_join(th_manager,NULL);
	pthread_join(th_procesoPeriodico, NULL);
	return EXIT_SUCCESS;
}


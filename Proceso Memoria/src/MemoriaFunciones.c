#include "MemoriaFunciones.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>
#include <wait.h>
#include <pthread.h>

void inicializarMP(t_list* memoriaPrincipal, MemoriaConfig* config){
	int tamanioMP = config->CANTIDAD_MARCOS;
	int i = 0;
	for(i=0; i<tamanioMP; i++){
		contenidoMarco* marcoMP = malloc(sizeof(contenidoMarco));
		marcoMP->marco = i;
		marcoMP->tamanio = config->TAMANIO_MARCO;
		marcoMP->pagina = -1;
		marcoMP->vacio = 1;
		marcoMP->usada = 0;
		marcoMP->modificada = 0;
		list_add(memoriaPrincipal, marcoMP);
	}
}

void* handlerSelect(void* data){
	datosHandlerSelect* dataHilos = (datosHandlerSelect*)data;
	t_list* memoriaPrincipal = dataHilos->memoriaPrincipal;
	t_list* tlb = dataHilos->tlb;
	t_log* logs = dataHilos->log;
	t_list* listaProcesos = dataHilos->listaProcesos;
	MemoriaConfig* config = dataHilos->config;
	int punteroMP = dataHilos->punteroMP;
	int socketSwap = dataHilos->socketSwap;
	int listenningSocket = dataHilos->listenningSocket;

	fd_set active_fd_set;
	fd_set read_fd_set;
	int estadoDescriptores;
	int unSocket;
	struct sockaddr_in clientname;
	size_t size;

	FD_ZERO (&active_fd_set);
	FD_SET (listenningSocket, &active_fd_set);

	while(1){
		FD_ZERO (&read_fd_set);
		read_fd_set = active_fd_set;
		estadoDescriptores = select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);

		if (estadoDescriptores == -1){
			perror ("select");
			exit (EXIT_FAILURE);
		} else {
			if(estadoDescriptores > 0){
				/* Atiendo a todos los sockets que tienen algo nuevo que debo leer. */
				for (unSocket = 0; unSocket < FD_SETSIZE; ++unSocket){
					if (FD_ISSET (unSocket, &read_fd_set)){
						if (unSocket == listenningSocket){
							/* Solicitudes de conexión sobre el socket escucha servidor. */
	//	    	            int nuevoCliente = Aceptar(listenningSocket);
							size = sizeof (clientname);
							int nuevoCliente = accept (listenningSocket, (struct sockaddr*) &clientname, &size);
							if (nuevoCliente < 0){
								puts("Error al aceptar un nuevo cliente");
								perror ("accept");
								exit (EXIT_FAILURE);
							}
							FD_SET (nuevoCliente, &active_fd_set);
							printf("Me informaron que se conecto un core\n");
						} else {
							/* Nueva información para leer en sockets ya conectados */
							t_paquete_envio* paqueteEnvio = malloc(sizeof(t_paquete_envio));
							int recibeDeCPU = RecvAll2(unSocket, paqueteEnvio);
							if (recibeDeCPU < 0){
								close(unSocket);
								FD_CLR (unSocket, &active_fd_set);
							}
							if (recibeDeCPU == 0){
								FD_CLR (unSocket, &active_fd_set);
							}
							if(recibeDeCPU > 0){
								ejecutarInstruccion(unSocket, socketSwap, listaProcesos, logs, config, memoriaPrincipal, tlb, paqueteEnvio, &punteroMP);
							}
						}
					}
				}
			}
		}
	}
}

void ejecutarInstruccion(int socketCPU, int socketSwap, t_list* listaProcesos, t_log* logs, MemoriaConfig* config, t_list* memoriaPrincipal, t_list* tlb, t_paquete_envio* paqueteEnvio, int* punteroMP){
	t_paquete_resultado_instruccion* envioCPU = malloc (sizeof(t_paquete_envio));
	t_paquete_para_memoria* paqueteInstruccion = malloc(sizeof(t_paquete_para_memoria));
	t_paquete_envio* paqueteEnvioACPU = malloc(sizeof(t_paquete_envio));

	int error;
	int send;
	t_paquete_con_pagina* paqueteConPagina = malloc(sizeof(t_paquete_con_pagina));
	t_paquete_con_instruccion* paqueteParaSwap = malloc(sizeof(t_paquete_con_instruccion));
	t_paquete_envio* paqueteEnvioSwap = malloc(sizeof(t_paquete_envio));
	t_paquete_envio* paqueteRetornoSwap = malloc(sizeof(t_paquete_envio));
	t_paquete_resultado_instruccion* paquete_de_Swap = malloc(sizeof(t_paquete_resultado_instruccion));
	t_paquete_fin* paquetePorFin = malloc(sizeof(t_paquete_fin));
	int recibeDeSwap;

	paqueteInstruccion = deserializar_t_paquete_de_CPU_ejecutar(paqueteEnvio);

	switch(paqueteInstruccion->instruccion){
		case 0:
			paqueteParaSwap = transformarParaSwap(paqueteInstruccion);
			paqueteEnvioSwap = serializarConInstruccion(paqueteParaSwap);
			send = SendAll(socketSwap,paqueteEnvioSwap);
			recibeDeSwap = RecvAll2(socketSwap, paqueteRetornoSwap);
			paqueteConPagina = deserializarInicioConPaginas(paqueteRetornoSwap);
			crearEstructurasDeProceso(paqueteInstruccion, config, listaProcesos, envioCPU, logs, memoriaPrincipal, tlb, paqueteConPagina);
			paqueteEnvioACPU = envioA_CPU_Serializer(envioCPU);
			error = SendAll(socketCPU,paqueteEnvioACPU);
			break;
		case 1:
			pthread_mutex_lock(&mutex_memoria);
			pthread_mutex_lock(&mutex_tlb);
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "Solicitud de lectura recibida para mProc con PID %d de la página %d", paqueteInstruccion->PID, paqueteInstruccion->pagina);
			pthread_mutex_unlock(&mutex_logs);
			leerPagina(paquete_de_Swap, envioCPU, logs, paqueteInstruccion, config, memoriaPrincipal, tlb, paqueteParaSwap, paqueteEnvioSwap, socketSwap, paqueteRetornoSwap, listaProcesos, punteroMP);
			pthread_mutex_unlock(&mutex_tlb);
			pthread_mutex_unlock(&mutex_memoria);
			paqueteEnvioACPU = envioA_CPU_Serializer(envioCPU);
			error = SendAll(socketCPU,paqueteEnvioACPU);
			break;
		case 2:
			pthread_mutex_lock(&mutex_memoria);
			pthread_mutex_lock(&mutex_tlb);
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "Solicitud de escritura recibida para mProc con PID %d de la página %d", paqueteInstruccion->PID, paqueteInstruccion->pagina);
			pthread_mutex_unlock(&mutex_logs);
			escribirPagina(paquete_de_Swap, envioCPU, logs, paqueteInstruccion, config, memoriaPrincipal, tlb, paqueteParaSwap, paqueteEnvioSwap, socketSwap, paqueteRetornoSwap, listaProcesos, punteroMP);
			pthread_mutex_unlock(&mutex_tlb);
			pthread_mutex_unlock(&mutex_memoria);
			paqueteEnvioACPU = envioA_CPU_Serializer(envioCPU);
			error = SendAll(socketCPU,paqueteEnvioACPU);
			break;
		case 3:
			paquetePorFin = transformarParaSwapFin(paqueteInstruccion);
			paqueteEnvioSwap = serializarPorFin(paquetePorFin);
			send = SendAll(socketSwap,paqueteEnvioSwap);
			eliminarEstructurasDeProceso(paqueteInstruccion, listaProcesos, memoriaPrincipal, tlb, logs);
			break;
	}

	free(paqueteConPagina);
	free(envioCPU);
	free(paquete_de_Swap);
	free(paqueteInstruccion);
	free(paqueteEnvio);
	free(paqueteEnvioACPU);
	free(paqueteParaSwap);
	free(paqueteRetornoSwap);
	free(paqueteEnvioSwap);
	free(paquetePorFin);
}

t_paquete_con_instruccion* transformarParaSwap(t_paquete_para_memoria* paqueteInstruccion){
	t_paquete_con_instruccion* paqueteParaSwap = malloc(sizeof(t_paquete_con_instruccion));
	paqueteParaSwap->instruccion = paqueteInstruccion->instruccion;
	paqueteParaSwap->cantPaginas = paqueteInstruccion->pagina;
	paqueteParaSwap->pid = paqueteInstruccion->PID;

	return paqueteParaSwap;
}

t_paquete_fin* transformarParaSwapFin(t_paquete_para_memoria* paqueteInstruccion){
	t_paquete_fin* paqueteParaSwap = malloc(sizeof(t_paquete_fin));
	paqueteParaSwap->instruccion = paqueteInstruccion->instruccion;
	paqueteParaSwap->pid = paqueteInstruccion->PID;

	return paqueteParaSwap;
}

void crearEstructurasDeProceso(t_paquete_para_memoria* paqueteInstruccion, MemoriaConfig* config, t_list* listaProcesos, t_paquete_resultado_instruccion* envioCPU, t_log* logs, t_list* memoriaPrincipal, t_list* tlb, t_paquete_con_pagina* paqueteConPagina){
	mProcMemoria* procesoNuevo = malloc(sizeof(mProcMemoria));
	if(paqueteConPagina->error==0){
		int i=0;
		procesoNuevo->pid = paqueteInstruccion->PID;
		procesoNuevo->cantPaginas = paqueteInstruccion->pagina;
		procesoNuevo->marcosAsignados = list_create();
		procesoNuevo->puntero = 0;
		procesoNuevo->fallos = 0;
		procesoNuevo->accedidas = 0;
		procesoNuevo->accesosSwap = 0;
		procesoNuevo->tablaDePaginas = list_create();
		for(i=0; i<procesoNuevo->cantPaginas;i++){
			estadoPagina* elemento = malloc(sizeof(estadoPagina));
			elemento->pagina = paqueteConPagina->pagInicial + i;
			elemento->marco = config->CANTIDAD_MARCOS; //Le asigno un número de marco que no existe
			elemento->modificado = 0;
			elemento->presencia = 0;
			elemento->instantes = 0;
			list_add(procesoNuevo->tablaDePaginas, elemento);
		}
		list_add(listaProcesos, procesoNuevo);
		envioCPU->contenido = string_duplicate(paqueteConPagina->contenido);
		envioCPU->error = paqueteConPagina->error;
		pthread_mutex_lock(&mutex_logs);
		log_info(logs, "Proceso mProc creado con PID: %d y %d paǵinas asignadas", procesoNuevo->pid, procesoNuevo->cantPaginas);
		pthread_mutex_unlock(&mutex_logs);
	} else {
		envioCPU->contenido = string_duplicate(paqueteConPagina->contenido);
		envioCPU->error = paqueteConPagina->error;
		pthread_mutex_lock(&mutex_logs);
		log_info(logs, "Error al intentar iniciar el proceso");
		pthread_mutex_unlock(&mutex_logs);
	}
}

void eliminarEstructurasDeProceso(t_paquete_para_memoria* paqueteInstruccion, t_list* listaProcesos, t_list* memoriaPrincipal, t_list* tlb, t_log* logs){
	int i;
	mProcMemoria* procesoBuscado = buscarYRemoverProceso(listaProcesos, paqueteInstruccion);
	printf("Proceso %d finalizado con un total de %d page faults y %d accesos a Swap\n", procesoBuscado->pid, procesoBuscado->fallos, procesoBuscado->accesosSwap);
	pthread_mutex_lock(&mutex_logs);
	log_info(logs, "Proceso %d finalizado con un total de %d page faults y %d accesos a Swap\n", procesoBuscado->pid, procesoBuscado->fallos, procesoBuscado->accesosSwap);
	pthread_mutex_unlock(&mutex_logs);
	for(i=0; i<list_size(procesoBuscado->marcosAsignados); i++){
		contenidoMarco* marcoAEliminar = (contenidoMarco*)list_get(procesoBuscado->marcosAsignados, i);
		pthread_mutex_lock(&mutex_memoria);
		limpiarProcesoDeMP(marcoAEliminar, memoriaPrincipal);
		pthread_mutex_unlock(&mutex_memoria);
		pthread_mutex_lock(&mutex_tlb);
		limpiarProcesoDeTLB(marcoAEliminar, tlb);
		pthread_mutex_unlock(&mutex_tlb);
	}
	free(procesoBuscado);
}

void limpiarProcesoDeMP(contenidoMarco* marcoAEliminar, t_list* memoriaPrincipal){
	int j;
	for(j=0; j<list_size(memoriaPrincipal); j++){
		contenidoMarco* marcoAVaciarDeMP = (contenidoMarco*)list_get(memoriaPrincipal, j);
		if(marcoAEliminar->marco == marcoAVaciarDeMP->marco){
			marcoAVaciarDeMP->modificada = 0;
			marcoAVaciarDeMP->usada = 0;
			marcoAVaciarDeMP->vacio = 1;
			marcoAVaciarDeMP->pagina = -1;
			marcoAVaciarDeMP->contenidoPagina = NULL;
		}
	}
}

void limpiarProcesoDeTLB(contenidoMarco* marcoAEliminar, t_list* tlb){
	int j;
	for(j=0; j<list_size(tlb); j++){
		contenidoMarco* marcoAEliminarDeMP = (contenidoMarco*)list_get(tlb, j);
		if(marcoAEliminar->marco == marcoAEliminarDeMP->marco){
			contenidoMarco* marcoASacar = list_remove(tlb, j);
		}
	}
}

mProcMemoria* buscarYRemoverProceso(t_list* listaProcesos, t_paquete_para_memoria* procesoABorrar){
	mProcMemoria* mProcADevolver;
	int i=0;
	for(i=0;i<list_size(listaProcesos);i++){
		mProcMemoria* mProc = (mProcMemoria*) list_get(listaProcesos,i);
		if(mProc->pid==procesoABorrar->PID){
			mProcADevolver = (mProcMemoria*) list_remove(listaProcesos, i);
		}
	}
	return mProcADevolver;
}

void leerPagina(t_paquete_resultado_instruccion* paquete_de_Swap, t_paquete_resultado_instruccion* envioCPU, t_log* logs, t_paquete_para_memoria* paqueteInstruccion, MemoriaConfig* config, t_list* memoriaPrincipal, t_list* tlb, t_paquete_con_instruccion* paqueteParaSwap, t_paquete_envio* paqueteEnvioSwap, int socketSwap, t_paquete_envio* paqueteRetornoSwap, t_list* listaProcesos, int* punteroMP){
	int numPaginaBuscada;
	mProcMemoria* proceso = buscarProcesoPorPID(listaProcesos, paqueteInstruccion);
	proceso->accedidas++;
	estadoPagina* paginaInicial = (estadoPagina*) list_get(proceso->tablaDePaginas, 0);
	numPaginaBuscada = paginaInicial->pagina + paqueteInstruccion->pagina;
	if(!(strcmp(config->TLB_HABILITADA, "SI"))){
		tlbStatus->ingresosTLB++;
		if(estaEnLista(numPaginaBuscada, tlb)){
			tlbStatus->aciertos++;
			contenidoMarco* marco = obtenerMarco(paqueteInstruccion, tlb, envioCPU, numPaginaBuscada);
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "TLB hit para página %d, que está en el marco %d", paqueteInstruccion->pagina, marco->marco);
			pthread_mutex_unlock(&mutex_logs);
			aumentarInstantes(proceso, marco->marco, config->CANTIDAD_MARCOS);
		} else {
			buscarEnMemoria(paquete_de_Swap, envioCPU, logs, paqueteInstruccion, memoriaPrincipal, paqueteParaSwap, paqueteEnvioSwap, socketSwap, paqueteRetornoSwap, config, tlb, listaProcesos, numPaginaBuscada, punteroMP);
		}
	} else {
		buscarEnMemoria(paquete_de_Swap, envioCPU, logs, paqueteInstruccion, memoriaPrincipal, paqueteParaSwap, paqueteEnvioSwap, socketSwap, paqueteRetornoSwap, config, tlb, listaProcesos, numPaginaBuscada, punteroMP);
	}
}

void escribirPagina(t_paquete_resultado_instruccion* paquete_de_Swap, t_paquete_resultado_instruccion* envioCPU, t_log* logs, t_paquete_para_memoria* paqueteInstruccion, MemoriaConfig* config, t_list* memoriaPrincipal, t_list* tlb, t_paquete_con_instruccion* paqueteParaSwap, t_paquete_envio* paqueteEnvioSwap, int socketSwap, t_paquete_envio* paqueteRetornoSwap, t_list* listaProcesos, int* punteroMP){
	int numPaginaBuscada;
	mProcMemoria* proceso = buscarProcesoPorPID(listaProcesos, paqueteInstruccion);
	proceso->accedidas++;
	estadoPagina* paginaInicial = (estadoPagina*) list_get(proceso->tablaDePaginas, 0);
	numPaginaBuscada = paginaInicial->pagina + paqueteInstruccion->pagina;
	if(!(strcmp(config->TLB_HABILITADA, "SI"))){
		tlbStatus->ingresosTLB++;
		if(estaEnLista(numPaginaBuscada, tlb)){
			tlbStatus->aciertos++;
			contenidoMarco* marco = obtenerMarcoYEscribir(paqueteInstruccion, tlb, numPaginaBuscada, proceso);
			obtenerMarcoYEscribir(paqueteInstruccion, memoriaPrincipal, numPaginaBuscada, proceso);
			envioCPU->contenido = "nada";
			envioCPU->error = 0;
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "TLB hit para página %d, que está en el marco %d", paqueteInstruccion->pagina, marco->marco);
			pthread_mutex_unlock(&mutex_logs);
			aumentarInstantes(proceso, marco->marco, config->CANTIDAD_MARCOS);
		} else {
			buscarEnMemoriaParaEscritura(paquete_de_Swap, envioCPU, logs, paqueteInstruccion, memoriaPrincipal, paqueteParaSwap, paqueteEnvioSwap, socketSwap, paqueteRetornoSwap, config, tlb, listaProcesos, numPaginaBuscada, punteroMP);
		}
	} else {
		buscarEnMemoriaParaEscritura(paquete_de_Swap, envioCPU, logs, paqueteInstruccion, memoriaPrincipal, paqueteParaSwap, paqueteEnvioSwap, socketSwap, paqueteRetornoSwap, config, tlb, listaProcesos, numPaginaBuscada, punteroMP);
	}
}

void buscarEnMemoria(t_paquete_resultado_instruccion* paquete_de_Swap, t_paquete_resultado_instruccion* envioCPU, t_log* logs, t_paquete_para_memoria* paqueteInstruccion, t_list* memoriaPrincipal, t_paquete_con_instruccion* paqueteParaSwap, t_paquete_envio* paqueteEnvioSwap, int socketSwap, t_paquete_envio* paqueteRetornoSwap, MemoriaConfig* config, t_list* tlb, t_list* listaProcesos, int numPaginaBuscada, int* punteroMP){
	if(estaEnLista(numPaginaBuscada, memoriaPrincipal)){
		contenidoMarco* marco = obtenerMarco(paqueteInstruccion, memoriaPrincipal, envioCPU, numPaginaBuscada);
		mProcMemoria* proceso = buscarProcesoPorPID(listaProcesos, paqueteInstruccion);
		aumentarInstantes(proceso, marco->marco, config->CANTIDAD_MARCOS);
		if(!(strcmp(config->TLB_HABILITADA,"SI"))){
			cargarMarcoEnTLB(marco, tlb, config, listaProcesos, numPaginaBuscada, logs);
		}
		pthread_mutex_lock(&mutex_logs);
		log_info(logs, "Acceso a memoria realizado por proceso con PID %d por la página: %d, que está en el marco %d", paqueteInstruccion->PID, paqueteInstruccion->pagina, marco->marco);
		pthread_mutex_unlock(&mutex_logs);
		usleep(config->RETARDO * 1000000);
	} else {
		contenidoMarco* marco = (contenidoMarco*) malloc(sizeof(contenidoMarco));
		mProcMemoria* proceso = buscarProcesoPorPID(listaProcesos, paqueteInstruccion);
		proceso->fallos++;
		proceso->accesosSwap++;
		marco->pagina = numPaginaBuscada;
		marco->tamanio = config->TAMANIO_MARCO;
		pthread_mutex_lock(&mutex_logs);
		log_info(logs, "Acceso a Swap realizado para lectura por el proceso con PID %d", paqueteInstruccion->PID);
		pthread_mutex_unlock(&mutex_logs);
		int recibeDeSwap;
		int send;
		paqueteParaSwap = transformarParaSwap(paqueteInstruccion);
		paqueteEnvioSwap = serializarConInstruccion(paqueteParaSwap);
		send = SendAll(socketSwap, paqueteEnvioSwap);
		recibeDeSwap = RecvAll2(socketSwap, paqueteRetornoSwap);
		paquete_de_Swap = deserializar_t_paquete_resultado_instruccion(paqueteRetornoSwap);
		marco->contenidoPagina = string_duplicate(paquete_de_Swap->contenido);
		marco->usada = 1;
		marco->modificada = 0;
		marco->vacio = 0;
		cargarMarcoEnMemoria(marco, memoriaPrincipal, config, listaProcesos, numPaginaBuscada, paqueteInstruccion, socketSwap, tlb, punteroMP, logs);
		envioCPU->contenido = string_duplicate(paquete_de_Swap->contenido);
		envioCPU->error = paquete_de_Swap->error;
	}
}

void buscarEnMemoriaParaEscritura(t_paquete_resultado_instruccion* paquete_de_Swap, t_paquete_resultado_instruccion* envioCPU, t_log* logs, t_paquete_para_memoria* paqueteInstruccion, t_list* memoriaPrincipal, t_paquete_con_instruccion* paqueteParaSwap, t_paquete_envio* paqueteEnvioSwap, int socketSwap, t_paquete_envio* paqueteRetornoSwap, MemoriaConfig* config, t_list* tlb, t_list* listaProcesos, int numPaginaBuscada, int* punteroMP){
	if(estaEnLista(numPaginaBuscada, memoriaPrincipal)){
		mProcMemoria* proceso = buscarProcesoPorPID(listaProcesos, paqueteInstruccion);
		contenidoMarco* marco = obtenerMarcoYEscribir(paqueteInstruccion, memoriaPrincipal, numPaginaBuscada, proceso);
		envioCPU->contenido = "nada";
		envioCPU->error = 0;
		aumentarInstantes(proceso, marco->marco, config->CANTIDAD_MARCOS);
		if(!(strcmp(config->TLB_HABILITADA,"SI"))){
			cargarMarcoEnTLB(marco, tlb, config, listaProcesos, numPaginaBuscada, logs);
		}
		pthread_mutex_lock(&mutex_logs);
		log_info(logs, "Acceso a memoria realizado por proceso con PID %d por la página: %d, que está en el marco %d", paqueteInstruccion->PID, paqueteInstruccion->pagina, marco->marco);
		pthread_mutex_unlock(&mutex_logs);
		usleep(config->RETARDO * 1000000);
	} else {
		contenidoMarco* marco = (contenidoMarco*) malloc(sizeof(contenidoMarco));
		marco->tamanio = config->TAMANIO_MARCO;
		if(strlen(paqueteInstruccion->texto) < marco->tamanio){
			mProcMemoria* proceso = buscarProcesoPorPID(listaProcesos, paqueteInstruccion);
			proceso->fallos++;
			proceso->accesosSwap++;
			marco->pagina = numPaginaBuscada;
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "Acceso a Swap realizado por el proceso con PID %d", paqueteInstruccion->PID);
			pthread_mutex_unlock(&mutex_logs);
			int recibeDeSwap;
			int send;
			paqueteInstruccion->instruccion = 1;
			paqueteParaSwap = transformarParaSwap(paqueteInstruccion);
			paqueteEnvioSwap = serializarConInstruccion(paqueteParaSwap);
			send = SendAll(socketSwap, paqueteEnvioSwap);
			recibeDeSwap = RecvAll2(socketSwap, paqueteRetornoSwap);
			paquete_de_Swap = deserializar_t_paquete_resultado_instruccion(paqueteRetornoSwap);
			marco->contenidoPagina = string_duplicate(paqueteInstruccion->texto);
			marco->usada = 1;
			marco->modificada = 1;
			marco->vacio = 0;
			asignarValorModificacion(proceso, numPaginaBuscada, 1);
			cargarMarcoEnMemoria(marco, memoriaPrincipal, config, listaProcesos, numPaginaBuscada, paqueteInstruccion, socketSwap, tlb, punteroMP, logs);
			envioCPU->contenido = "nada";
			envioCPU->error = 0;
		} else {
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "No se pudo escribir la pagina porque su contenido es mayor al tamaño del marco");
			pthread_mutex_unlock(&mutex_logs);
			envioCPU->contenido = "nada";
			envioCPU->error = -1;
		}
	}
}


void cargarMarcoEnTLB(contenidoMarco* marco, t_list* tlb, MemoriaConfig* config, t_list* listaProcesos, int numPaginaBuscada, t_log* logs){
	int i;
	mProcMemoria* procesoBuscado = buscarProcesoPorPagina(listaProcesos, numPaginaBuscada);
	if(list_size(tlb) < config->ENTRADAS_TLB){
		list_add(tlb, marco);
	} else {
		reemplazarMarcoTLB(marco, tlb, config, procesoBuscado, listaProcesos, logs);
	}
	for(i=0; i<list_size(procesoBuscado->tablaDePaginas); i++){
		estadoPagina* elemento = (estadoPagina*) list_get(procesoBuscado->tablaDePaginas,i);
		if(elemento->pagina == numPaginaBuscada){
			elemento->presencia = 1;
		}
	}
}

void cargarMarcoEnMemoria(contenidoMarco* marco, t_list* memoriaPrincipal, MemoriaConfig* config, t_list* listaProcesos, int numPaginaBuscada, t_paquete_para_memoria* paqueteInstruccion, int socketSwap, t_list* tlb, int* punteroMP, t_log* logs){
	mProcMemoria* procesoBuscado = buscarProcesoPorPagina(listaProcesos, numPaginaBuscada);
	if((list_size(procesoBuscado->marcosAsignados)) < config->MAXIMO_MARCOS_POR_PROCESO){
		t_list* marcosVacios = list_create();
		int i;
		for(i=0; i<list_size(memoriaPrincipal); i++){
			contenidoMarco* marcoVacio = (contenidoMarco*) list_get(memoriaPrincipal, i);
			if(marcoVacio->vacio){
				list_add(marcosVacios, marcoVacio);
			}
		}
		if(list_size(marcosVacios) > 0){
			contenidoMarco* marcoASacar = (contenidoMarco*) list_get(marcosVacios, 0);
			contenidoMarco* marcoEnMemoria = buscarMarco(memoriaPrincipal, marcoASacar->marco);
			copiarValoresSinMarco(marcoEnMemoria, marco);
			if(!(strcmp(config->TLB_HABILITADA,"SI"))){
				cargarMarcoEnTLB(marcoEnMemoria, tlb, config, listaProcesos, numPaginaBuscada, logs);
			}
			list_add(procesoBuscado->marcosAsignados, marcoEnMemoria);
			aumentarInstantes(procesoBuscado, marcoEnMemoria->marco, config->CANTIDAD_MARCOS);
			agregarElMarcoAlProceso(procesoBuscado, numPaginaBuscada, marcoEnMemoria->marco);
		} else if(list_size(procesoBuscado->marcosAsignados)>0){
			int j;
			t_list* listaMarcosProcesoEnTLB = list_create();
			for(j=0; j<list_size(procesoBuscado->tablaDePaginas); j++){
				estadoPagina* elemento = (estadoPagina*) list_get(procesoBuscado->tablaDePaginas, j);
				if(elemento->presencia == 1){
					contenidoMarco* marcoEnTLB = buscarMarco(tlb, elemento->marco);
					list_add(listaMarcosProcesoEnTLB, marcoEnTLB);
				}
			}
			reemplazarMarcoPorMaximos(marco, memoriaPrincipal, config, procesoBuscado, paqueteInstruccion, socketSwap, tlb, listaMarcosProcesoEnTLB, listaProcesos, punteroMP, logs);
		} else {
			int send;
			t_paquete_fin* paquetePorFin = (t_paquete_fin*)malloc(sizeof(t_paquete_fin));
			t_paquete_envio* paqueteEnvioSwap = (t_paquete_envio*)malloc(sizeof(t_paquete_envio));
			paquetePorFin->instruccion = 3;
			paquetePorFin->pid = paqueteInstruccion->PID;
			paqueteEnvioSwap = serializarPorFin(paquetePorFin);
			send = SendAll(socketSwap,paqueteEnvioSwap);
			eliminarEstructurasDeProceso(paqueteInstruccion, listaProcesos, memoriaPrincipal, tlb, logs);
			pthread_mutex_lock(&mutex_logs);
			log_info(logs, "Proceso %d finalizado porque no se le pudo asignar ningún marco", paquetePorFin->pid);
			pthread_mutex_unlock(&mutex_logs);
		}
	} else {
		int j;
		t_list* listaMarcosProcesoEnTLB = list_create();
		for(j=0; j<list_size(procesoBuscado->tablaDePaginas); j++){
			estadoPagina* elemento = (estadoPagina*) list_get(procesoBuscado->tablaDePaginas, j);
			if(elemento->presencia == 1){
				contenidoMarco* marcoEnTLB = buscarMarco(tlb, elemento->marco);
				list_add(listaMarcosProcesoEnTLB, marcoEnTLB);
			}
		}
		reemplazarMarcoPorMaximos(marco, memoriaPrincipal, config, procesoBuscado, paqueteInstruccion, socketSwap, tlb, listaMarcosProcesoEnTLB, listaProcesos, punteroMP, logs);
	}
}

void reemplazarMarcoTLB(contenidoMarco* marco, t_list* tlb, MemoriaConfig* config, mProcMemoria* procesoBuscado, t_list* listaProcesos, t_log* logs){
	contenidoMarco* marcoAReemplazar = list_remove(tlb, 0);
	int i;
	mProcMemoria* proceso = buscarProcesoPorPagina(listaProcesos, marcoAReemplazar->pagina);
	list_add(tlb, marco);
	pthread_mutex_lock(&mutex_logs);
	log_info(logs, "Página %d del marco %d, reemplazada por página %d del marco %d", marcoAReemplazar->pagina, marcoAReemplazar->marco, marco->pagina, marco->marco);
	pthread_mutex_unlock(&mutex_logs);
	for(i=0; i<list_size(proceso->tablaDePaginas); i++){
		estadoPagina* elemento = (estadoPagina*) list_get(proceso->tablaDePaginas,i);
		if(elemento->pagina == marcoAReemplazar->pagina){
			elemento->presencia = 0;
		}
	}
}

void reemplazarMarcoPorMaximos(contenidoMarco* marco, t_list* memoriaPrincipal, MemoriaConfig* config, mProcMemoria* proceso, t_paquete_para_memoria* paqueteInstruccion, int socketSwap, t_list* tlb, t_list* listaMarcosProcesoEnTLB, t_list* listaProcesos, int* punteroMP, t_log* logs){
	int i, j;
	pthread_mutex_lock(&mutex_logs);
	log_info(logs, "Estado inicial de las colas: ");
	if(!(strcmp(config->ALGORITMO_REEMPLAZO, "FIFO"))){
		for(i=0; i<list_size(proceso->marcosAsignados); i++){
			contenidoMarco* marcoALoguear = (contenidoMarco*)list_get(proceso->marcosAsignados, i);
			log_info(logs, "Marco: %d, Pagina: %d", marcoALoguear->marco, marcoALoguear->pagina);
		}
	} else if(!(strcmp(config->ALGORITMO_REEMPLAZO, "LRU"))){
		estadoPagina* elementoALoguear;
		for(i=0; i<list_size(proceso->marcosAsignados); i++){
			contenidoMarco* marcoALoguear = (contenidoMarco*)list_get(proceso->marcosAsignados, i);
			for(j=0; j<list_size(proceso->tablaDePaginas); j++){
				estadoPagina* elemento = (estadoPagina*)list_get(proceso->tablaDePaginas, j);
				if(marcoALoguear->pagina == elemento->pagina){
					elementoALoguear = elemento;
				}
			}
			log_info(logs, "Marco: %d, Pagina: %d, referenciada hace %d instantes", marcoALoguear->marco, marcoALoguear->pagina, elementoALoguear->instantes);
		}
	} else if((!(strcmp(config->ALGORITMO_REEMPLAZO, "CLOCK"))) || (!(strcmp(config->ALGORITMO_REEMPLAZO, "CLOCK-M")))){
		for(i=0; i<list_size(proceso->marcosAsignados); i++){
			contenidoMarco* marcoALoguear = (contenidoMarco*)list_get(proceso->marcosAsignados, i);
			log_info(logs, "Marco: %d, Pagina: %d, U=%d, M=%d", marcoALoguear->marco, marcoALoguear->pagina, marcoALoguear->usada, marcoALoguear->modificada);
		}
		log_info(logs, "El puntero queda en el marco %d", *punteroMP);
	}
	pthread_mutex_unlock(&mutex_logs);
	contenidoMarco* marcoAReemplazar = aplicarAlgoritmo(config, proceso->marcosAsignados, config->MAXIMO_MARCOS_POR_PROCESO, &(proceso->puntero), proceso);
	contenidoMarco* marcoAuxiliar = copiarMarco(marcoAReemplazar);
	estadoPagina* pagInicial = (estadoPagina*) list_get(proceso->tablaDePaginas,0);
	contenidoMarco* marcoEnMemoria = buscarMarco(memoriaPrincipal, marcoAReemplazar->marco);
	copiarValoresSinMarco(marcoAReemplazar, marco);
	copiarValoresSinMarco(marcoEnMemoria, marco);
	aumentarInstantes(proceso, marcoEnMemoria->marco, config->CANTIDAD_MARCOS);
	if(!(strcmp(config->ALGORITMO_REEMPLAZO, "FIFO"))){
		buscarYRemoverMarco(proceso->marcosAsignados, marcoAuxiliar);
		list_add(proceso->marcosAsignados, marcoEnMemoria);
	}
	pthread_mutex_lock(&mutex_logs);
	log_info(logs, "Estado final de las colas: ");
	if(!(strcmp(config->ALGORITMO_REEMPLAZO, "FIFO"))){
		for(i=0; i<list_size(proceso->marcosAsignados); i++){
			contenidoMarco* marcoALoguear = (contenidoMarco*)list_get(proceso->marcosAsignados, i);
			log_info(logs, "Marco: %d, Pagina: %d", marcoALoguear->marco, marcoALoguear->pagina);
		}
	} else if(!(strcmp(config->ALGORITMO_REEMPLAZO, "LRU"))){
		estadoPagina* elementoALoguear;
		for(i=0; i<list_size(proceso->marcosAsignados); i++){
			contenidoMarco* marcoALoguear = (contenidoMarco*)list_get(proceso->marcosAsignados, i);
			for(j=0; j<list_size(proceso->tablaDePaginas); j++){
				estadoPagina* elemento = (estadoPagina*)list_get(proceso->tablaDePaginas, j);
				if(marcoALoguear->pagina == elemento->pagina){
					elementoALoguear = elemento;
				}
			}
			log_info(logs, "Marco: %d, Pagina: %d, referenciada hace %d instantes", marcoALoguear->marco, marcoALoguear->pagina, elementoALoguear->instantes);
		}
	} else if((!(strcmp(config->ALGORITMO_REEMPLAZO, "CLOCK"))) || (!(strcmp(config->ALGORITMO_REEMPLAZO, "CLOCK-M")))){
		*punteroMP = marcoAReemplazar->marco + 1;
		if(*punteroMP == config->CANTIDAD_MARCOS) *punteroMP = 0;
		for(i=0; i<list_size(proceso->marcosAsignados); i++){
			contenidoMarco* marcoALoguear = (contenidoMarco*)list_get(proceso->marcosAsignados, i);
			log_info(logs, "Marco: %d, Pagina: %d, U=%d, M=%d", marcoALoguear->marco, marcoALoguear->pagina, marcoALoguear->usada, marcoALoguear->modificada);
		}
		log_info(logs, "El puntero queda en el marco %d", *punteroMP);
	}
	pthread_mutex_unlock(&mutex_logs);
	sacarMarcoAlProceso(proceso, marcoAuxiliar->pagina, config->CANTIDAD_MARCOS);
	agregarElMarcoAlProceso(proceso, marcoEnMemoria->pagina, marcoEnMemoria->marco);
	if(!(strcmp(config->TLB_HABILITADA,"SI"))){
		if(estaEnLista(marcoAuxiliar->pagina, tlb)){
			if(!(list_size(listaMarcosProcesoEnTLB) < config->MAXIMO_MARCOS_POR_PROCESO)){
				reemplazarMarcoPorMaximosTLB(marco, tlb, marcoAuxiliar, config, proceso, logs);
			} else {
				int i;
				buscarYRemoverMarco(tlb, marcoAuxiliar);
				for(i=0; i<list_size(proceso->tablaDePaginas); i++){
					estadoPagina* elemento = (estadoPagina*) list_get(proceso->tablaDePaginas,i);
					if(elemento->pagina == marcoAuxiliar->pagina){
						elemento->presencia = 0;
					}
				}
				cargarMarcoEnTLB(marco, tlb, config, listaProcesos, marco->pagina, logs);
			}
		}
	}
	estadoPagina* elemento = obtenerMarcoDeProceso(proceso, marcoAuxiliar->pagina);
	elemento->instantes = 0;
	if(elemento->modificado == 1){
		printf("Le mando a Swap la pagina %d del proceso %d con el texto %s\n", marcoAuxiliar->pagina-pagInicial->pagina, proceso->pid, marcoAuxiliar->contenidoPagina);
		int send;
		proceso->accesosSwap++;
		t_paquete_envio* paqueteEnvioSwap = malloc(sizeof(t_paquete_envio));
		t_paquete_para_memoria* paqueteEscrituraSwap = malloc(sizeof(t_paquete_para_memoria));
		paqueteEscrituraSwap->instruccion = 2;
		paqueteEscrituraSwap->PID = proceso->pid;
		paqueteEscrituraSwap->pagina = marcoAuxiliar->pagina - pagInicial->pagina;
		paqueteEscrituraSwap->texto = string_duplicate(marcoAuxiliar->contenidoPagina);
		paqueteEnvioSwap = serializarPorEscritura(paqueteEscrituraSwap);
		send = SendAll(socketSwap, paqueteEnvioSwap);
		free(paqueteEnvioSwap);
		free(paqueteEscrituraSwap);
		elemento->modificado = 0;
	}
}

void reemplazarMarcoPorMaximosTLB(contenidoMarco* marco, t_list* tlb, contenidoMarco* marcoAReemplazar, MemoriaConfig* config, mProcMemoria* proceso, t_log* logs){
	int i;
	buscarYRemoverMarco(tlb, marcoAReemplazar);
	list_add(tlb, marco);
	pthread_mutex_lock(&mutex_logs);
	log_info(logs, "Página %d del marco %d, reemplazada por página %d del marco %d", marcoAReemplazar->pagina, marcoAReemplazar->marco, marco->pagina, marco->marco);
	pthread_mutex_unlock(&mutex_logs);
	for(i=0; i<list_size(proceso->tablaDePaginas); i++){
		estadoPagina* elemento = (estadoPagina*) list_get(proceso->tablaDePaginas,i);
		if(elemento->pagina == marcoAReemplazar->pagina){
			elemento->presencia = 0;
		}
		if(elemento->pagina == marco->pagina){
			elemento->presencia = 1;
		}
	}
}

contenidoMarco* aplicarAlgoritmo(MemoriaConfig* config, t_list* lista, int tamanioMaximo, int* puntero, mProcMemoria* proceso){
	contenidoMarco* marcoAReemplazar;
	if(!(strcmp(config->ALGORITMO_REEMPLAZO, "FIFO"))){
		marcoAReemplazar = (contenidoMarco*) list_get(lista, *puntero);
	} else if(!(strcmp(config->ALGORITMO_REEMPLAZO, "LRU"))){
		int i;
		estadoPagina* masViejo = (estadoPagina*)malloc(sizeof(estadoPagina));
		masViejo->instantes = 0;
		for(i=0; i<list_size(proceso->tablaDePaginas); i++){
			estadoPagina* elemento = (estadoPagina*)list_get(proceso->tablaDePaginas, i);
			if(elemento->instantes > masViejo->instantes){
				masViejo->instantes = elemento->instantes;
				masViejo->marco = elemento->marco;
				masViejo->modificado = elemento->modificado;
				masViejo->pagina = elemento->pagina;
				masViejo->presencia = elemento->presencia;
			}
		}
		marcoAReemplazar = buscarMarco(lista, masViejo->marco);
		free(masViejo);
	} else if(!(strcmp(config->ALGORITMO_REEMPLAZO, "CLOCK"))){
		int i = 1;
		while(i){
			contenidoMarco* marcoAux = (contenidoMarco*) list_get(lista, *puntero);
			if(marcoAux->usada == 0){
				marcoAReemplazar = (contenidoMarco*)list_get(lista, *puntero);
				i = 0;
			} else if(marcoAux->usada == 1){
				marcoAux->usada = 0;
			}
			*puntero = *puntero + 1;
			if(*puntero == tamanioMaximo){
				*puntero = 0;
			}
		}
	} else if(!(strcmp(config->ALGORITMO_REEMPLAZO, "CLOCK-M"))){
		int i = 1;
		while(i){
			int a = 1;
			int punteroInicial = *puntero;
			int c = 1;
			while(a){
				contenidoMarco* marcoAux = (contenidoMarco*) list_get(lista, *puntero);
				if(marcoAux->usada == 0 && marcoAux->modificada == 0){
					marcoAReemplazar = (contenidoMarco*)list_get(lista, *puntero);
					a = 0;
					i = 0;
					c = 0;
				}
				*puntero = *puntero + 1;
				if(*puntero == tamanioMaximo){
					*puntero = 0;
				}
				if(*puntero == punteroInicial) a=0;
			}
			while(c){
				contenidoMarco* marcoAux = (contenidoMarco*) list_get(lista, *puntero);
				if(marcoAux->usada == 0 && marcoAux->modificada == 1){
					marcoAReemplazar = (contenidoMarco*)list_get(lista, *puntero);
					i = 0;
					c = 0;
				} else if(marcoAux->usada == 1){
					marcoAux->usada = 0;
				}
				*puntero = *puntero + 1;
				if(*puntero == tamanioMaximo){
					*puntero = 0;
				}
				if(*puntero == punteroInicial) c = 0;
			}
		}
	}
	return marcoAReemplazar;
}

void aumentarInstantes(mProcMemoria* procesoBuscado, int marco, int maximo){
	int j;
	for(j=0;j<list_size(procesoBuscado->tablaDePaginas); j++){
		estadoPagina* elemento = (estadoPagina*)list_get(procesoBuscado->tablaDePaginas, j);
		if(elemento->marco != maximo && elemento->marco != marco){
			elemento->instantes++;
		} else if(elemento->marco == marco){
			elemento->instantes = 0;
		}
	}
}

contenidoMarco* copiarMarco(contenidoMarco* marco){
	contenidoMarco* marcoNuevo = (contenidoMarco*)malloc(sizeof(contenidoMarco));
	marcoNuevo->marco = marco->marco;
	marcoNuevo->contenidoPagina = string_duplicate(marco->contenidoPagina);
	marcoNuevo->modificada = marco->modificada;
	marcoNuevo->pagina = marco->pagina;
	marcoNuevo->tamanio = marco->tamanio;
	marcoNuevo->usada = marco->usada;
	marcoNuevo->vacio = marco->vacio;
	return marcoNuevo;
}

void copiarValoresSinMarco(contenidoMarco* marcoAReemplazar, contenidoMarco* marco){
	marcoAReemplazar->contenidoPagina = string_duplicate(marco->contenidoPagina);
	marcoAReemplazar->modificada = marco->modificada;
	marcoAReemplazar->pagina = marco->pagina;
	marcoAReemplazar->tamanio = marco->tamanio;
	marcoAReemplazar->usada = marco->usada;
	marcoAReemplazar->vacio = marco->vacio;
}

void asignarValorModificacion(mProcMemoria* proceso, int numPaginaBuscada, int valor){
	int i;
	for(i=0; i<list_size(proceso->tablaDePaginas); i++){
		estadoPagina* elemento = (estadoPagina*) list_get(proceso->tablaDePaginas, i);
		if(elemento->pagina == numPaginaBuscada){
			elemento->modificado = valor;
		}
	}
}

estadoPagina* obtenerMarcoDeProceso(mProcMemoria* proceso, int numPaginaBuscada){
	int i;
	estadoPagina* elementoADevolver;
	for(i=0; i<list_size(proceso->tablaDePaginas); i++){
		estadoPagina* elemento = (estadoPagina*) list_get(proceso->tablaDePaginas, i);
		if(elemento->pagina == numPaginaBuscada){
			elementoADevolver = elemento;
		}
	}
	return elementoADevolver;
}

contenidoMarco* buscarElPrimeroEnLaLista(t_list* listaMarcosProceso, t_list* lista){
	contenidoMarco* marcoADevolver;
	int i = 0;
	int j;
	int a = 1;
	while(a){
		contenidoMarco* marcoAux = (contenidoMarco*) list_get(lista,i);
		for(j=0; j<list_size(listaMarcosProceso); j++){
			contenidoMarco* marcoAuxProceso = (contenidoMarco*) list_get(listaMarcosProceso,j);
			if(marcoAux->marco == marcoAuxProceso->marco){
				marcoADevolver = marcoAux;
				a = 0;
			}
		}
		i++;
	}
	return marcoADevolver;
}

void agregarElMarcoAlProceso(mProcMemoria* procesoBuscado, int numPaginaBuscada, int marco){
	int i;
	for(i=0; i<list_size(procesoBuscado->tablaDePaginas); i++){
		estadoPagina* elemento = list_get(procesoBuscado->tablaDePaginas, i);
		if(elemento->pagina == numPaginaBuscada){
			elemento->marco = marco;
		}
	}
}

void sacarMarcoAlProceso(mProcMemoria* proceso, int pagina, int maximo){
	int i;
	for(i=0; i<list_size(proceso->tablaDePaginas); i++){
		estadoPagina* elemento = list_get(proceso->tablaDePaginas, i);
		if(elemento->pagina == pagina){
			elemento->marco = maximo;
		}
	}
}

contenidoMarco* buscarMarco(t_list* lista, int marco){
	int i;
	contenidoMarco* marcoADevolver;
	for(i=0; i<list_size(lista); i++){
		contenidoMarco* marcoAux = list_get(lista, i);
		if(marcoAux->marco == marco){
			marcoADevolver = marcoAux;
		}
	}
	return marcoADevolver;
}

mProcMemoria* buscarProcesoPorPagina(t_list* lista, int numPaginaBuscada){
	mProcMemoria* procesoADevolver;
	int i = 0;
	int j = 0;
	for(i=0; i<list_size(lista); i++){
		mProcMemoria* proceso = (mProcMemoria*) list_get(lista, i);
		for(j=0; j<list_size(proceso->tablaDePaginas); j++){
			estadoPagina* elemento = (estadoPagina*) list_get(proceso->tablaDePaginas, j);
			if(elemento->pagina == numPaginaBuscada){
				procesoADevolver = proceso;
			}
		}
	}
	return procesoADevolver;
}

mProcMemoria* buscarProcesoPorPID(t_list* lista, t_paquete_para_memoria* paqueteInstruccion){
	mProcMemoria* procesoADevolver;
	int i = 0;
	for(i=0;i<list_size(lista);i++){
		mProcMemoria* proceso = (mProcMemoria*) list_get(lista, i);
		if(proceso->pid == paqueteInstruccion->PID){
			procesoADevolver = proceso;
		}
	}
	return procesoADevolver;
}

contenidoMarco* obtenerMarco(t_paquete_para_memoria* paqueteInstruccion, t_list* lista, t_paquete_resultado_instruccion* envioCPU, int numPaginaBuscada){
	contenidoMarco* marcoADevolver;
	int i = 0;
	for(i=0;i<list_size(lista);i++){
		contenidoMarco* marco = (contenidoMarco*) list_get(lista, i);
		if(marco->pagina == numPaginaBuscada){
			marcoADevolver = marco;
			envioCPU->contenido = string_duplicate(marco->contenidoPagina);
			envioCPU->error = 0;
		}
	}
	return marcoADevolver;
}

contenidoMarco* obtenerMarcoYEscribir(t_paquete_para_memoria* paqueteInstruccion, t_list* lista, int numPaginaBuscada, mProcMemoria* proceso){
	contenidoMarco* marcoADevolver;
	int i = 0;
	for(i=0;i<list_size(lista);i++){
		contenidoMarco* marco = (contenidoMarco*) list_get(lista, i);
		if(marco->pagina == numPaginaBuscada){
			marco->contenidoPagina = string_duplicate(paqueteInstruccion->texto);
			marco->modificada = 1;
			marcoADevolver = marco;
			asignarValorModificacion(proceso, numPaginaBuscada, 1);
		}
	}
	return marcoADevolver;
}

void buscarYRemoverMarco(t_list* lista, contenidoMarco* marcoASacar){
	int i = 0;
	contenidoMarco* marcoARemover;
	for(i=0; i < list_size(lista); i++){
		contenidoMarco* aux = list_get(lista, i);
		if(aux->marco == marcoASacar->marco){
			marcoARemover = list_remove(lista, i);
		}
	}
}

int estaEnLista(int numPaginaBuscada, t_list* lista){
	int i = 0;
	int aux = 0;
	for(i=0;i<list_size(lista);i++){
		contenidoMarco* marco = (contenidoMarco*) list_get(lista, i);
		if(marco->pagina == numPaginaBuscada){
			aux = 1;
		}
	}
	return aux;
}

void limpiarTLB(t_list* tlb, t_list* listaProcesos){
	int tamanio = list_size(tlb);
	int i, j, c;
	for(i=0; i<tamanio; i++){
		contenidoMarco* marco = (contenidoMarco*)list_remove(tlb, 0);
	}
	for(j=0; j<list_size(listaProcesos); j++){
		mProcMemoria* proceso = (mProcMemoria*)list_get(listaProcesos, j);
		for(c=0; c<list_size(proceso->tablaDePaginas);c++){
			estadoPagina* elemento = (estadoPagina*)list_get(proceso->tablaDePaginas, c);
			if(elemento->presencia == 1){
				elemento->presencia = 0;
			}
		}
	}
}

void limpiarMP(t_list* memoriaPrincipal, t_list* listaProcesos, t_list* tlb, int socketSwap, int maximo){
	int i, j, c;
	for(j=0; j<list_size(listaProcesos); j++){
		mProcMemoria* proceso = (mProcMemoria*)list_get(listaProcesos, j);
		estadoPagina* pagInicial = (estadoPagina*) list_get(proceso->tablaDePaginas,0);
		int tamanio = list_size(proceso->marcosAsignados);
		for(i=0; i<tamanio; i++){
			contenidoMarco* marco = (contenidoMarco*)list_remove(proceso->marcosAsignados, 0);
			for(c=0; c<list_size(proceso->tablaDePaginas);c++){
				estadoPagina* elemento = (estadoPagina*)list_get(proceso->tablaDePaginas, c);
				if(marco->marco == elemento->marco){
					if(elemento->modificado == 1){
						printf("Le mando a Swap la pagina %d con el texto %s\n", marco->pagina-pagInicial->pagina, marco->contenidoPagina);
						int send;
						proceso->accesosSwap++;
						t_paquete_envio* paqueteEnvioSwap = malloc(sizeof(t_paquete_envio));
						t_paquete_para_memoria* paqueteEscrituraSwap = malloc(sizeof(t_paquete_para_memoria));
						paqueteEscrituraSwap->instruccion = 2;
						paqueteEscrituraSwap->PID = proceso->pid;
						paqueteEscrituraSwap->pagina = marco->pagina - pagInicial->pagina;
						paqueteEscrituraSwap->texto = string_duplicate(marco->contenidoPagina);
						paqueteEnvioSwap = serializarPorEscritura(paqueteEscrituraSwap);
						send = SendAll(socketSwap, paqueteEnvioSwap);
						free(paqueteEnvioSwap);
						free(paqueteEscrituraSwap);
						elemento->modificado = 0;
						elemento->marco = maximo;
						sleep(1);
					}
				}
			}
		}
		proceso->puntero = 0;
	}
	for(i=0; i<list_size(memoriaPrincipal); i++){
		contenidoMarco* marcoMem = (contenidoMarco*)list_get(memoriaPrincipal, i);
		marcoMem->modificada = 0;
		marcoMem->pagina = -1;
		marcoMem->usada = 0;
		marcoMem->vacio = 1;
		marcoMem->contenidoPagina = "\0";
	}
	pthread_mutex_lock(&mutex_tlb);
	limpiarTLB(tlb, listaProcesos);
	pthread_mutex_unlock(&mutex_tlb);
}

void loguearMP(t_list* memoriaPrincipal, t_log* logs){
	int i;
	for(i=0; i<list_size(memoriaPrincipal); i++){
		contenidoMarco* marco = (contenidoMarco*)list_get(memoriaPrincipal, i);
		if(marco->pagina!=-1){
			if(strlen(marco->contenidoPagina)>0){
				log_info(logs, "El marco %d tiene la pagina %d, cuyo contenido es %s, U=%d, M=%d", marco->marco, marco->pagina, marco->contenidoPagina, marco->usada, marco->modificada);
			} else {
				log_info(logs, "El marco %d tiene la pagina %d, la cual esta vacia, U=%d, M=%d", marco->marco, marco->pagina, marco->usada, marco->modificada);
			}
		} else {
			log_info(logs, "El marco %d esta vacio", marco->marco);
		}
	}
	log_info(logs, "Tratamiento de señal terminado");
}

void crearFork(t_list* memoriaPrincipal, t_log* logs){
	/* Identificador del proceso creado */
		pid_t idProceso;

		/* Variable para comprobar que se copia inicialmente en cada proceso y que
		 * luego puede cambiarse independientemente en cada uno de ellos. */
		int variable = 1;

		/* Estado devuelto por el hijo */
		int estadoHijo;

		/* Se crea el proceso hijo. En algún sitio dentro del fork(), nuestro
		 * programa se duplica en dos procesos. Cada proceso obtendrá una salida
		 * distinta. */
		idProceso = fork();

		/* Si fork() devuelve -1, es que hay un error y no se ha podido crear el
		 * proceso hijo. */
		if (idProceso == -1)
		{
			perror ("No se puede crear proceso");
			exit (-1);
		}

		/* fork() devuelve 0 al proceso hijo.*/
		if (idProceso == 0)
		{
			/* El hijo escribe su pid en pantalla y el valor de variable */
//			printf ("Hijo  : Mi pid es %d. El pid de mi padre es %d\n",
//				getpid(), getppid());
			pthread_mutex_lock(&mutex_logs);
			loguearMP(memoriaPrincipal, logs);
			pthread_mutex_unlock(&mutex_logs);
		}

		/* fork() devuelve un número positivo al padre. Este número es el id del
		 * hijo. */
		if (idProceso > 0)
		{
			/* Espera un segundo (para dar tiempo al hijo a hacer sus cosas y no
			 * entremezclar salida en la pantalla) y escribe su pid y el de su hijo */
			sleep (1);
//			printf ("Padre : Mi pid es %d. El pid de mi hijo es %d\n",
//				getpid(), idProceso);

			/* Espera que el hijo muera */
			wait (&estadoHijo);

			/* Comprueba la salida del hijo */
			if (WIFEXITED(estadoHijo) != 0)
			{
//				printf ("Padre : Mi hijo ha salido. Devuelve %d\n",	WEXITSTATUS(estadoHijo));
			}


		}
}
void* temporizador(void* data){

	 struct itimerval it_val;  /* for setting itimer */

	  /* Upon SIGALRM, call DoStuff().
	   * Set interval timer.  We want frequency in ms,
	   * but the setitimer call needs seconds and useconds. */
	  if (signal(SIGALRM, (void (*)(int)) timer_handler) == SIG_ERR) {
	    perror("Unable to catch SIGALRM");
	    exit(1);
	  }
	  it_val.it_value.tv_sec = 60;
	  it_val.it_value.tv_usec = 0;
	  it_val.it_interval = it_val.it_value;
	  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
	    perror("error calling setitimer()");
	    exit(1);
	  }

	  while (1)
	    pause();
}

void timer_handler()
{

	if(tlbStatus->ingresosTLB!=0){
		double aciertos = ((((double)tlbStatus->aciertos)/((double)tlbStatus->ingresosTLB)) *100);
		printf("La tasa de aciertos de la TLB fue de: %lf %c\n", aciertos, '%');
	}
}

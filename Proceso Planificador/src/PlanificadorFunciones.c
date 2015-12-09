/*
 * PlanificadorFunciones.c
 *
 *  Created on: 7/9/2015
 *      Author: utnso
 */

#include "PlanificadorArranque.h"
#include "PlanificadorFunciones.h"

void* consola(void* data) {
	t_conexiones_solicitudes_recursos* dataHilos = (t_conexiones_solicitudes_recursos*) data;
	char eleccion[265]; //256 de PATH Máximo + Comando + '\0'
	char* comando;
	char* parametro;
	int inicioParametro = 0;
	int consola = 1;

	while (consola) {
		inicioParametro = 0;
		comando = string_new();
		puts(
				"____________________________________________________________________\n\n"
						"Comandos disponibles: \n"
						"- 1) correr PATH \n"
						"- 2) finalizar PID \n"
						"- 3) ps / ps++ \n"
						"- 4) cpu \n"
						"******BONUS******\n"
						"- X) cs \n"
						"*****************\n"
						">>Ingrese un comando: "
			);
		gets(eleccion);
//		printf("Lei: %s\n", eleccion);

		int i = 0;
		while(eleccion[i] != '\0'){
			if(eleccion[i] == ' ') {
				inicioParametro = i+1;
				i = strlen(eleccion)-1;
			}
			i++;
		}
		//printf("El parametro empieza en la posicion %d\n", posInicioParametro);

		if(inicioParametro != 0){
			parametro = string_new();
			comando = (char*) malloc( sizeof(char) * strlen(string_substring(eleccion, 0, inicioParametro-1)));
			comando = string_substring(eleccion, 0, inicioParametro-1);
			parametro = (char*) malloc( sizeof(char) * strlen(string_substring_from(eleccion, inicioParametro)));
			parametro = string_substring_from(eleccion, inicioParametro);
		}
		else{
			comando = (char*) malloc( sizeof(char) * strlen(eleccion) );
			comando = string_duplicate(eleccion);
		}

		if(inicioParametro == 0){
			if(strcmp(eleccion, "ps") == 0){
				printf("Comando a ejecutar: %s\n", eleccion);
				process_status(dataHilos->procesos, 0);
			}
			else{
				if(strcmp(eleccion, "cpu") == 0){
					printf("Comando a ejecutar: %s\n", eleccion);
					if(dataHilos->cpuData){
						cpu_status(dataHilos->cpuData);
					}
				}
				else{
					if(strcmp(eleccion, "ps++") == 0){
						printf("Comando a ejecutar: %s\n", eleccion);
						process_status_historico(dataHilos->procesos, dataHilos->finalizados);
					}
					else{
						if(strcmp(eleccion, "cs") == 0){
							printf("Comando a ejecutar: %s\n", eleccion);
							core_status(dataHilos->cpuConectadas, dataHilos->procesos);
						}
						else{
							printf("El comando (%s) ingresado no es valido \n", eleccion);
						}
					}
				}
			}
		}

		if(inicioParametro != 0){
			if(strcmp(comando, "correr") == 0){
				int j = 0;
				int longPath = 0;
				char* unPath = string_new();
				while(j<=strlen(parametro)){
					if(parametro[j] == ' '){
						longPath--; //Si luego de un mCod el siguiente empieza con espacio, no quiero considerarlo en su Path
					}
					if(parametro[j] == ',' || parametro[j] == ';' || parametro[j] == '\0') { // Indicadores de separación de Paths
						unPath = (char*) malloc( sizeof(char) * strlen(string_substring(parametro, j-longPath, longPath)));
						unPath = string_substring(parametro, j-longPath, longPath);
						longPath=-1;
						char* pathAbsoluto = realpath(unPath, NULL);
						if(pathAbsoluto){
							printf("Comando a ejecutar: %s %s\n", comando, pathAbsoluto);
							correr_PATH(dataHilos->procesos, dataHilos->procesosListos, &dataHilos->ultimoPidAsignado, pathAbsoluto, dataHilos->logs);
						}
						else{
							printf("Error: La ruta o el archivo (%s) especificados no son validos\n", unPath);
						}
						free(unPath);
						free(pathAbsoluto);
					}
					j++;
					longPath++;
				}
			}
			else{
				if(strcmp(comando, "finalizar") == 0){
					printf("Comando a ejecutar: %s\n", eleccion);
					int pidAux = atoi(parametro);
					finalizar_PID(dataHilos->procesos, pidAux);
				}
				else{
					printf("El comando (%s) ingresado no es valido \n", eleccion);
				}
			}
		}

		free(comando);
		if(inicioParametro != 0){
			free(parametro);
		}
	}
}

void* solicitarEjecuciones(void* data){
	/*----ASIGNACION DE VARIABLES PARA MEJOR MANEJO----*/
	t_conexiones_solicitudes_recursos* dataHilos = (t_conexiones_solicitudes_recursos*) data;
	t_list* cpuConectadas = dataHilos->cpuConectadas;
	t_queue* procesosListos = dataHilos->procesosListos;
	t_list* procesos = dataHilos->procesos;
	t_queue* entradaSalida = dataHilos->entradaSalida;
	t_planificadorConfig* config = dataHilos->config;
	t_log* logs = dataHilos->logs;
	/*-------------------------------------------------*/
	t_cpu* cpuSeleccionada;
	t_pcb* procesoSeleccionado;
	int pidSeleccionado;
	int pidLogueo;
	t_entradaSalida* logueoES;
	t_paquete_ejecutar* pedidoEjecucion;
	t_paquete_envio* paqueteEnvio;
	int i = 0;
	int cpuDisponible = 0;
	t_periodo* comienzoDePeriodoEjecucion;
	t_periodo* finDePeriodoEspera;

	while(1){
		pthread_mutex_lock(&mutex_envios);
		cpuDisponible = 0;
		for (i=0; i<list_size(cpuConectadas); i++){
			cpuSeleccionada = (t_cpu*) list_get(cpuConectadas, i);
			if(cpuSeleccionada->estado == 1) {
				i = list_size(cpuConectadas);
				cpuDisponible = 1;
			}
		}

		if(cpuDisponible){
			pthread_mutex_lock(&mutex_colaReady);
			if(!queue_is_empty(procesosListos)){
				pidSeleccionado = (int) queue_pop(procesosListos);

				cpuSeleccionada->estado = 0;
				procesoSeleccionado = get_proceso(procesos, pidSeleccionado);

				pedidoEjecucion = (t_paquete_ejecutar*) malloc(sizeof(t_paquete_ejecutar));
				pedidoEjecucion->path = string_duplicate(procesoSeleccionado->path);
				pedidoEjecucion->pid = procesoSeleccionado->pid;
				pedidoEjecucion->proximaInstruccion = procesoSeleccionado->proximaInstruccion;
				if(strcmp(config->ALGORITMO_PLANIF, "FIFO") == 0) pedidoEjecucion->rafaga = 0;
				if(strcmp(config->ALGORITMO_PLANIF, "RR") == 0) pedidoEjecucion->rafaga = config->QUANTUM;
				procesoSeleccionado->estado = EJECUTANDO;

				finDePeriodoEspera = (t_periodo*) list_get(procesoSeleccionado->tiempoEspera, list_size(procesoSeleccionado->tiempoEspera)-1);
				finDePeriodoEspera->tiempoFin = temporal_get_string_time();

				comienzoDePeriodoEjecucion = (t_periodo*) malloc(sizeof(t_periodo));
				comienzoDePeriodoEjecucion->tiempoInicio = temporal_get_string_time();
				list_add(procesoSeleccionado->tiempoEjecucion, comienzoDePeriodoEjecucion);

				paqueteEnvio = serializar_t_paquete_ejecutar(pedidoEjecucion);
				int solicitudEnviada = SendAll(cpuSeleccionada->socketCPU, paqueteEnvio);
				printf("Solicitud de ejecucion enviada a la CPU (%d) \n", cpuSeleccionada->core);

				pthread_mutex_unlock(&mutex_colaReady);

				if(solicitudEnviada){
					cpuSeleccionada->ultimoPidEjecutado = pidSeleccionado;

		    		pthread_mutex_lock(&mutex_logs);

		    		if(procesoSeleccionado->proximaInstruccion == 0)
		    			log_info(logs, "COMIENZO EJECUCION: mProc (%s) de PID (%d)", procesoSeleccionado->nombre, procesoSeleccionado->pid);

		    		log_info(logs, "Ejecucion del mProc (%s) de PID (%d) segun algoritmo (%s)", procesoSeleccionado->nombre, procesoSeleccionado->pid, config->ALGORITMO_PLANIF);

		    		log_info(logs, "Estado Cola Ready");
		    		pthread_mutex_lock(&mutex_colaReady);
		    		for(i=0; i<queue_size(procesosListos); i++){
		    			pidLogueo = (int) queue_pop(procesosListos);
		    			log_info(logs, "Posicion %d Cola Ready: PID (%d)", i+1 , pidLogueo);
		    			queue_push(procesosListos, (void*) pidLogueo);
		    		}
		    		pthread_mutex_unlock(&mutex_colaReady);

		    		log_info(logs, "Estado Cola E/S");
		    		pthread_mutex_lock(&mutex_entradaSalida);
		    		for(i=0; i<queue_size(entradaSalida); i++){
		    			logueoES = (t_entradaSalida*) queue_pop(entradaSalida);
		    			pidLogueo = logueoES->pid;
		    			log_info(logs, "Posicion %d Cola Entrada/Salida: PID (%d)", i+1, pidLogueo);
		    			queue_push(entradaSalida, (void*) logueoES);
		    		}
		    		pthread_mutex_unlock(&mutex_entradaSalida);

		    		pthread_mutex_unlock(&mutex_logs);
				}

				free(pedidoEjecucion->path);
				free(pedidoEjecucion);
				//sleep(1);
			}
			else{
				pthread_mutex_unlock(&mutex_colaReady);
			}
		}
		pthread_mutex_unlock(&mutex_envios);
	}
}

void* handlerSelect(void* data) {
	/*----ASIGNACION DE VARIABLES PARA MEJOR MANEJO----*/
	t_conexiones_solicitudes_recursos* dataHilos = (t_conexiones_solicitudes_recursos*) data;
	t_list* cpuConectadas = dataHilos->cpuConectadas;
	t_queue* procesosListos = dataHilos->procesosListos;
	t_list* procesos = dataHilos->procesos;
	t_queue* entradaSalida = dataHilos->entradaSalida;
	t_planificadorConfig* config = dataHilos->config;
	t_log* logs = dataHilos->logs;
	t_list* finalizados = dataHilos->finalizados;
	int* cpuData = &dataHilos->cpuData;
	/*-------------------------------------------------*/
	struct sockaddr_in clientname;
	size_t size;
	int listenningSocket = Escuchar(config->PUERTO_ESCUCHA);
	int unSocket;
	fd_set active_fd_set;
	fd_set read_fd_set;
    int estadoDescriptores;
	t_periodo* finDePeriodoEjecucion;
	t_periodo* comienzoDePeriodoEspera;
	int esConexionDeUnCore = 0;
	int bytesRecibidos = 0;
	int i = 0;

	pthread_mutex_init(&mutex_cpus, NULL );
	/* Inicializacion del set de sockets activos */
	FD_ZERO (&active_fd_set);
 	FD_SET (listenningSocket, &active_fd_set);

	while (1){
		pthread_mutex_lock(&mutex_recepciones);
		/* Se bloquea hasta que haya algo nuevo que leer en uno o más de los sockets activos: mis sockets son solo de lectura */
		FD_ZERO (&read_fd_set);
		read_fd_set = active_fd_set;
	    estadoDescriptores = select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);
	    if (estadoDescriptores == -1){
	    	perror ("select");
	    	exit (EXIT_FAILURE);
	    }
	    else{
	    	if(estadoDescriptores > 0){
	    	    /* Atiendo a todos los sockets que tienen algo nuevo que debo leer. */
	    	    for (unSocket = 0; unSocket < FD_SETSIZE; ++unSocket){
	    	    	if (FD_ISSET (unSocket, &read_fd_set)){
	    	    		if (unSocket == listenningSocket){
	    	    			/* Solicitudes de conexión sobre el socket escucha servidor. */
	    	                size = sizeof (clientname);
	    	                int nuevoCliente = accept (listenningSocket, (struct sockaddr*) &clientname, &size);
	    	                if (nuevoCliente < 0){
	    	                	puts("Error al aceptar un nuevo cliente");
	    	                    perror ("accept");
	    	                    exit (EXIT_FAILURE);
	    	                }

	    	                fprintf(stderr, "Server: connect from host %s, port %d.\n", inet_ntoa (clientname.sin_addr), ntohs (clientname.sin_port));
	    	                FD_SET (nuevoCliente, &active_fd_set);
	    	                if(!esConexionDeUnCore){
	    	                	printf("Conexion con CPU establecida\n");
	    	                	*cpuData = nuevoCliente;
	    	                	esConexionDeUnCore = 1;

	    	                	//sleep(1);
	    	                }
	    	                else{
		                		t_cpu* cpuAux = (t_cpu*) malloc(sizeof(t_cpu)); //Pido memoria para nueva cpu
		                		t_paquete_envio* paqueteBienvenida = serializar_string("BIENVENIDO AL PLANIFICADOR"); //ACA DEBERIA HACER UN LOG
		                		SendAll(nuevoCliente, paqueteBienvenida); //Mensaje de bienvenida

		                		t_paquete_envio* paqueteAux = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
		                		RecvAll2(nuevoCliente, paqueteAux); //Recibo el número de core de la cpu recién conectada

		                		cpuAux->core = atoi(deserializar_string(paqueteAux)); //Convierto y guardo el número de core
		                		printf("Conexion del core (%d)\n", cpuAux->core);
		                		cpuAux->socketCPU = nuevoCliente; //Guardo el socket
		                		cpuAux->estado = 1; //Indico que la nueva CPU está libre
		                		cpuAux->ultimoPidEjecutado = 0; //Indica que no ha ejecutado nunca nada
		                		pthread_mutex_lock(&mutex_cpus);
		                		list_add(cpuConectadas, (void*) cpuAux); // Agrego la estructura de cpu a la lista de cpu's conectadas
		                		pthread_mutex_unlock(&mutex_cpus);
		                		pthread_mutex_lock(&mutex_logs);
		                		log_warning(logs, "Se ha conectado la CPU (%d) desde IP (%s) y Puerto (%d)", cpuAux->core, inet_ntoa (clientname.sin_addr), ntohs (clientname.sin_port));
		                		pthread_mutex_unlock(&mutex_logs);

		                		free(paqueteBienvenida->data);
		                		free(paqueteBienvenida);
		                		free(paqueteAux->data);
		                		free(paqueteAux);

		                		//sleep(1);
	    	                }
	    	    		}
	    	            else{
	    	            	if(unSocket == *cpuData){
	    	            		t_list* estadosCpus;
	    	            		char* unEstado;
	    	            		bytesRecibidos = 0;
								t_paquete_envio* paqueteEstadosCpus = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
								bytesRecibidos = RecvAll2(unSocket, paqueteEstadosCpus); //Recibo la rta de ejecucion
								if (bytesRecibidos < 0){
									close(unSocket);
									FD_CLR (unSocket, &active_fd_set);
									pthread_mutex_lock(&mutex_logs);
									log_warning(logs, "Error de recepcion: no se pudo recibir el estado de las CPUs");
									pthread_mutex_unlock(&mutex_logs);
								}
								if (bytesRecibidos == 0){
									FD_CLR (unSocket, &active_fd_set);
									pthread_mutex_lock(&mutex_logs);
									log_warning(logs, "FD desconectado: no se pudo recibir el estado de las CPUs");
									pthread_mutex_unlock(&mutex_logs);
								}
								if(bytesRecibidos > 0){
									estadosCpus = deserializar_cpu_status(paqueteEstadosCpus);
									for(i=0; i<list_size(estadosCpus); i++){
										unEstado = (char*) list_get(estadosCpus, i);
										printf("(%s)\n", unEstado);
									}
								}
	    	            	}
	    	            	else{
	    	            		/* Nueva información para leer en sockets ya conectados */
								t_list_retorno* retornos;
								char* unRetorno;
								bytesRecibidos = 0;
								t_cpu* cpuUsada;
								int pidEjecutado;
								t_pcb* procesoEjecutado;
								t_pcb* procesoFinalizado;
								t_pcb* procesoFallido;
								int index = 0;
								t_paquete_envio* respuestaEjecucion = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
								bytesRecibidos = RecvAll2(unSocket, respuestaEjecucion); //Recibo la rta de ejecucion
								//BUSCO CUAL FUE LA CPU QUE ACABA DE EJECUTAR
								for(i=0; i<list_size(cpuConectadas); i++){
									cpuUsada = (t_cpu*) list_get(cpuConectadas, i);
									if(cpuUsada->socketCPU == unSocket) i = list_size(cpuConectadas);
								}
								if (bytesRecibidos < 0){
									close(unSocket);
									FD_CLR (unSocket, &active_fd_set);
									pthread_mutex_lock(&mutex_logs);
									log_warning(logs, "Error de recepcion. CPU (%d) desconectada", cpuUsada->core);
									pthread_mutex_unlock(&mutex_logs);
								}
								if (bytesRecibidos == 0){
									FD_CLR (unSocket, &active_fd_set);
									pthread_mutex_lock(&mutex_logs);
									log_warning(logs, "Se ha desconectado la CPU (%d)", cpuUsada->core);
									pthread_mutex_unlock(&mutex_logs);
								}
								if(bytesRecibidos > 0){
									cpuUsada->estado = 1;
									retornos = deserializar_t_list_retorno(respuestaEjecucion); //Convierto y guardo la estructura con los logs

									//BUSCO CUAL FUE EL PROCESO QUE ACABA DE EJECUTAR
									pidEjecutado = cpuUsada->ultimoPidEjecutado;
									for(i=0; i<list_size(procesos); i++){
										procesoEjecutado = (t_pcb*) list_get(procesos, i);
										if(procesoEjecutado->pid == pidEjecutado){
											index = i;
											i = list_size(procesos);
										}
									}
									if(!procesoEjecutado->flagAbortado) procesoEjecutado->proximaInstruccion = retornos->proximaInstruccion;

									finDePeriodoEjecucion = (t_periodo*) list_get(procesoEjecutado->tiempoEjecucion, list_size(procesoEjecutado->tiempoEjecucion)-1);
									finDePeriodoEjecucion->tiempoFin = temporal_get_string_time();

									comienzoDePeriodoEspera = (t_periodo*) malloc(sizeof(t_periodo));
									comienzoDePeriodoEspera->tiempoInicio = temporal_get_string_time();
									list_add(procesoEjecutado->tiempoEspera, comienzoDePeriodoEspera);


									//LOGUEAR LO QUE HIZO LA CPU
									pthread_mutex_lock(&mutex_logs);
									log_info(logs, "Rafaga de CPU (%d) completada. Se ejecuto mProc (%s) de PID (%d): ", cpuUsada->core, procesoEjecutado->nombre, procesoEjecutado->pid);
									for(i=0; i<list_size(retornos->retornos); i++){
										unRetorno = (char*) list_get(retornos->retornos, i);
										printf("Log: (%s)\n", unRetorno);
										log_info(logs, unRetorno);
									}
									pthread_mutex_unlock(&mutex_logs);

									//ORGANIZAR COLA DE READY Y COLA DE E/S

									if(strstr(unRetorno, "entrada-salida") && !procesoEjecutado->flagAbortado){
										//ADMINISTRAR E/S
										printf("Bloqueando mProc: (%d) por E/S\n", procesoEjecutado->pid);
										t_entradaSalida* solicitudEntradaSalida = (t_entradaSalida*) malloc(sizeof(t_entradaSalida));
										solicitudEntradaSalida->pid = procesoEjecutado->pid;
										int offsetAux = 0;
										int posicionTiempoEntradaSalida;
										while(unRetorno[offsetAux]){
											if(unRetorno[offsetAux] == ' ') posicionTiempoEntradaSalida = offsetAux;
											offsetAux++;
										}
										posicionTiempoEntradaSalida++;
										solicitudEntradaSalida->segundos = atoi(string_substring_from(unRetorno, posicionTiempoEntradaSalida));

										pthread_mutex_lock(&mutex_entradaSalida);
										procesoEjecutado->estado = BLOQUEADO;
										queue_push(entradaSalida, (void*) solicitudEntradaSalida);
										pthread_mutex_unlock(&mutex_entradaSalida);
									}
									else{
										if(strstr(unRetorno, "finalizado")){
											//ADMINISTRAR FINALIZADOS
											pthread_mutex_lock(&mutex_procesos);
											procesoFinalizado = (t_pcb*) list_remove(procesos, index);
											pthread_mutex_unlock(&mutex_procesos);

											procesoFinalizado->proximaInstruccion = -1;
											if(procesoFinalizado->tiempoRespuesta->tiempoFin == NULL){
												procesoFinalizado->tiempoRespuesta->tiempoFin = temporal_get_string_time();
											}
											list_remove(procesoFinalizado->tiempoEspera, list_size(procesoFinalizado->tiempoEspera)-1);

											pthread_mutex_lock(&mutex_finalizados);
											procesoFinalizado->estado = FINALIZADO;
											list_add(finalizados, (void*) procesoFinalizado);
											pthread_mutex_unlock(&mutex_finalizados);

											pthread_mutex_lock(&mutex_logs);
											log_info(logs, "FIN EJECUCION: mProc (%s) de PID (%d)", procesoFinalizado->nombre, procesoFinalizado->pid);
											loguear_metricas(logs, procesoFinalizado);
											pthread_mutex_unlock(&mutex_logs);

											int periodos=0;
											t_periodo* periodoAux;
											for(periodos=0; periodos<list_size(procesoFinalizado->tiempoEspera); periodos++){
													periodoAux = (t_periodo*) list_remove(procesoFinalizado->tiempoEspera, periodos);
													free(periodoAux->tiempoInicio);
													free(periodoAux->tiempoFin);
													free(periodoAux);
											}
											list_destroy(procesoFinalizado->tiempoEspera);
											for(periodos=0; periodos<list_size(procesoFinalizado->tiempoEjecucion); periodos++){
													periodoAux = (t_periodo*) list_remove(procesoFinalizado->tiempoEjecucion, periodos);
													free(periodoAux->tiempoInicio);
													free(periodoAux->tiempoFin);
													free(periodoAux);
											}
											list_destroy(procesoFinalizado->tiempoEjecucion);
											free(procesoFinalizado->tiempoRespuesta->tiempoInicio);
											free(procesoFinalizado->tiempoRespuesta->tiempoFin);
											free(procesoFinalizado->tiempoRespuesta);
											free(procesoFinalizado->path);
										}
										else{
											if(strstr(unRetorno, "Fallo")){
												//ADMINISTRAR FALLIDOS
												pthread_mutex_lock(&mutex_procesos);
												procesoFallido = (t_pcb*) list_remove(procesos, index);
												pthread_mutex_unlock(&mutex_procesos);

												pthread_mutex_lock(&mutex_logs);
												log_info(logs, "FALLO INICIO: mProc (%s) de PID (%d)", procesoFallido->nombre, procesoFallido->pid);
												pthread_mutex_unlock(&mutex_logs);

												int periodos=0;
												t_periodo* periodoAux;
												for(periodos=0; periodos<list_size(procesoFallido->tiempoEspera); periodos++){
														periodoAux = (t_periodo*) list_remove(procesoFallido->tiempoEspera, periodos);
														free(periodoAux->tiempoInicio);
														free(periodoAux->tiempoFin);
														free(periodoAux);
												}
												list_destroy(procesoFallido->tiempoEspera);
												for(periodos=0; periodos<list_size(procesoFallido->tiempoEjecucion); periodos++){
														periodoAux = (t_periodo*) list_remove(procesoFallido->tiempoEjecucion, periodos);
														free(periodoAux->tiempoInicio);
														free(periodoAux->tiempoFin);
														free(periodoAux);
												}
												list_destroy(procesoFallido->tiempoEjecucion);
												free(procesoFallido->tiempoRespuesta->tiempoInicio);
												free(procesoFallido->tiempoRespuesta->tiempoFin);
												free(procesoFallido->tiempoRespuesta);
												free(procesoFallido->nombre);
												free(procesoFallido->path);
												free(procesoFallido);
											}
											else{
												if(strstr(unRetorno, "entrada-salida") && procesoEjecutado->flagAbortado){
													printf("No dejo que se encole una E/S por pedido de finalizacion\n");
													pthread_mutex_lock(&mutex_logs);
													log_info(logs, "E/S abortada: se ordeno finalizar mProc (%s) de PID (%d). Vuelve a Cola Ready", procesoEjecutado->nombre, procesoEjecutado->pid);
													pthread_mutex_unlock(&mutex_logs);
												}
												//ADMINISTRAR COLA DE READY
												pthread_mutex_lock(&mutex_colaReady);
												procesoEjecutado->estado = LISTO;
												queue_push(procesosListos, (void*) pidEjecutado); // VUELVO A ENCOLAR EL PROCESO
												pthread_mutex_unlock(&mutex_colaReady);
											}
										}
									}

									free(respuestaEjecucion->data);
									free(respuestaEjecucion);
								}
								//sleep(1);
	    	            	}
	    	            }
	    	    	}
	    	    	else{
	    	        	 // printf("%d no seteado", i);
	    			}
	    	    }
	    	}
	    	else{
	    		//printf("No data within five seconds\n");
	    	}
	    }
	    pthread_mutex_unlock(&mutex_recepciones);
	 }
}

void* dma(void* data){
	/*----ASIGNACION DE VARIABLES PARA MEJOR MANEJO----*/
	t_conexiones_solicitudes_recursos* dataHilos = (t_conexiones_solicitudes_recursos*) data;
	t_queue* procesosListos = dataHilos->procesosListos;
	t_list* procesos = dataHilos->procesos;
	t_queue* entradaSalida = dataHilos->entradaSalida;
	t_log* logs = dataHilos->logs;
	/*-------------------------------------------------*/
	unsigned int tiempoEntradaSalida = 0;
	int pidEntradaSalida = 0;
	t_pcb* procesoEntradaSalida;
	t_periodo* comienzoDePeriodoEspera;
	t_periodo* finDePeriodoEspera;
	int retornoES;
	while(1){
		if(!queue_is_empty(entradaSalida)){
			pthread_mutex_lock(&mutex_entradaSalida);
			t_entradaSalida* solicitudEntradaSalida = (t_entradaSalida*) queue_pop(entradaSalida);
			pthread_mutex_unlock(&mutex_entradaSalida);

			tiempoEntradaSalida = solicitudEntradaSalida->segundos;
			pidEntradaSalida = solicitudEntradaSalida->pid;

			procesoEntradaSalida = get_proceso(procesos, pidEntradaSalida);

			finDePeriodoEspera = (t_periodo*) list_get(procesoEntradaSalida->tiempoEspera, list_size(procesoEntradaSalida->tiempoEspera)-1);
			finDePeriodoEspera->tiempoFin = temporal_get_string_time();

			if(!procesoEntradaSalida->flagAbortado){
				printf("Gestionando E/S de mProc: (%d) de tiempo (%d) \n", pidEntradaSalida, tiempoEntradaSalida);
				retornoES = usleep(tiempoEntradaSalida*1000000); //SIMULACIÓN DE LA E/S
				printf("E/S de mProc (%d) FINALIZADA\n", pidEntradaSalida);

				if(retornoES == 0){
					procesoEntradaSalida->estado = LISTO;
					if(!procesoEntradaSalida->flagAbortado) procesoEntradaSalida->proximaInstruccion++;
					if(procesoEntradaSalida->tiempoRespuesta->tiempoFin == NULL){
						procesoEntradaSalida->tiempoRespuesta->tiempoFin = temporal_get_string_time();
					}

					comienzoDePeriodoEspera = (t_periodo*) malloc(sizeof(t_periodo));
					comienzoDePeriodoEspera->tiempoInicio = temporal_get_string_time();
					list_add(procesoEntradaSalida->tiempoEspera, comienzoDePeriodoEspera);

					pthread_mutex_lock(&mutex_colaReady);
					queue_push(procesosListos, (void*) pidEntradaSalida);
					pthread_mutex_unlock(&mutex_colaReady);

					pthread_mutex_lock(&mutex_logs);
					log_info(logs, "E/S completada: mProc (%s) de PID (%d). Vuelve a Cola Ready", procesoEntradaSalida->nombre, procesoEntradaSalida->pid);
					pthread_mutex_unlock(&mutex_logs);
				}
			}
			else{
				printf("Aborte una E/S encolada por finalizacion\n");

				procesoEntradaSalida->estado = LISTO;
				if(procesoEntradaSalida->tiempoRespuesta->tiempoFin == NULL){
					procesoEntradaSalida->tiempoRespuesta->tiempoFin = temporal_get_string_time();
				}

				comienzoDePeriodoEspera = (t_periodo*) malloc(sizeof(t_periodo));
				comienzoDePeriodoEspera->tiempoInicio = temporal_get_string_time();
				list_add(procesoEntradaSalida->tiempoEspera, comienzoDePeriodoEspera);

				pthread_mutex_lock(&mutex_colaReady);
				queue_push(procesosListos, (void*) pidEntradaSalida);
				pthread_mutex_unlock(&mutex_colaReady);

				pthread_mutex_lock(&mutex_logs);
				log_info(logs, "E/S abortada: se ordeno finalizar mProc (%s) de PID (%d). Vuelve a Cola Ready", procesoEntradaSalida->nombre, procesoEntradaSalida->pid);
				pthread_mutex_unlock(&mutex_logs);
			}
		}
	}
}

void cpu_status(int cpuData){
	t_paquete_envio* pedidoStatusCpu = serializar_string("Status CPUs");
	SendAll(cpuData, pedidoStatusCpu); //Pedido Status
	free(pedidoStatusCpu->data);
	free(pedidoStatusCpu);
	return;
}

void process_status_historico(t_list* procesos, t_list* finalizados){
	printf("Process Status++:\n");
	if(!list_is_empty(finalizados)){
		list_iterate(finalizados, ps);
		process_status(procesos, 1);
	}
	else{
		printf("Aun no hay procesos finalizados\n");
	}
	return;
}

void process_status(t_list* procesos, int esHistorico){
	if(!esHistorico) printf("Process Status:\n");
	if(!list_is_empty(procesos))
		list_iterate(procesos, ps);
	else{
		if(!esHistorico)
			printf("No hay procesos en el planificador a corto plazo\n");
	}

	return;
}
void ps(void* proceso){
	t_pcb* procesoSeleccionado = (t_pcb*) proceso;
	char* estadoActual;
	if(procesoSeleccionado->estado == NUEVO){
		estadoActual = (char*) malloc(sizeof(char)*6);
		strcpy(estadoActual, "Nuevo");
		estadoActual[5] = '\0';
	}
	else{
		if(procesoSeleccionado->estado == LISTO){
			estadoActual = (char*) malloc(sizeof(char)*6);
			strcpy(estadoActual, "Listo");
			estadoActual[5] = '\0';
		}
		else{
			if(procesoSeleccionado->estado == EJECUTANDO){
				estadoActual = (char*) malloc(sizeof(char)*11);
				strcpy(estadoActual, "Ejecutando");
				estadoActual[10] = '\0';
			}
			else{
				if(procesoSeleccionado->estado == BLOQUEADO){
					estadoActual = (char*) malloc(sizeof(char)*10);
					strcpy(estadoActual, "Bloqueado");
					estadoActual[9] = '\0';
				}
				else{
					if(procesoSeleccionado->estado == FINALIZADO){
						estadoActual = (char*) malloc(sizeof(char)*11);
						strcpy(estadoActual, "Finalizado");
						estadoActual[10] = '\0';
					}
				}
			}
		}
	}
	printf("mProc %d: %s -> %s\n", procesoSeleccionado->pid, procesoSeleccionado->nombre, estadoActual);
	free(estadoActual);
}

//Issue 83: Avala la libre implementación de esta función
void finalizar_PID(t_list* procesos, int pid){
	t_pcb* procesoSeleccionado;
	procesoSeleccionado = get_proceso(procesos, pid);
	if(procesoSeleccionado){
		//	printf("La proxima instruccion del archivo (%s) a ejecutar es la numero (%d)\n", procesoSeleccionado->path, procesoSeleccionado->proximaInstruccion);
			pthread_mutex_unlock(&mutex_procesos);
			finalizarProceso(procesoSeleccionado);
			pthread_mutex_unlock(&mutex_procesos);
		//	printf("La proxima instruccion del archivo (%s) a ejecutar es la numero (%d)\n", procesoSeleccionado->path, procesoSeleccionado->proximaInstruccion);
	}
	else
		printf("El proceso de pid (%d) no existe o ya ha finalizado\n", pid);
	return;
}

void finalizarProceso(t_pcb* procesoSeleccionado){

	FILE* fp;
	int c;
	int instrucciones = 0;
	char* path = procesoSeleccionado->path;

	if((fp = fopen(path, "r")) == NULL){
		printf("No se pudo abrir el archivo: %s\n", path);
		return;
	}

    while (feof(fp) == 0){
    	c = fgetc(fp);
    	if(c == ';') instrucciones ++;
    }

	fclose(fp);

	procesoSeleccionado->flagAbortado = 1; //Indico que el proceso fue finalizado de manera forzada
	procesoSeleccionado->proximaInstruccion = instrucciones-1; //Arrancamos a contar desde 0

	return;
}

void correr_PATH(t_list* procesos, t_queue* procesosListos, int* pidAnterior, char* path, t_log* logs){
	t_pcb* procesoNuevo = (t_pcb*) malloc(sizeof(t_pcb));
	t_periodo* comienzoDePeriodoEspera;
	procesoNuevo->estado = NUEVO;
	procesoNuevo->path = string_duplicate(path);

	int i = 0;
	int inicioNombre = 0;
	while(path[i] != '\0'){
		if(path[i] == '/') inicioNombre = i+1;
		i++;
	}
	if(inicioNombre != 0){
		procesoNuevo->nombre = (char*) malloc( sizeof(char) * strlen(string_substring_from(path, inicioNombre)));
		procesoNuevo->nombre = string_substring_from(path, inicioNombre);
	}
	else{
		procesoNuevo->nombre = (char*) malloc( sizeof(char) * strlen(path) );
		procesoNuevo->nombre = string_duplicate(path);
	}
	*pidAnterior = *pidAnterior + 1;
	procesoNuevo->pid = *pidAnterior;
	procesoNuevo->proximaInstruccion = 0;
	procesoNuevo->flagAbortado = 0;
	procesoNuevo->estado = LISTO;
	procesoNuevo->tiempoRespuesta = (t_periodo*) malloc(sizeof(t_periodo));
	procesoNuevo->tiempoRespuesta->tiempoInicio = temporal_get_string_time();
	procesoNuevo->tiempoRespuesta->tiempoFin = NULL;
	procesoNuevo->tiempoEjecucion = list_create();
	procesoNuevo->tiempoEspera = list_create();
	comienzoDePeriodoEspera = (t_periodo*) malloc(sizeof(t_periodo));
	comienzoDePeriodoEspera->tiempoInicio = temporal_get_string_time();
	list_add(procesoNuevo->tiempoEspera, comienzoDePeriodoEspera);

	pthread_mutex_lock(&mutex_procesos);
	pthread_mutex_lock(&mutex_colaReady);
//	printf("Se genero un proceso de nombre (%s), path (%s) y pid (%d)\n", procesoNuevo->nombre, procesoNuevo->path, procesoNuevo->pid);
//	printf("La lista de procesos tenía (%d) elementos y la cola de ready (%d) elementos\n", list_size(procesos), queue_size(procesosListos));
	list_add(procesos, (void*) procesoNuevo);
	queue_push(procesosListos, (void*) procesoNuevo->pid);
//	printf("La lista de procesos tiene (%d) elementos y la cola de ready (%d) elementos\n", list_size(procesos), queue_size(procesosListos));
	pthread_mutex_unlock(&mutex_colaReady);
	pthread_mutex_unlock(&mutex_procesos);

	pthread_mutex_lock(&mutex_logs);
	log_info(logs, "El mProc (%s) de PID (%d) ha sido creado y encolado", procesoNuevo->nombre, procesoNuevo->pid);
	pthread_mutex_unlock(&mutex_logs);

	return;
}

void core_status(t_list* cpuConectadas, t_list* procesos){
	int i = 0;
	t_cpu* cpuAux;
	t_pcb* procAux;
	for (i=0; i<list_size(cpuConectadas); i++){
		cpuAux = (t_cpu*) list_get(cpuConectadas, i);
		if(cpuAux->estado == 1){
			printf("-> CPU %d: Libre\n", cpuAux->core);
		}
		else{
			procAux = get_proceso(procesos, cpuAux->ultimoPidEjecutado);
			printf("-> CPU %d: Ejecutando (%s) de PID %d\n", cpuAux->core, procAux->nombre, procAux->pid);
		}
	}
}

t_pcb* get_proceso(t_list* procesos, int pid){
	t_pcb* procesoSeleccionado;
	int i = 0;
	int encontrado = 0;

	for (i=0; i<list_size(procesos); i++){
		procesoSeleccionado = (t_pcb*) list_get(procesos, i);
		if(procesoSeleccionado->pid == pid){
			i = list_size(procesos);
			encontrado = 1;
		}
	}
	if(encontrado)
		return procesoSeleccionado;
	else
		return NULL;
}

void get_tiempo(int* hora, int* minuto, int* segundo, int* ms){
	char* tiempoActual = temporal_get_string_time(); //hh:mm:ss:mmm
	*hora = atoi( string_substring(tiempoActual, 0, 2) );
	*minuto = atoi( string_substring(tiempoActual, 3, 2) );
	*segundo = atoi( string_substring(tiempoActual, 6, 2) );
	*ms = atoi( string_substring(tiempoActual, 9, 3) );
	//printf("La hora es: %d:%d:%d:%d \n", *hora, *minuto, *segundo, *ms);
	//printf("La hora es: %s \n", tiempoActual);
}

void convertir_tiempo(char* tiempoActual, int* hora, int* minuto, int* segundo, int* ms){
	*hora = atoi( string_substring(tiempoActual, 0, 2) );
	*minuto = atoi( string_substring(tiempoActual, 3, 2) );
	*segundo = atoi( string_substring(tiempoActual, 6, 2) );
	*ms = atoi( string_substring(tiempoActual, 9, 3) );
	//printf("La hora es: %d:%d:%d:%d \n", *hora, *minuto, *segundo, *ms);
	//printf("La hora es: %s \n", tiempoActual);
}

void loguear_metricas(t_log* logs, t_pcb* proceso){
	int tiempoRespuesta = 0;
	int tiempoEjecucion = 0;
	int tiempoEspera = 0;
	int horaInicio, minutoInicio, segundoInicio, msInicio;
	int horaFin, minutoFin, segundoFin, msFin;
	int i,j;
	t_periodo* periodoAux;

	convertir_tiempo(proceso->tiempoRespuesta->tiempoInicio, &horaInicio, &minutoInicio, &segundoInicio, &msInicio);
	convertir_tiempo(proceso->tiempoRespuesta->tiempoFin, &horaFin, &minutoFin, &segundoFin, &msFin);
	tiempoRespuesta = (horaFin - horaInicio)*1000*60*60 + (minutoFin-minutoInicio)*1000*60 + (segundoFin - segundoInicio)*1000 + (msFin - msInicio);

	for(i = 0; i < list_size(proceso->tiempoEjecucion); i++){
		periodoAux = (t_periodo*) list_get(proceso->tiempoEjecucion, i);
		convertir_tiempo(periodoAux->tiempoInicio, &horaInicio, &minutoInicio, &segundoInicio, &msInicio);
		convertir_tiempo(periodoAux->tiempoFin, &horaFin, &minutoFin, &segundoFin, &msFin);
		tiempoEjecucion += (horaFin - horaInicio)*1000*60*60 + (minutoFin-minutoInicio)*1000*60 + (segundoFin - segundoInicio)*1000 + (msFin - msInicio);
	}

	for(j = 0; j < list_size(proceso->tiempoEspera); j++){
		periodoAux = (t_periodo*) list_get(proceso->tiempoEspera, j);
		convertir_tiempo(periodoAux->tiempoInicio, &horaInicio, &minutoInicio, &segundoInicio, &msInicio);
		convertir_tiempo(periodoAux->tiempoFin, &horaFin, &minutoFin, &segundoFin, &msFin);
		tiempoEspera += (horaFin - horaInicio)*1000*60*60 + (minutoFin-minutoInicio)*1000*60 + (segundoFin - segundoInicio)*1000 + (msFin - msInicio);
	}

	log_info(logs, "Tiempo de Respuesta mProc (%s): %d ms", proceso->nombre, tiempoRespuesta);
	log_info(logs, "Tiempo de Ejecucion mProc (%s): %d ms", proceso->nombre, tiempoEjecucion);
	log_info(logs, "Tiempo de Espera mProc (%s): %d ms", proceso->nombre, tiempoEspera);

}

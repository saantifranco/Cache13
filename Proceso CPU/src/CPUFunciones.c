#include "CPUFunciones.h"
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
#include <pthread.h>
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"

//FALTA VER QUE VALOR RETORNA
//0 INICIAR (a la MEMORIA para la TLB)
//1 LEER (a la MEMORIA)
//2 ESCRIBIR (a la MEMORIA)
//3 FINALIZAR (a la MEMORIA para la TLB)



t_paquete_para_memoria* enviarPaginasYTexto(int protocolo, int paginas, char* texto,int pid){
	t_paquete_para_memoria* memoria = malloc (sizeof(t_paquete_para_memoria));
	memoria->instruccion = protocolo;
	memoria->pagina = paginas;
	memoria->texto = texto;
	memoria->PID = pid;
	return memoria;
}

void enviarPaginas(int protocolo, int paginas, t_paquete_para_memoria* memoria, int pid){
	memoria->instruccion = protocolo;
	memoria->pagina = paginas;
	memoria->PID = pid;
	memoria->texto = "nada";
}

//
int obtenerPaginas(char instruccion[],int start)
{
	int i = 0;
	int paginasNum;
	while(instruccion[i]!='\n'){
		i++;
	}
	char* miInstruccion = malloc(sizeof(char)*i+1);
	miInstruccion = string_duplicate(instruccion);
	char* paginas = string_substring_from(miInstruccion,start);
	paginasNum = atoi(paginas);
	return paginasNum;
}
//

char* obtenerTexto(char instruccion[],int start){
	int i = 0;
	while(instruccion[i]!='\n'){
		i++;
	}
	char* miInstruccion = malloc(sizeof(char)*i+1);
	miInstruccion = string_duplicate(instruccion);
	char* texto = string_substring_from(miInstruccion,start-1);
	texto = string_substring(texto, 0, strlen(texto)-2); //saca el ;

	texto= string_substring(texto,1,strlen(texto)-2); //saca las " " del texto
	return texto;
}


int lecturaInstruccion(int socketMemoria,char letra,char instruccion[],int numeroCore,t_log* logs,t_paquete_ejecutar* cpu,t_list* respuestaPlanif,t_list* periodosEjecucion) {

	//inicializar variable para lectura

	int resultadoEjecucion;
	int paginas, tiempo;
	char* texto;
	int resultadoSendAll;
	int resultadoRecvAll;
	char* unLog;
	char* paraPlanif;
	t_periodo* comienzoDePeriodoEjecucion;
	t_periodo* finDePeriodoEjecucion;

	//
	t_paquete_para_memoria* memoria = malloc (sizeof(t_paquete_para_memoria));
	t_paquete_resultado_instruccion* resultadoInstruccion = malloc (sizeof(t_paquete_resultado_instruccion));

	t_paquete_envio* paquete = malloc(sizeof(t_paquete_envio));
	t_paquete_envio* package = malloc(sizeof(t_paquete_envio));
	//creo los paquetes 2memoria, 1planif

	//analisis de instrucciones
	switch (letra)
	{
		case 'i': //iniciar

			//
			paginas = obtenerPaginas(instruccion, 7);
			enviarPaginas(0, paginas, memoria, cpu->pid);
			//calculo de paginas y armado de paquete
			pthread_mutex_lock(&mutex_logs);
			log_info(logs,"CPU: %d Instruccion: INICIAR",numeroCore);
			log_info(logs,"CPU: %d PID: %d",numeroCore,cpu->pid);
			log_info(logs,"CPU: %d Paginas: %d",numeroCore,paginas);
			pthread_mutex_unlock(&mutex_logs);
			//
			pthread_mutex_lock(&mutex_envioMemoria);
			paquete = envioA_Memoria_Serializer(memoria);

			pthread_mutex_lock(&mutex_listaStatus);
			//finDePeriodoEjecucion = obtenerUltimoPeriodoDeEjecucion(periodosEjecucion, numeroCore);
			//finDePeriodoEjecucion->tiempoFin = temporal_get_string_time();
			pthread_mutex_unlock(&mutex_listaStatus);

			resultadoSendAll = SendAll(socketMemoria,paquete);
			//envio a memoria

			//

			resultadoRecvAll = RecvAll2(socketMemoria, package);
			resultadoInstruccion = deserializar_t_paquete_resultado_instruccion(package);
			pthread_mutex_unlock(&mutex_envioMemoria);
			pthread_mutex_lock(&mutex_listaStatus);
			comienzoDePeriodoEjecucion = (t_periodo*) malloc(sizeof(t_periodo));
			comienzoDePeriodoEjecucion->core = numeroCore;
			comienzoDePeriodoEjecucion->tiempoInicio = temporal_get_string_time();
			comienzoDePeriodoEjecucion->tiempoFin = NULL;
			list_add(periodosEjecucion, comienzoDePeriodoEjecucion);
			pthread_mutex_unlock(&mutex_listaStatus);

			//recibe de memoria
			//printf("%s \n",resultadoInstruccion->contenido);
			//printf("%d \n",resultadoInstruccion->error);
			//
			if(resultadoInstruccion->error==0) //se inicio correctamente
			{
				pthread_mutex_lock(&mutex_logs);
				log_info(logs,"CPU: %d mProc:%d - Iniciado",numeroCore,cpu->pid);
				pthread_mutex_unlock(&mutex_logs);
				unLog = malloc(sizeof(char)*27);

				sprintf(unLog,"mProc: %d - Iniciado", cpu->pid);

				resultadoEjecucion=0;

			}
			else //hubo error en iniciarse
			{
				pthread_mutex_lock(&mutex_logs);
				log_info(logs,"CPU: %d mProc:%d - Fallo",numeroCore,cpu->pid);
				pthread_mutex_unlock(&mutex_logs);
				unLog = malloc(sizeof(char)*25);

				sprintf(unLog,"mProc: %d - Fallo", cpu->pid);
				resultadoEjecucion= -1;
			}

			break;
			//me fijo como fue lo que me mando memoria y completo el paquete para planif


		case 'l': //leer

			//
			paginas = obtenerPaginas(instruccion, 4);
			enviarPaginas(1, paginas, memoria, cpu->pid);
			//calculo de paginas y armado de paquete
			pthread_mutex_lock(&mutex_logs);
			log_info(logs,"CPU: %d Instruccion: LEER",numeroCore);
			log_info(logs,"CPU: %d PID: %d",numeroCore,cpu->pid);
			log_info(logs,"CPU: %d Paginas: %d",numeroCore,paginas);
			pthread_mutex_unlock(&mutex_logs);
			//
			pthread_mutex_lock(&mutex_envioMemoria);
			paquete = envioA_Memoria_Serializer(memoria);

			pthread_mutex_lock(&mutex_listaStatus);
			//finDePeriodoEjecucion = obtenerUltimoPeriodoDeEjecucion(periodosEjecucion, numeroCore);
			//finDePeriodoEjecucion->tiempoFin = temporal_get_string_time();
			pthread_mutex_unlock(&mutex_listaStatus);
			resultadoSendAll = SendAll(socketMemoria,paquete);
			//envio a memoria

			//
			resultadoRecvAll = RecvAll2(socketMemoria, package);
			resultadoInstruccion = deserializar_t_paquete_resultado_instruccion(package);
			pthread_mutex_unlock(&mutex_envioMemoria);
			pthread_mutex_lock(&mutex_listaStatus);
			comienzoDePeriodoEjecucion = (t_periodo*) malloc(sizeof(t_periodo));
			comienzoDePeriodoEjecucion->core = numeroCore;
			comienzoDePeriodoEjecucion->tiempoInicio = temporal_get_string_time();
			comienzoDePeriodoEjecucion->tiempoFin = NULL;
			list_add(periodosEjecucion, comienzoDePeriodoEjecucion);
			pthread_mutex_unlock(&mutex_listaStatus);

			//recibo de memoria

			//
			if(resultadoInstruccion->error==0)//no hay error de lectura
			{
				pthread_mutex_lock(&mutex_logs);
				log_info(logs,"CPU: %d mProc: %d - Pagina: %d leida:%s",numeroCore,cpu->pid,paginas,resultadoInstruccion->contenido);
				pthread_mutex_unlock(&mutex_logs);

				unLog = malloc(sizeof(char)*34+strlen(resultadoInstruccion->contenido)+1); //ver esto
				sprintf(unLog,"mProc: %d - Pagina: %d leida: ", cpu->pid,paginas);
				strcat(unLog,resultadoInstruccion->contenido);

				resultadoEjecucion=0;

			}
			//¿¿¿¿hay error en lectura????

			break;

		case 'e': //escribir o entrada-salida

			if((letra=instruccion[1])=='n') //entrada-salida
			{

				tiempo = obtenerPaginas(instruccion, 14); //calculo del tiempo del E/S
				pthread_mutex_lock(&mutex_logs);
				log_info(logs,"CPU: %d Instruccion: ENTRADA-SALIDA",numeroCore);
				log_info(logs,"CPU: %d PID: %d",numeroCore,cpu->pid);
				log_info(logs,"CPU: %d TIEMPO DE E/S: %d",numeroCore,tiempo);
				pthread_mutex_unlock(&mutex_logs);

				pthread_mutex_lock(&mutex_logs);
				log_info(logs,"CPU: %d mProc: %d en entrada-salida de tiempo: %d",numeroCore,cpu->pid,tiempo);
				pthread_mutex_unlock(&mutex_logs);
				unLog = malloc(sizeof(char)*100);

				sprintf(unLog,"mProc: %d en entrada-salida de tiempo %d", cpu->pid,tiempo);

				resultadoEjecucion = -1;

				//completo paquete para planif

			}
			if((letra=instruccion[1])=='s') //escribir
			{
				//

				paginas = obtenerPaginas(instruccion, 8);
				int lasPaginas;
				lasPaginas = paginas;
				int cifrasPaginas = 0;
				while(lasPaginas/10>0)
				{
					lasPaginas = lasPaginas/10;
					cifrasPaginas++;
				}
				int tamanioComienzo = 12+cifrasPaginas;
				texto = obtenerTexto(instruccion,tamanioComienzo);


				memoria = enviarPaginasYTexto(2, paginas, texto,cpu->pid);
				//calculo de paginas y de texto , y armado de paquete

				pthread_mutex_lock(&mutex_logs);
				log_info(logs,"CPU: %d Instruccion: ESCRIBIR",numeroCore);
				log_info(logs,"CPU: %d PID: %d",numeroCore,cpu->pid);
				log_info(logs,"CPU: %d Paginas: %d",numeroCore,paginas);
				log_info(logs,"CPU: %d Texto: %s",numeroCore,texto);
				pthread_mutex_unlock(&mutex_logs);
				//
				pthread_mutex_lock(&mutex_envioMemoria);
				paquete = envioA_Memoria_Serializer(memoria);

				pthread_mutex_lock(&mutex_listaStatus);
				//finDePeriodoEjecucion = obtenerUltimoPeriodoDeEjecucion(periodosEjecucion, numeroCore);
				//finDePeriodoEjecucion->tiempoFin = temporal_get_string_time();
				pthread_mutex_unlock(&mutex_listaStatus);

				resultadoSendAll = SendAll(socketMemoria,paquete);
				//envio a memoria

				//
				resultadoRecvAll = RecvAll2(socketMemoria, package);
				resultadoInstruccion = deserializar_t_paquete_resultado_instruccion(package);
				pthread_mutex_unlock(&mutex_envioMemoria);
				//recibo de memoria
				pthread_mutex_lock(&mutex_listaStatus);
				comienzoDePeriodoEjecucion = (t_periodo*) malloc(sizeof(t_periodo));
				comienzoDePeriodoEjecucion->core = numeroCore;
				comienzoDePeriodoEjecucion->tiempoInicio = temporal_get_string_time();
				comienzoDePeriodoEjecucion->tiempoFin = NULL;
				list_add(periodosEjecucion, comienzoDePeriodoEjecucion);
				pthread_mutex_unlock(&mutex_listaStatus);

				//
				if(resultadoInstruccion->error==0) //no hay error
				{
					pthread_mutex_lock(&mutex_logs);
					log_info(logs,"CPU: %d mProc: %d - Pagina: %d escrita: %s",numeroCore,cpu->pid,paginas,texto);
					pthread_mutex_unlock(&mutex_logs);

					unLog = malloc(sizeof(char)*36 +strlen(texto)+1);
					sprintf(unLog,"mProc: %d - Pagina: %d escrita:", cpu->pid,paginas);
					strcat(unLog,texto);

					resultadoEjecucion=0;

				}
				if(resultadoInstruccion->error==-1)
				{
					pthread_mutex_lock(&mutex_logs);
					log_info(logs,"CPU:%d mProc: %d - Pagina: %d no ha podido ser escrita",numeroCore,cpu->pid,paginas);
					pthread_mutex_unlock(&mutex_logs);

					unLog = malloc(sizeof(char)*51 + 1);
					sprintf(unLog,"CPU:%d mProc: %d - Pagina: %d no ha podido ser escrita",numeroCore,cpu->pid,paginas);

					resultadoEjecucion=0;
				}
				//¿¿hay error en escribir??
			}
			break;

		case 'f': //finalizar

			enviarPaginas(3,0, memoria, cpu->pid); //codigo 3 es finalizar para memoria y cerrar sus estructuras
			//armado de paquete a memoria

			pthread_mutex_lock(&mutex_logs);
			log_info(logs,"CPU: %d Instruccion: FINALIZAR",numeroCore);
			log_info(logs,"CPU: %d PID: %d",numeroCore,cpu->pid);
			pthread_mutex_unlock(&mutex_logs);
			//
			pthread_mutex_lock(&mutex_envioMemoria);
			paquete = envioA_Memoria_Serializer(memoria);

			pthread_mutex_lock(&mutex_listaStatus);
			//finDePeriodoEjecucion = obtenerUltimoPeriodoDeEjecucion(periodosEjecucion, numeroCore);
			//finDePeriodoEjecucion->tiempoFin = temporal_get_string_time();
			pthread_mutex_unlock(&mutex_listaStatus);

			resultadoSendAll = SendAll(socketMemoria,paquete);
			pthread_mutex_unlock(&mutex_envioMemoria);
			//envio a memoria

			pthread_mutex_lock(&mutex_logs);
			log_info(logs,"CPU: %d mProc: %d finalizado",numeroCore,cpu->pid);
			pthread_mutex_unlock(&mutex_logs);

			unLog = malloc(sizeof(char)*28);
			sprintf(unLog,"mProc: %d finalizado", cpu->pid);

			resultadoEjecucion=-2;

			break;


		default:
			pthread_mutex_lock(&mutex_logs);
			log_info(logs,"CPU: %d no comprende la proxima instruccion a ejecutar del PID:%d",numeroCore,cpu->pid);
			pthread_mutex_unlock(&mutex_logs);
			break;
	}

	paraPlanif = string_duplicate(unLog);
	list_add(respuestaPlanif,(void*)paraPlanif);

	//
	free(memoria);

	free(resultadoInstruccion);
	free(unLog);
	//free(paraPlanif);
	free(paquete);
	free(package);
	//libero espacio de memoria usados

	return resultadoEjecucion; //devuelvo el resultado de ejecucion de instruccion
	// 1 NO COMPRENDE LA INSTRUCCION
	// 0 CORRECTO
	//-1 Finaliza CPU

}


void arranqueCPU(t_paquete_ejecutar* cpu,t_conexiones_cores* data ,int socketPlanificador,int socketMemoria, int numeroCore,t_list* periodosEjecucion){

	t_log* logs = data->logs;
	//int idCPU = data->idCPU;
	CPUConfig* config = data->configCore;
	//printf("%s \n", cpu->path);
	//printf("%d \n", cpu->proximaInstruccion);
	//printf("%d \n", cpu->pid);
	//printf("%d \n", cpu->rafaga);
	//printf("%d \n", numeroCore);
	t_periodo* comienzoDePeriodoEjecucion;
	t_periodo* finDePeriodoEjecucion;


	//inicializar variable para la ejecucion de tareas
	int valorRetorno=0; //entero que devuelve el resultado de lectura de una instruccion
	int punteroFinal; //valor de puntero que le devuelve al planif
	int contadorArchivo = 0; //contador de lineas del archivo
	int contadorInstrucciones=1; //contador de la rafaga
	int enviado; //entero para el retorno de Enviar()

	t_list_retorno* retorno = (t_list_retorno*) malloc(sizeof(t_list_retorno));
	retorno->retornos = list_create();
	t_list* respuestaPlanif = retorno->retornos;

	//
	char letra;
	//char instruccion[100];
	//CLAVE para leer cada linea / cada instruccion

	char * instruccion = NULL; //una linea del archivo
	int i, c; //contadores


	//
	FILE* contextoEjecucion;
	contextoEjecucion = fopen(cpu->path,"r");
	//path apertura

	//
	if (contextoEjecucion ==NULL)
	{
		fputs("Error en la apertura del archivo \n",stderr);
		exit(1);
	}
	//cierre del path


	if (cpu->rafaga==0)
	{
		pthread_mutex_lock(&mutex_logs);
		log_info(logs,"CPU: %d está ejecutando el archivo: %s teniendo como proxima instruccion numero: %d con algoritmo FIFO",numeroCore,cpu->path,cpu->proximaInstruccion);
		pthread_mutex_unlock(&mutex_logs);
	}
	else
	{
		pthread_mutex_lock(&mutex_logs);
		log_info(logs,"CPU: %d está ejecutando el archivo: %s teniendo como proxima instruccion numero: %d con un QUANTUM de: %d",numeroCore,cpu->path,cpu->proximaInstruccion,cpu->rafaga);
		pthread_mutex_unlock(&mutex_logs);
	}

	//0 FIFO
	//X>0 QUANTUM

	//printf("El PUNTERO INICIAL ES:%d",cpu->proximaInstruccion);
	//lectura de cada instruccion
	while(feof(contextoEjecucion)==0 && contadorArchivo <= cpu->proximaInstruccion && valorRetorno != -2)
	//llegada hasta el puntero que nos paso Planif
	{

		if(contadorArchivo==cpu->proximaInstruccion) // arribo al puntero solicitado
		{
			pthread_mutex_lock(&mutex_listaStatus);
			comienzoDePeriodoEjecucion = (t_periodo*) malloc(sizeof(t_periodo));
			comienzoDePeriodoEjecucion->core = numeroCore;
			comienzoDePeriodoEjecucion->tiempoInicio = temporal_get_string_time();
			comienzoDePeriodoEjecucion->tiempoFin = NULL;
			list_add(periodosEjecucion, comienzoDePeriodoEjecucion);
			pthread_mutex_unlock(&mutex_listaStatus);
			if (cpu->rafaga ==0) //FIFO
			{
				while(valorRetorno > -1)//mientras que sea distinto de finalizar,iniciar mal o entrada-salida..
				{

					c = fgetc(contextoEjecucion);

					instruccion = (char*)realloc(NULL, sizeof(char));
					i = 0;
					while( c != ';')
					{
						instruccion[i] = c;
						i++;
						instruccion = (char*)realloc(instruccion, (i+1)*sizeof(char));
						c = fgetc(contextoEjecucion);
					}
					if(c == ';')
					{
						instruccion[i] = c;
						i++;
						instruccion = (char*)realloc(instruccion, (i+1)*sizeof(char));
						c = fgetc(contextoEjecucion);
					}
					/*Agrego el \n al buffer*/
					instruccion = (char*)realloc(instruccion, (i+3)*sizeof(char));
					instruccion[i] = '\n';
					instruccion[i+1] = '\0';
					letra=instruccion[0];


					valorRetorno = lecturaInstruccion(socketMemoria,letra,instruccion,numeroCore,logs,cpu,respuestaPlanif,periodosEjecucion);
					free(instruccion);
					//printf("Resultado de instruccion %d \n", valorRetorno);


					contadorInstrucciones++;
					t_instruccionesPorCpu* aux;
					for(i=0; i<list_size(instruccionesPorCpu); i++){
						aux = list_get(instruccionesPorCpu, i);
						if(aux->core == numeroCore){
							aux->instrucciones++;
							usleep(config->RETARDO*1000000); //duerme al proceso un tiempo configurable
							//aux->instrucciones++;

						}
					}

				}
			}
			else //RRQ
			{

				while(valorRetorno > -1 && contadorInstrucciones <=cpu->rafaga)//mientras que sea distinto de finalizar,iniciar mal o entrada-salida..
				{
					//printf("%d");
					c = fgetc(contextoEjecucion);

					instruccion = (char*)realloc(NULL, sizeof(char));
					i = 0;
					while( c != ';')
					{
						instruccion[i] = c;
						i++;
						instruccion = (char*)realloc(instruccion, (i+1)*sizeof(char));
						c = fgetc(contextoEjecucion);
					}
					if(c == ';')
					{
						instruccion[i] = c;
						i++;
						instruccion = (char*)realloc(instruccion, (i+1)*sizeof(char));
						c = fgetc(contextoEjecucion);
					}
					/*Agrego el \n al buffer*/
					instruccion = (char*)realloc(instruccion, (i+3)*sizeof(char));
					instruccion[i] = '\n';
					instruccion[i+1] = '\0';
					letra=instruccion[0];


					valorRetorno = lecturaInstruccion(socketMemoria,letra,instruccion,numeroCore,logs,cpu,respuestaPlanif,periodosEjecucion);
					free(instruccion);
					//printf("Resultado de instruccion %d \n", valorRetorno);

					contadorInstrucciones++;
					t_instruccionesPorCpu* aux;
					for(i=0; i<list_size(instruccionesPorCpu); i++){
						aux = list_get(instruccionesPorCpu, i);
						if(aux->core == numeroCore){
							aux->instrucciones++;
							usleep(config->RETARDO*1000000); //duerme al proceso un tiempo configurable
							//aux->instrucciones++;
						}
					}

				}

				if(contadorInstrucciones > cpu->rafaga)
				{
					pthread_mutex_lock(&mutex_logs);
					log_info(logs,"CPU: %d ha concluido el PID: %d con QUANTUM de: %d",numeroCore,cpu->pid,cpu->rafaga);
					pthread_mutex_unlock(&mutex_logs);


				}

				if(valorRetorno ==-1 && contadorInstrucciones <= cpu->rafaga)
				{

					contadorInstrucciones--;

				}

			}
			contadorInstrucciones--; //vuelve al valor de adentro del while
		}
		//if(valorRetorno ==-1) printf("SALIO POR E/S\n");
		if(valorRetorno !=-2)
		{

			c = fgetc(contextoEjecucion);
			instruccion = (char*)realloc(NULL, sizeof(char));
			i = 0;
			while( c != ';')
			{
				instruccion[i] = c;
				i++;
				instruccion = (char*)realloc(instruccion, (i+1)*sizeof(char));
				c = fgetc(contextoEjecucion);
			}
			if(c == ';')
			{
				instruccion[i] = c;
				i++;
				instruccion = (char*)realloc(instruccion, (i+1)*sizeof(char));
				c = fgetc(contextoEjecucion);
			}
			/*Agrego el \n al buffer*/
			instruccion = (char*)realloc(instruccion, (i+3)*sizeof(char));
			instruccion[i] = '\n';
			instruccion[i+1] = '\0';
			contadorArchivo++; //sale del while
		}
	}
	//ya termino de leer lo correspondiente

	contadorArchivo--; //vuelve al puntero que nos dio el planif
	if(cpu->rafaga==0)contadorInstrucciones--;//resta ya que se inicializa en 1 (solo para fifo)
	if(contadorInstrucciones == cpu->rafaga && valorRetorno ==-1)
	{
		contadorInstrucciones--;
	}

	//puntero que retorna al planif
	punteroFinal = contadorInstrucciones+cpu->proximaInstruccion;

	//
	if (fclose(contextoEjecucion)!= 0)
	{
		printf( "Problemas al cerrar el fichero\n" );
	}
	//cierre del path

	//
	t_paquete_envio* paquete_envio = malloc(sizeof(t_paquete_envio));
	retorno->proximaInstruccion = punteroFinal;
	paquete_envio = serializar_t_list_retorno(retorno);

	pthread_mutex_lock(&mutex_listaStatus);
	//finDePeriodoEjecucion = obtenerUltimoPeriodoDeEjecucion(periodosEjecucion, numeroCore);
	//finDePeriodoEjecucion->tiempoFin = temporal_get_string_time();
	pthread_mutex_unlock(&mutex_listaStatus);

	enviado = SendAll(socketPlanificador, paquete_envio);
	//envio al planificador del resultado de cada instruccion

	//

	free(paquete_envio->data);
	free(paquete_envio);
	list_clean(retorno->retornos);
	list_destroy(retorno->retornos);
	free(retorno);
	//libero todos los espacios de memoria solicitados


}

void* ejecucionCPU(void*data)
{
	t_conexiones_cores* dataHilos = (t_conexiones_cores*) data;
	t_list* periodosEjecucion  = dataHilos->periodosEjecucion;
	int numeroCore = dataHilos->idCPU;
	t_periodo* comienzoDePeriodoEjecucion;
	t_periodo* finDePeriodoEjecucion;

	int recibe;
	int envio;

	//conexion con planificador
	int socketPlanificador = Conectar(dataHilos->configCore->IP_PLANIFICADOR, dataHilos->configCore->PUERTO_PLANIFICADOR);

	if (socketPlanificador == -1)
	{
		pthread_mutex_lock(&mutex_logs);
		log_info(dataHilos->logs, "CPU: %d no pudo conectarse con el Planificador",numeroCore);
		pthread_mutex_unlock(&mutex_logs);
		exit(1);
	}
	pthread_mutex_lock(&mutex_logs);
	log_info(dataHilos->logs,"CPU:%d se ha conectado con el Planificador",numeroCore);
	pthread_mutex_unlock(&mutex_logs);
	//
	t_paquete_envio* paqueteSaludo = malloc(sizeof(t_paquete_envio));
	recibe = RecvAll(socketPlanificador,paqueteSaludo);
	char* saludo = deserializar_string(paqueteSaludo);
	printf("%s\n",saludo);
	free(paqueteSaludo);
	//recibeSaludoPlanif

	//

	t_paquete_envio* paqueteID = malloc(sizeof(t_paquete_envio));
	char* core = string_itoa(dataHilos->idCPU);
	paqueteID = serializar_string(core);
	envio = SendAll(socketPlanificador,paqueteID);
	free(paqueteID);


	//conexion con memoria
	int socketMemoria = Conectar(dataHilos->configCore->IP_MEMORIA, dataHilos->configCore->PUERTO_MEMORIA);
	if (socketMemoria == -1)
	{
		pthread_mutex_lock(&mutex_logs);
		log_info(dataHilos->logs, "CPU: %d no pudo conectarse con el Administrador de Memoria",dataHilos->idCPU);
		pthread_mutex_unlock(&mutex_logs);
		exit(1);
	}
	pthread_mutex_lock(&mutex_logs);
	log_info(dataHilos->logs,"CPU: %d se ha conectado con el Administrador de Memoria",dataHilos->idCPU);
	pthread_mutex_unlock(&mutex_logs);

	pthread_mutex_unlock(&mutex_Core);

	while(1)
	{
		t_paquete_ejecutar* paqueteEjecutar = malloc(sizeof(t_paquete_ejecutar));
		t_paquete_envio* package = malloc(sizeof(t_paquete_envio));


		//recibe un paquete del Planificador
		recibe = RecvAll2(socketPlanificador, package);
		paqueteEjecutar = deserializar_t_paquete_ejecutar(package);
		pthread_mutex_lock(&mutex_logs);
		log_info(dataHilos->logs,"CPU: %d ha recibido un Contexto de Ejecucion",numeroCore);
		pthread_mutex_unlock(&mutex_logs);
		//empieza el trabajo de la CPU
		pthread_mutex_lock(&mutex_listaStatus);
		comienzoDePeriodoEjecucion = (t_periodo*) malloc(sizeof(t_periodo));
		comienzoDePeriodoEjecucion->tiempoInicio = temporal_get_string_time();
		list_add(periodosEjecucion, comienzoDePeriodoEjecucion);
		pthread_mutex_unlock(&mutex_listaStatus);
		ejecucionIniciada = 1;
		arranqueCPU(paqueteEjecutar,dataHilos,socketPlanificador,socketMemoria, numeroCore,periodosEjecucion);
		free(paqueteEjecutar);
		free(package);
	}

	//se desconecta de ambos procesos
	Desconectar(socketPlanificador);
	Desconectar(socketMemoria);

	free(dataHilos->logs);
	free(dataHilos->configCore);
	free(dataHilos);

}

void get_tiempo(int* hora, int* minuto, int* segundo, int* ms){
	char* tiempoActual = temporal_get_string_time(); //hh:mm:ss:mmm
	*hora = atoi( string_substring(tiempoActual, 0, 2) );
	*minuto = atoi( string_substring(tiempoActual, 3, 2) );
	*segundo = atoi( string_substring(tiempoActual, 6, 2) );
	*ms = atoi( string_substring(tiempoActual, 9, 3) );

}

void convertir_tiempo(char* tiempoActual, int* hora, int* minuto, int* segundo, int* ms){
	*hora = atoi( string_substring(tiempoActual, 0, 2) );
	*minuto = atoi( string_substring(tiempoActual, 3, 2) );
	*segundo = atoi( string_substring(tiempoActual, 6, 2) );
	*ms = atoi( string_substring(tiempoActual, 9, 3) );

}

void* cpu_status(void* data){
	int recibe;
	int envio;

	t_conexiones_cores* dataHilos = (t_conexiones_cores*) data;
	t_list* periodosEjecucion  = dataHilos->periodosEjecucion;
	t_list* status = list_create();

	//conexion con planificador
	int socketPlanificador = Conectar(dataHilos->configCore->IP_PLANIFICADOR, dataHilos->configCore->PUERTO_PLANIFICADOR);
	pthread_mutex_unlock(&mutex_Core);
	//
	t_paquete_envio* paqueteSaludo = malloc(sizeof(t_paquete_envio));

	int p;
	int c;
	while(1)
	{
		t_paquete_envio* paqueteSaludo = malloc(sizeof(t_paquete_envio));
		recibe = RecvAll2(socketPlanificador,paqueteSaludo);
		free(paqueteSaludo);

		c = 0;
		t_instruccionesPorCpu* aux;
		for(c=0; c<list_size(instruccionesPorCpu); c++){
			aux = list_get(instruccionesPorCpu, c);
			char* unStatus = (char*) malloc(100);
			sprintf(unStatus, "cpu %d: %lf %c", aux->core, aux->uso, '%');
			list_add(status, unStatus);
		}

		t_paquete_envio* paqueteID = malloc(sizeof(t_paquete_envio));
		paqueteID = serializar_cpu_status(status);
		envio = SendAll(socketPlanificador,paqueteID);
		free(paqueteID);
		p=0;
		char* statusAux;
		for(p=0; p<list_size(status); p++){
			statusAux = list_remove(status, p);
			free(statusAux);
		}
		list_destroy(status);
		status = list_create();

	}
}








//************************FUNCIONES SIN USAR***********************************//
int contarTamanioPath(FILE* contextoEjecucion,t_paquete_ejecutar* cpu){
	//inicializar variables
	int tamanio;
	int contadorHastaPuntero;
	char instruccion[100];
	int punteroArchivo =cpu->proximaInstruccion;
	contextoEjecucion = fopen(cpu->path,"r");

	//cuenta mientras no llegue al puntero que nos dio Planif
	while(contadorHastaPuntero <=punteroArchivo)
	{
		fgets(instruccion,100,contextoEjecucion);

		if(contadorHastaPuntero==punteroArchivo) //si llega al puntero..
		{
			//cuenta hasta que encuentra un finalizar o una E/S
			while(strcmp(instruccion,"finalizar") && strcmp(instruccion,"entrada-salida"))
			{
				fgets(instruccion,100,contextoEjecucion);
				tamanio++; //calcula la cantidad de lineas
			}
		}
		contadorHastaPuntero++;
	}
	//

	//
	fclose(contextoEjecucion);
	//cierra el archivo
	return tamanio;//devuelve el tamaño que va a ser el struct
}

void* temporizador(void* data){

	t_conexiones_cores* dataHilos = (t_conexiones_cores*) data;
	//t_list instruccionesPorCpu = dataHilos->instruccionesPorCpu;
	 struct itimerval it_val;  /* for setting itimer */

	  /* Upon SIGALRM, call DoStuff().
	   * Set interval timer.  We want frequency in ms,
	   * but the setitimer call needs seconds and useconds. */
	  if (signal(SIGALRM, (void (*)(int)) timer_handler) == SIG_ERR) {
	    perror("Unable to catch SIGALRM");
	    exit(1);
	  }
	  it_val.it_value.tv_sec = 1;
	  it_val.it_value.tv_usec = 0;
	  it_val.it_interval = it_val.it_value;
	  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
	    perror("error calling setitimer()");
	    exit(1);
	  }

	  while (1)
	    pause();
}

void timer_handler (/*int signum*/){
	if(ejecucionIniciada){
		static int segundos = 0;
		segundos++;
		t_instruccionesPorCpu* aux;
		int i = 0;
		double maximoInstrucciones = segundos/retardoGlobal;
		if(segundos == 55){
			printf("El CPU Status esta a 5 segundos de reiniciarse\n");
					}
		if(segundos == 60){

			for(i=0; i<list_size(instruccionesPorCpu); i++){
				aux = list_get(instruccionesPorCpu, i);
				if(maximoInstrucciones !=0){
					if(aux->instrucciones <= maximoInstrucciones){
						aux->uso = (double)aux->instrucciones*100/maximoInstrucciones;
						printf("CPU:%d ,Instrucciones %d, Maximo Posible %lf\n",aux->core, aux->instrucciones, maximoInstrucciones);
					}
					else{
						aux->uso = 100;
					}
				}
			}
			segundos = 0;
			for(i=0; i<list_size(instruccionesPorCpu); i++){
				aux = list_get(instruccionesPorCpu, i);
				aux->instrucciones = 0;
			}
		}
		else{
			double maximoInstrucciones = segundos/retardoGlobal;
			for(i=0; i<list_size(instruccionesPorCpu); i++){
				aux = list_get(instruccionesPorCpu, i);
				if(maximoInstrucciones !=0){
					if(aux->instrucciones <= maximoInstrucciones){
						aux->uso = (double)aux->instrucciones*100/maximoInstrucciones;

					}
					else{
						aux->uso = 100;
					}
				}
			}	
		}
	}
}



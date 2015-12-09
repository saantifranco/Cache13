#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"
#include "CPUArranque.h"

typedef struct{
	t_log* logs;
	CPUConfig* configCore;
	int idCPU;
	t_list* periodosEjecucion;
}t_conexiones_cores;

t_list* instruccionesPorCpu;
double retardoGlobal;
int ejecucionIniciada;

typedef struct{
	int core;
	int instrucciones;
	double uso;
}t_instruccionesPorCpu;

typedef struct{
	int core; //Numero de CPU
	int porcentajeUso; //Cantidad de segundos que ejecutó en el minuto
	int estado; //0 para ocupada, 1 para libre
	int ultimoPidEjecutado; //Ultimo proceso que ejecuto, para manejar los retornos: 0 nunca ejecutó nada
	//t_list* periodosEjecucion;
}t_cpu;

typedef struct{
	int core;
	char* tiempoInicio;
	char* tiempoFin;
}t_periodo; //Refleja un ciclo de ejecución de una CPU

//**********************************SEMAFOROS CPU**********************************//
pthread_mutex_t mutex_logs;
pthread_mutex_t mutex_idCore;
pthread_mutex_t mutex_Core;
pthread_mutex_t mutex_envioMemoria;

pthread_mutex_t mutex_listaStatus;
//********************************************************************************//

//**********************************FUNCIONES CPU**********************************//

void* ejecucionCPU(void*);
void arranqueCPU(t_paquete_ejecutar*, t_conexiones_cores*, int, int, int,t_list*);
void *funcionThread (void *);
void crearHilo(void *);

void get_tiempo(int*, int*, int*, int*);
void convertir_tiempo(char*, int*, int*, int*, int*);
void* cpu_status(void*);
void* temporizador(void*);
void timer_handler();

int contarTamanioPath(FILE* contextoEjecucion ,t_paquete_ejecutar* paquete);
int lecturaInstruccion(int,char,char*,int,t_log* ,t_paquete_ejecutar* ,t_list*,t_list*);
int obtenerPaginas(char instruccion[],int);

char* obtenerTexto(char instruccion[], int);

void enviarPaginas(int, int, t_paquete_para_memoria*, int);
t_paquete_para_memoria* enviarPaginasYTexto(int protocolo, int paginas, char*,int);



//*********************************************************************************//


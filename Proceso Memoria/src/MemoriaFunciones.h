#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/log.h>
#include <sys/time.h>
#include <signal.h>
#include <commons/temporal.h>
#include <wait.h>
#include <pthread.h>
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"
#include "MemoriaArranque.h"

typedef struct{
	int pagina;
	int marco;
	int tamanio;
	char* contenidoPagina;
	int usada;
	int modificada;
	int vacio;
}contenidoMarco;

typedef struct{
	int pagina;
	int marco;
	int presencia;
	int modificado;
	int instantes;
}estadoPagina;

typedef struct{
	int pid;
	int cantPaginas;
	t_list* marcosAsignados;
	t_list* tablaDePaginas;
	int puntero;
	int fallos;
	int accedidas;
	int accesosSwap;
}mProcMemoria;

typedef struct{
	int ingresosTLB;
	int aciertos;

}t_tlbStatus;

t_tlbStatus* tlbStatus;

typedef struct{
	t_list* memoriaPrincipal;
	t_list* tlb;
	t_log* log;
	t_list* listaProcesos;
	int socketSwap;
	MemoriaConfig* config;
	int punteroMP;
	int listenningSocket;

}datosHandlerSelect;

pthread_mutex_t mutex_tlb;
pthread_mutex_t mutex_memoria;
pthread_mutex_t mutex_logs;

void inicializarMP(t_list*, MemoriaConfig*);
void* handlerSelect(void*);
void ejecutarInstruccion(int, int, t_list*, t_log*, MemoriaConfig*, t_list*, t_list*, t_paquete_envio*, int*);
t_paquete_con_instruccion* transformarParaSwap(t_paquete_para_memoria*);
t_paquete_fin* transformarParaSwapFin(t_paquete_para_memoria*);
void crearEstructurasDeProceso(t_paquete_para_memoria*, MemoriaConfig*, t_list*, t_paquete_resultado_instruccion*, t_log*, t_list*, t_list*, t_paquete_con_pagina*);
void eliminarEstructurasDeProceso(t_paquete_para_memoria*, t_list*, t_list*, t_list*, t_log*);
void limpiarProcesoDeMP(contenidoMarco*, t_list*);
void limpiarProcesoDeTLB(contenidoMarco*, t_list*);
mProcMemoria* buscarYRemoverProceso(t_list*, t_paquete_para_memoria*);
void leerPagina(t_paquete_resultado_instruccion*, t_paquete_resultado_instruccion*, t_log*, t_paquete_para_memoria*, MemoriaConfig*, t_list*, t_list*, t_paquete_con_instruccion*, t_paquete_envio*, int, t_paquete_envio*, t_list*, int*);
void escribirPagina(t_paquete_resultado_instruccion*, t_paquete_resultado_instruccion*, t_log*, t_paquete_para_memoria*, MemoriaConfig*, t_list*, t_list*, t_paquete_con_instruccion*, t_paquete_envio*, int, t_paquete_envio*, t_list*, int*);
void buscarEnMemoria(t_paquete_resultado_instruccion*, t_paquete_resultado_instruccion*, t_log*, t_paquete_para_memoria*, t_list*, t_paquete_con_instruccion*, t_paquete_envio*, int, t_paquete_envio*, MemoriaConfig*, t_list*, t_list*, int, int*);
void buscarEnMemoriaParaEscritura(t_paquete_resultado_instruccion*, t_paquete_resultado_instruccion*, t_log*, t_paquete_para_memoria*, t_list*, t_paquete_con_instruccion*, t_paquete_envio*, int, t_paquete_envio*, MemoriaConfig*, t_list*, t_list*, int, int*);
void cargarMarcoEnTLB(contenidoMarco*, t_list*, MemoriaConfig*, t_list*, int, t_log*);
void cargarMarcoEnMemoria(contenidoMarco*, t_list*, MemoriaConfig*, t_list*, int, t_paquete_para_memoria*, int, t_list*, int*, t_log*);
void reemplazarMarcoTLB(contenidoMarco*, t_list*, MemoriaConfig*, mProcMemoria*, t_list*, t_log*);
void reemplazarMarcoPorMaximos(contenidoMarco*, t_list*, MemoriaConfig*, mProcMemoria*, t_paquete_para_memoria*, int, t_list*, t_list*, t_list*, int*, t_log*);
void reemplazarMarcoPorMaximosTLB(contenidoMarco*, t_list*, contenidoMarco*, MemoriaConfig*, mProcMemoria*, t_log*);
contenidoMarco* aplicarAlgoritmo(MemoriaConfig*, t_list*, int, int*, mProcMemoria*);
void aumentarInstantes(mProcMemoria*, int, int);
contenidoMarco* copiarMarco(contenidoMarco*);
void copiarValoresSinMarco(contenidoMarco*, contenidoMarco*);
void asignarValorModificacion(mProcMemoria*, int, int);
estadoPagina* obtenerMarcoDeProceso(mProcMemoria*, int);
contenidoMarco* buscarElPrimeroEnLaLista(t_list*, t_list*);
void agregarElMarcoAlProceso(mProcMemoria*, int, int);
void sacarMarcoAlProceso(mProcMemoria*, int, int);
contenidoMarco* buscarMarco(t_list*, int);
mProcMemoria* buscarProcesoPorPagina(t_list*, int);
mProcMemoria* buscarProcesoPorPID(t_list* lista, t_paquete_para_memoria* paqueteInstruccion);
contenidoMarco* obtenerMarco(t_paquete_para_memoria*, t_list*, t_paquete_resultado_instruccion*, int);
contenidoMarco* obtenerMarcoYEscribir(t_paquete_para_memoria*, t_list*, int, mProcMemoria*);
void buscarYRemoverMarco(t_list*, contenidoMarco*);
int estaEnLista(int, t_list*);
void limpiarTLB(t_list*, t_list*);
void limpiarMP(t_list*, t_list*, t_list*, int, int);
void loguearMP(t_list*, t_log*);
void funcHiloTLB(void*);
void funcHiloMP(void*);
void crearFork(t_list*, t_log*);
void* temporizador(void*);
void timer_handler();

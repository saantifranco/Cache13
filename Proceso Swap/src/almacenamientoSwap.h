/*
 * almacenamientoSwap.h
 *
 *  Created on: 15/9/2015
 *      Author: utnso
 */

#ifndef ALMACENAMIENTOSWAP_H_
#define ALMACENAMIENTOSWAP_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include<commons/collections/queue.h>
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include "swapArranque.h"

typedef struct{
	int pid;
	int pagInicial;
	int cantPags;
} mProc;

typedef struct{
	int comienzoLibre;
	int pagsContiguas;
} seccionLibre;


t_list* inicializarAlmacenamiento(int,t_list*);
void* atenderConexionesDeMemoria(void* data);
int llegadaDeMProc(mProcNuevo*,t_list*);
int verificarEspacioLibre(mProcNuevo*,t_list*);
int sumarPaginas(t_list*);
int obtenerPagsContiguas(seccionLibre*);
int hayXPaginasContiguas(int, t_list*);
void realizarCompactacion(t_list*,t_list*,swapConfig*, t_log* logs);
int reacomodado(int pid, t_list* reacomodados);
void rechazarmProc(int, t_paquete_resultado_instruccion*);
int escribirEnParticion(swapConfig*,t_paquete_escritura*,t_list*,t_log*);
int buscarByteInicial(int,t_paquete_escritura*,t_list*);
int buscarByteInicial2(int,t_paquete_lectura*,t_list*);
int buscarPaginaInicial(t_paquete_escritura*,t_list*);
int buscarPaginaInicial2(mProcNuevo*,t_list*);
int asignarPaginaInicial(mProcNuevo*,t_list*);
void achicarSeccionLibre(int,int,seccionLibre*,t_list*);
int obtenerPid(mProc*);
char* leerEnParticion(swapConfig*,t_paquete_lectura*,t_list*);
mProc* eliminarmProc(int,t_list*,swapConfig*);
void agregarEspacioLibre(mProc*,t_list*);
void* recibirPedidosMemoria(void*);
void borrarDatosDeParticion(mProc*,swapConfig*);
void agregarMProc(mProcNuevo*,infoHilos*,t_paquete_envio*,t_paquete_con_pagina*);
void* seccionDestroy(seccionLibre*);



#endif /* ALMACENAMIENTOSWAP_H_ */

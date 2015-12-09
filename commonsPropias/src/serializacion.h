/*
 * serializacion.h
 *
 *  Created on: 21/9/2015
 *      Author: utnso
 */

#ifndef SRC_SERIALIZACION_H_
#define SRC_SERIALIZACION_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <commons/collections/list.h>

/********************ESTRUCTURAS********************/
//Estructuras utilizadas para comunicación entre procesos del sistema

typedef struct{
	uint32_t data_size;
	void* data;
}t_paquete_envio;

//INTERACCION PLANIFICADOR - CPU

typedef struct{
	char* path; //Ruta al "mCod"
	int proximaInstruccion; //Puntero a la prox instruccion
	int rafaga; // 0 si no hay desalojo, n cantidad de ráfagas si hay desalojo
	int pid;
}t_paquete_ejecutar;

typedef struct{
	int proximaInstruccion; //Puntero a la prox instruccion
	t_list* retornos; //Lista que contiene un char* por cada log que tenga que hacer el planificador
}t_list_retorno;

//INTERACCION CPU - MEMORIA

typedef struct{
	int instruccion;
	int pagina;
	char* texto;
	int PID;
}t_paquete_para_memoria;

typedef struct{
	char* contenido;
	int error;
}t_paquete_resultado_instruccion;

//INTERACCION MEMORIA - SWAP

typedef struct{
	char* textoAEscribir;
	int pid;
	int numPagina;
} t_paquete_escritura;

typedef struct {
	int pid;
	int cantPaginas;
} mProcNuevo;

typedef struct {
	int pid;
	int numPagina;
}t_paquete_lectura;

typedef struct {
	int instruccion;
	int pid;
	int cantPaginas;
} t_paquete_con_instruccion;

typedef struct {
	int instruccion;
	int pid;
}t_paquete_fin;

typedef struct {
	char* contenido;
	int error;
	int pagInicial;
} t_paquete_con_pagina;

/********************Funciones********************/
t_paquete_envio* serializar_string(char*);
char* deserializar_string(t_paquete_envio*);

//INTERACCION PLANIFICADOR - CPU

t_paquete_envio* serializar_t_paquete_ejecutar(t_paquete_ejecutar*);
t_paquete_ejecutar* deserializar_t_paquete_ejecutar(t_paquete_envio*);
t_paquete_envio* serializar_t_list_retorno(t_list_retorno*);
t_list_retorno* deserializar_t_list_retorno(t_paquete_envio*);
t_paquete_envio* serializar_cpu_status(t_list* retorno);
t_list* deserializar_cpu_status(t_paquete_envio* paqueteEnvio);

//INTERACCION CPU - MEMORIA

t_paquete_envio* envioA_Memoria_Serializer(t_paquete_para_memoria*);
t_paquete_resultado_instruccion* deserializar_t_paquete_resultado_instruccion(t_paquete_envio*);
t_paquete_envio* envioA_CPU_Serializer(t_paquete_resultado_instruccion* envioCPU);
t_paquete_para_memoria* deserializar_t_paquete_de_CPU_ejecutar(t_paquete_envio* paqueteEnvio);

//INTERACCION MEMORIA - SWAP

t_paquete_envio* serializarPorEscritura(t_paquete_para_memoria* escrituraSwap);
t_paquete_envio* serializarConInstruccion(t_paquete_con_instruccion* paqueteSwap);
t_paquete_envio* serializarPorFin(t_paquete_fin* finSwap);
t_paquete_envio* serializarConPagina(t_paquete_con_pagina*);

int deserializar_t_paquete_swap(t_paquete_envio*);
mProcNuevo* deserializarPorInicio(t_paquete_envio*);
t_paquete_escritura* deserializarPorEscritura(t_paquete_envio*);
t_paquete_lectura* deserializarPorLectura(t_paquete_envio*);
int deserializarPorFin(t_paquete_envio*);
t_paquete_con_pagina* deserializarInicioConPaginas(t_paquete_envio* paqueteEnvio);

#endif /* SRC_SERIALIZACION_H_ */

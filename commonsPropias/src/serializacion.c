/*
 * serializacion.c
 *
 *  Created on: 21/9/2015
 *      Author: utnso
 */

#include "serializacion.h"

t_paquete_envio* serializar_string(char* string){
	int tamanioDatos = sizeof(int)*2 + strlen(string)*sizeof(char);
	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data
	size_arg=tamanioDatos-sizeof(int);;
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//tamaño string
	size_arg=strlen(string);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//path efectivo
	memcpy(stream+offset, string, sizeof(char)*size_arg);
	aux=sizeof(char)*size_arg;
	offset+=aux;

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;
	return paqueteEnvio;
}

char* deserializar_string(t_paquete_envio* paqueteEnvio){
	int offset = 0;
	int aux=0;
	int size_arg = 0;

	//tamaño path
	memcpy(&size_arg, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;

	//path efectivo
	char* string = (char*) malloc(sizeof(char)*(size_arg+1));
	memcpy(string, (paqueteEnvio->data)+offset, sizeof(char)*size_arg);
	string[size_arg]='\0';
	aux=(sizeof(char)*size_arg);
	offset+=aux;

	return string;
}

/**********Serializacion Planificador - CPU***********/

t_paquete_envio* serializar_t_paquete_ejecutar(t_paquete_ejecutar* paquete){
	int tamanioDatos = sizeof(int) + strlen(paquete->path)*sizeof(char) + sizeof(int) + sizeof(paquete->pid) + sizeof(paquete->proximaInstruccion) + sizeof(paquete->rafaga);
	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);;
	//printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//tamaño path
	size_arg=strlen(paquete->path);
	//printf("TAM PATH (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//path efectivo
	memcpy(stream+offset, paquete->path, sizeof(char)*size_arg);
	aux=sizeof(char)*size_arg;
	offset+=aux;

	memcpy(stream+offset, &paquete->pid, sizeof(paquete->pid));
	aux = sizeof(paquete->pid);
	offset+=aux;

	memcpy(stream+offset, &paquete->proximaInstruccion, sizeof(paquete->proximaInstruccion));
	aux = sizeof(paquete->proximaInstruccion);
	offset+=aux;

	memcpy(stream+offset, &paquete->rafaga, sizeof(paquete->rafaga));
	aux = sizeof(paquete->rafaga);
	offset+=aux;

//	printf("Longitud serializada (%d)\n", offset);

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;

	return paqueteEnvio;
}

t_paquete_ejecutar* deserializar_t_paquete_ejecutar(t_paquete_envio* paqueteEnvio){
	int offset = 0;
	int aux = 0;
	int size_arg = 0;
	t_paquete_ejecutar* paquete = (t_paquete_ejecutar*) malloc(sizeof(t_paquete_ejecutar));

	//tamaño path
	memcpy(&size_arg, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;
//	printf("Longitud de la siguiente cadena a deserializar (%d)\n", size_arg);

	//path efectivo
	paquete->path = (char*) malloc(sizeof(char)*(size_arg+1));
	memcpy(paquete->path, (paqueteEnvio->data)+offset, sizeof(char)*size_arg);
	paquete->path[size_arg]='\0';
	aux=(sizeof(char)*size_arg);
	offset+=aux;

	memcpy(&paquete->pid, (paqueteEnvio->data)+offset, sizeof(paquete->pid));
	aux = sizeof(paquete->pid);
	offset+=aux;

	memcpy(&paquete->proximaInstruccion, (paqueteEnvio->data)+offset, sizeof(paquete->proximaInstruccion));
	aux = sizeof(paquete->proximaInstruccion);
	offset+=aux;

	memcpy(&paquete->rafaga, (paqueteEnvio->data)+offset, sizeof(paquete->rafaga));
	aux = sizeof(paquete->rafaga);
	offset+=aux;

//	printf("Path (%s)\n", paquete->path);
//	printf("Pid (%d)\n", paquete->pid);
//	printf("Prox inst (%d)\n", paquete->proximaInstruccion);
//	printf("Rafaga (%d)\n", paquete->rafaga);

	return paquete;
}

t_paquete_envio* serializar_t_list_retorno(t_list_retorno* retorno){
	int cantidadInstrucciones = list_size(retorno->retornos);
	int tamanioDatos = sizeof(int)*3 + sizeof(int)*cantidadInstrucciones; // Tamaño del header principal + tamaño de los headers de cada instruccion
	int largoAux = 0;
	int contador = 0;
	char* unRetorno;
//	printf("El retorno tiene (%d) instrucciones\n", cantidadInstrucciones);
	while(contador < cantidadInstrucciones){ //Calculo el largo de cada instruccion
		unRetorno = (char*)list_get(retorno->retornos, contador);
		largoAux = strlen(unRetorno);
//		printf("El log tiene (%d) caracteres\n", largoAux);
		contador++;
		tamanioDatos += largoAux; //Incremento el tamanioDatos en una cantidad igual al largo char* que representa una instruccion
	}

	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
	//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);;
//	printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	memcpy(stream+offset, &retorno->proximaInstruccion, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//cantidad de instrucciones que va a tener se van a tener que deserializar
	memcpy(stream+offset, &cantidadInstrucciones, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	contador = 0;
	while(contador < cantidadInstrucciones){
		unRetorno = (char*) list_get(retorno->retornos, contador);
//		printf("UN LOG (%s)\n", unRetorno);
		//tamaño path
		size_arg=strlen(unRetorno);
//		printf("TAM PATH (%d)\n", size_arg);
		memcpy(stream+offset, &size_arg, sizeof(int));
		aux = sizeof(int);
		offset+=aux;

		//path efectivo
		memcpy(stream+offset, unRetorno, sizeof(char)*size_arg);
		aux=sizeof(char)*size_arg;
		offset+=aux;
		contador++;
	}

//	printf("Longitud serializada (%d)\n", offset);

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;

	return paqueteEnvio;
}

t_list_retorno* deserializar_t_list_retorno(t_paquete_envio* paqueteEnvio){
	int offset = 0;
	int aux = 0;
	int size_arg = 0;
	int contador = 0;
	int cantidadInstrucciones = 0;
	char* unRetorno;
	t_list_retorno* paquete = (t_list_retorno*) malloc(sizeof(t_list_retorno));

	memcpy(&paquete->proximaInstruccion, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;
//	printf("La prox instruccion del proc será (%d)\n", paquete->proximaInstruccion);

	//cantidad de instrucciones que va a tener se van a tener que deserializar
	memcpy(&cantidadInstrucciones, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;
	paquete->retornos = list_create(); //Reservo memoria para tener tantos char* como instrucciones tenga
//	printf("Cantidad de instrucciones a deserlizar para ser logueadas (%d)\n", cantidadInstrucciones);

	while(contador < cantidadInstrucciones){

		//tamaño instruccion a loguear
		memcpy(&size_arg, (paqueteEnvio->data)+offset, sizeof(int));
		aux=sizeof(int);
		offset+=aux;
//		printf("Tamaño proximo log(%d)\n", size_arg);

		unRetorno = (char*) malloc(sizeof(char)*(size_arg+1));
		memcpy(unRetorno, (paqueteEnvio->data)+offset, sizeof(char)*size_arg);
		unRetorno[size_arg]='\0';
		aux=(sizeof(char)*size_arg);
		offset+=aux;
		list_add(paquete->retornos, (void*) unRetorno);
		contador++;
	}

	return paquete;
}

t_paquete_envio* serializar_cpu_status(t_list* retorno){
	int cantidadInstrucciones = list_size(retorno);
	int tamanioDatos = sizeof(int)*2 + sizeof(int)*cantidadInstrucciones; // Tamaño del header principal + tamaño de los headers de cada instruccion
	int largoAux = 0;
	int contador = 0;
	char* unRetorno;
//	printf("El retorno tiene (%d) instrucciones\n", cantidadInstrucciones);
	while(contador < cantidadInstrucciones){ //Calculo el largo de cada instruccion
		unRetorno = (char*)list_get(retorno, contador);
		largoAux = strlen(unRetorno);
//		printf("El log tiene (%d) caracteres\n", largoAux);
		contador++;
		tamanioDatos += largoAux; //Incremento el tamanioDatos en una cantidad igual al largo char* que representa una instruccion
	}

	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
	//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);;
//	printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//cantidad de instrucciones que va a tener se van a tener que deserializar
	memcpy(stream+offset, &cantidadInstrucciones, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	contador = 0;
	while(contador < cantidadInstrucciones){
		unRetorno = (char*) list_get(retorno, contador);
//		printf("UN LOG (%s)\n", unRetorno);
		//tamaño path
		size_arg=strlen(unRetorno);
//		printf("TAM PATH (%d)\n", size_arg);
		memcpy(stream+offset, &size_arg, sizeof(int));
		aux = sizeof(int);
		offset+=aux;

		//path efectivo
		memcpy(stream+offset, unRetorno, sizeof(char)*size_arg);
		aux=sizeof(char)*size_arg;
		offset+=aux;
		contador++;
	}

//	printf("Longitud serializada (%d)\n", offset);

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;

	return paqueteEnvio;
}

t_list* deserializar_cpu_status(t_paquete_envio* paqueteEnvio){
	int offset = 0;
	int aux = 0;
	int size_arg = 0;
	int contador = 0;
	int cantidadInstrucciones = 0;
	char* unRetorno;

	//cantidad de instrucciones que va a tener se van a tener que deserializar
	memcpy(&cantidadInstrucciones, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;
	t_list* retornos = list_create(); //Reservo memoria para tener tantos char* como instrucciones tenga
//	printf("Cantidad de instrucciones a deserlizar para ser logueadas (%d)\n", cantidadInstrucciones);

	while(contador < cantidadInstrucciones){

		//tamaño instruccion a loguear
		memcpy(&size_arg, (paqueteEnvio->data)+offset, sizeof(int));
		aux=sizeof(int);
		offset+=aux;
//		printf("Tamaño proximo log(%d)\n", size_arg);

		unRetorno = (char*) malloc(sizeof(char)*(size_arg+1));
		memcpy(unRetorno, (paqueteEnvio->data)+offset, sizeof(char)*size_arg);
		unRetorno[size_arg]='\0';
		aux=(sizeof(char)*size_arg);
		offset+=aux;
		list_add(retornos, (void*) unRetorno);
		contador++;
	}

	return retornos;
}

/**********Serialización CPU - Memoria************/

t_paquete_envio* envioA_Memoria_Serializer(t_paquete_para_memoria* envioMemoria){

	int tamanioDatos = sizeof(int) + sizeof(envioMemoria->instruccion) + sizeof(envioMemoria->pagina) + sizeof(int) + strlen(envioMemoria->texto) + sizeof(envioMemoria->PID);
	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);;
//	printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	memcpy(stream+offset, &envioMemoria->instruccion, sizeof(envioMemoria->instruccion));
	aux = sizeof(envioMemoria->instruccion);
	offset+=aux;

	memcpy(stream+offset, &envioMemoria->pagina, sizeof(envioMemoria->pagina));
	aux = sizeof(envioMemoria->pagina);
	offset+=aux;

	//tamaño texto
	size_arg=strlen(envioMemoria->texto);
//	printf("TAM TEXTO (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//texto efectivo
	memcpy(stream+offset, envioMemoria->texto, sizeof(char)*size_arg);
	aux=sizeof(char)*size_arg;
	offset+=aux;


	memcpy(stream+offset, &envioMemoria->PID, sizeof(envioMemoria->PID));
	aux = sizeof(envioMemoria->PID);
	offset+=aux;

//	printf("Longitud serializada (%d)\n", offset);

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;

	return paqueteEnvio;
}

t_paquete_resultado_instruccion* deserializar_t_paquete_resultado_instruccion(t_paquete_envio* paqueteEnvio){

	int offset =0 ;
	int aux=0;
	int size_arg = 0;
	t_paquete_resultado_instruccion* paquete = (t_paquete_resultado_instruccion*) malloc(sizeof(t_paquete_resultado_instruccion));

	//tamaño path
	memcpy(&size_arg, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;
//	printf("Longitud de la siguiente cadena a deserializar (%d)\n", size_arg);

	//path efectivo
	paquete->contenido = (char*) malloc(sizeof(char)*size_arg+1);
	memcpy(paquete->contenido, (paqueteEnvio->data)+offset, sizeof(char)*size_arg);
	paquete->contenido[size_arg]='\0';
	aux=(sizeof(char)*size_arg);
	offset+=aux;

	memcpy(&paquete->error, (paqueteEnvio->data)+offset, sizeof(paquete->error));
	aux = sizeof(paquete->error);
	offset+=aux;

//	printf("%s \n", paquete->contenido);
//	printf("%d \n", paquete->error);

	return paquete;
}

t_paquete_envio* envioA_CPU_Serializer(t_paquete_resultado_instruccion* envioCPU){
	int tamanioDatos = sizeof(int) + sizeof(int) + strlen(envioCPU->contenido)*sizeof(char) + sizeof(int);
	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);;
//	printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//tamaño contenido
	size_arg=strlen(envioCPU->contenido);
//	printf("TAM CONTENIDO (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//contenido efectivo
	memcpy(stream+offset, envioCPU->contenido, sizeof(char)*size_arg);
	aux=sizeof(char)*size_arg;
	offset+=aux;

	memcpy(stream+offset, &envioCPU->error, sizeof(envioCPU->error));
	aux = sizeof(envioCPU->error);
	offset+=aux;

//	printf("%s \n",envioCPU->contenido);
//	printf("%d \n", envioCPU->error);

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;

	return paqueteEnvio;
}

t_paquete_para_memoria* deserializar_t_paquete_de_CPU_ejecutar(t_paquete_envio* paqueteEnvio){
	int offset = 0;
	int aux=0;
	int size_arg = 0;
	t_paquete_para_memoria* paquete = (t_paquete_para_memoria*) malloc(sizeof(t_paquete_para_memoria));

	memcpy(&paquete->instruccion, (paqueteEnvio->data)+offset, sizeof(paquete->instruccion));
	aux = sizeof(paquete->instruccion);
	offset+=aux;

	memcpy(&paquete->pagina, (paqueteEnvio->data)+offset, sizeof(paquete->pagina));
	aux = sizeof(paquete->pagina);
	offset+=aux;

	//tamaño proximo char*
	memcpy(&size_arg, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;
//	printf("Longitud de la siguiente cadena a deserializar (%d)\n", size_arg);

	//texto
	paquete->texto = (char*) malloc(sizeof(char)*size_arg+1);
	memcpy(paquete->texto, (paqueteEnvio->data)+offset, sizeof(char)*size_arg);
	paquete->texto[size_arg]='\0';
	aux=(sizeof(char)*size_arg);
	offset+=aux;

	memcpy(&paquete->PID, (paqueteEnvio->data)+offset, sizeof(paquete->PID));
	aux = sizeof(paquete->PID);
	offset+=aux;

//	printf("Instruccion (%d)\n", paquete->instruccion);
//	printf("Pagina (%d)\n", paquete->pagina);
//	printf("Texto (%s)\n", paquete->texto);
//	printf("PID (%d)\n", paquete->PID);

	return paquete;
}
//*********SERIALIZACIÓN MEMORIA - SWAP ******************

t_paquete_envio* serializarPorEscritura(t_paquete_para_memoria* escrituraSwap){
	int tamanioDatos = sizeof(int) + sizeof(int) + strlen(escrituraSwap->texto)*sizeof(char) + sizeof(escrituraSwap->instruccion) + sizeof(escrituraSwap->PID) + sizeof(escrituraSwap->pagina);
	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);;
//	printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	memcpy(stream+offset, &escrituraSwap->instruccion, sizeof(escrituraSwap->instruccion));
	aux = sizeof(escrituraSwap->instruccion);
	offset+=aux;

//	printf("PAGINA: %d\n",escrituraSwap->pagina);
	memcpy(stream+offset, &escrituraSwap->pagina, sizeof(escrituraSwap->pagina));
	aux = sizeof(escrituraSwap->pagina);
	offset+=aux;

	//tamaño contenido
	size_arg=strlen(escrituraSwap->texto);
//	printf("TAM TEXTO (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//contenido efectivo
//	printf("TEXTO: %s\n",escrituraSwap->texto);
	memcpy(stream+offset, escrituraSwap->texto, sizeof(char)*size_arg);
	aux=sizeof(char)*size_arg;
	offset+=aux;

//	printf("PID: %d\n",escrituraSwap->PID);
	memcpy(stream+offset, &escrituraSwap->PID, sizeof(escrituraSwap->PID));
	aux = sizeof(escrituraSwap->PID);
	offset+=aux;

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;


	return paqueteEnvio;
}

t_paquete_envio* serializarConInstruccion(t_paquete_con_instruccion* paqueteSwap){
	int tamanioDatos = sizeof(int) + sizeof(paqueteSwap->instruccion) + sizeof(paqueteSwap->pid) + sizeof(paqueteSwap->cantPaginas);
	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);
//	printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

//	printf("Instruccion: %d \n", paqueteSwap->instruccion);
	memcpy(stream+offset, &paqueteSwap->instruccion, sizeof(paqueteSwap->instruccion));
	aux = sizeof(paqueteSwap->instruccion);
	offset+=aux;

//	printf("PID: %d \n",paqueteSwap->pid);
	memcpy(stream+offset, &paqueteSwap->pid, sizeof(paqueteSwap->pid));
	aux = sizeof(paqueteSwap->pid);
	offset+=aux;

//	printf("Cantidad Paginas/Pagina %d \n",paqueteSwap->cantPaginas);
	memcpy(stream+offset, &paqueteSwap->cantPaginas, sizeof(paqueteSwap->cantPaginas));
	aux = sizeof(paqueteSwap->cantPaginas);
	offset+=aux;

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;

	return paqueteEnvio;
}

t_paquete_envio* serializarPorFin(t_paquete_fin* finSwap){
	int tamanioDatos = sizeof(int) + sizeof(finSwap->instruccion) + sizeof(finSwap->pid);
	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);;
//	printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	memcpy(stream+offset, &finSwap->instruccion, sizeof(finSwap->instruccion));
	aux = sizeof(finSwap->instruccion);
	offset+=aux;

	memcpy(stream+offset, &finSwap->pid, sizeof(finSwap->pid));
	aux = sizeof(finSwap->pid);
	offset+=aux;

//	printf("MANDE UNA INSTRUCCION DE FIN\n");

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;

	return paqueteEnvio;
}


//***************DESERIALIZACION EN SWAP***************

int deserializar_t_paquete_swap (t_paquete_envio* paqueteEnvio){
	int deserializador=-1;

	memcpy(&deserializador, (paqueteEnvio->data), sizeof(int));

	printf("Inicio:0/Lectura:1/Escritura:2/Fin:3/Error:-1 ---> (%d)\n", deserializador);

	if(deserializador >=0 && deserializador <=3)
		return deserializador;
	else return -1;
}

t_paquete_escritura* deserializarPorEscritura(t_paquete_envio* paqueteEnvio){

	int offset = sizeof(int);
	int aux= offset;
	int size_arg = 0;

	t_paquete_escritura* paquete = (t_paquete_escritura*)malloc(sizeof(t_paquete_escritura));


	memcpy(&paquete->numPagina, (paqueteEnvio->data)+offset, sizeof(paquete->numPagina));
		aux = sizeof(paquete->numPagina);
		offset+=aux;

	//tamaño path
	memcpy(&size_arg, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;
//	printf("Longitud de la siguiente cadena a deserializar (%d)\n", size_arg);

    //path efectivo
	paquete->textoAEscribir = (char*) malloc(sizeof(char)*size_arg+1);
	memcpy(paquete->textoAEscribir, (paqueteEnvio->data)+offset, sizeof(char)*size_arg);
	paquete->textoAEscribir[size_arg]='\0';
	aux=(sizeof(char)*size_arg);
	offset+=aux;

	memcpy(&paquete->pid, (paqueteEnvio->data)+offset, sizeof(paquete->pid));
				aux = sizeof(paquete->pid);
				offset+=aux;

	return paquete;
}

mProcNuevo* deserializarPorInicio(t_paquete_envio* paqueteEnvio){

		int aux = sizeof(int);
	    int offset = aux;

		mProcNuevo* mProc = (mProcNuevo*) malloc(sizeof(mProcNuevo));

		memcpy(&mProc->pid, (paqueteEnvio->data)+offset, sizeof(mProc->pid));
			aux = sizeof(mProc->pid);
			offset+=aux;

		memcpy(&mProc->cantPaginas, (paqueteEnvio->data)+offset, sizeof(mProc->cantPaginas));
				aux = sizeof(mProc->cantPaginas);
				offset+=aux;

				return mProc;

}

t_paquete_lectura* deserializarPorLectura(t_paquete_envio* paqueteEnvio){

	       int offset = sizeof(int);
		   int aux= offset;

			t_paquete_lectura* paquete = (t_paquete_lectura*) malloc(sizeof(t_paquete_lectura));

			memcpy(&paquete->pid, (paqueteEnvio->data)+offset, sizeof(paquete->pid));
				aux = sizeof(paquete->pid);
				offset+=aux;

			memcpy(&paquete->numPagina, (paqueteEnvio->data)+offset, sizeof(paquete->numPagina));
					aux = sizeof(paquete->numPagina);
					offset+=aux;

					return paquete;

}

int deserializarPorFin(t_paquete_envio* paqueteEnvio){

	int offset=sizeof(int);
	int pid;

	memcpy(&pid, (paqueteEnvio->data)+offset, sizeof(pid));
	return pid;

}

t_paquete_envio* serializarConPagina(t_paquete_con_pagina* paquete){
	int tamanioDatos = sizeof(int) + sizeof(int) + strlen(paquete->contenido) + sizeof(paquete->error) + sizeof(paquete->pagInicial);
	t_paquete_envio* paqueteEnvio = (t_paquete_envio*) malloc(sizeof(t_paquete_envio));
	void* stream = malloc(tamanioDatos);
//	printf("Tengo que serializar %d bytes\n", tamanioDatos);
	int offset = 0;
	int aux = 0;
	int size_arg = 0;

	//tamaño total data: pongo al principio de la data, de manera redundante y para uso del socket, la longitud total
	size_arg=tamanioDatos-sizeof(int);;
//	printf("Longitud TOTAL (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//tamaño texto
	size_arg=strlen(paquete->contenido);
//	printf("TAM TEXTO (%d)\n", size_arg);
	memcpy(stream+offset, &size_arg, sizeof(int));
	aux = sizeof(int);
	offset+=aux;

	//texto efectivo
	memcpy(stream+offset, paquete->contenido, sizeof(char)*size_arg);
	aux=sizeof(char)*size_arg;
	offset+=aux;

	memcpy(stream+offset, &paquete->error, sizeof(paquete->error));
	aux = sizeof(paquete->error);
	offset+=aux;

	memcpy(stream+offset, &paquete->pagInicial, sizeof(paquete->pagInicial));
	aux = sizeof(paquete->pagInicial);
	offset+=aux;

//	printf("Longitud serializada (%d)\n", offset);

	paqueteEnvio->data = stream;
	paqueteEnvio->data_size = offset;

	return paqueteEnvio;
}

t_paquete_con_pagina* deserializarInicioConPaginas(t_paquete_envio* paqueteEnvio){

	int offset =0 ;
	int aux=0;
	int size_arg = 0;
	t_paquete_con_pagina* paquete = (t_paquete_con_pagina*) malloc(sizeof(t_paquete_con_pagina));

	//tamaño path
	memcpy(&size_arg, (paqueteEnvio->data)+offset, sizeof(int));
	aux=sizeof(int);
	offset+=aux;
//	printf("Longitud de la siguiente cadena a deserializar (%d)\n", size_arg);

	//path efectivo
	paquete->contenido = (char*) malloc(sizeof(char)*size_arg+1);
	memcpy(paquete->contenido, (paqueteEnvio->data)+offset, sizeof(char)*size_arg);
	paquete->contenido[size_arg]='\0';
	aux=(sizeof(char)*size_arg);
	offset+=aux;

	memcpy(&paquete->error, (paqueteEnvio->data)+offset, sizeof(paquete->error));
	aux = sizeof(paquete->error);
	offset+=aux;

	memcpy(&paquete->pagInicial, (paqueteEnvio->data)+offset, sizeof(paquete->pagInicial));
	aux = sizeof(paquete->pagInicial);
	offset+=aux;

//	printf("%s \n", paquete->contenido);
//	printf("%d \n", paquete->error);
//	printf("%d \n", paquete->pagInicial);

	return paquete;
}

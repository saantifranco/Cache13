/*
 * almacenamientoSwap.c
 *
 *  Created on: 15/9/2015
 *      Author: utnso
 */

#include "almacenamientoSwap.h"
//FUNCION PRINCIPAL QUE MANEJA LOS PEDIDOS DE MEMORIA


t_list* inicializarAlmacenamiento(int cantPaginas,t_list* espacioLibre){

	seccionLibre* seccionNueva = malloc(sizeof(seccionLibre));
	seccionNueva->comienzoLibre=0;
	seccionNueva->pagsContiguas=cantPaginas;
	list_add(espacioLibre,seccionNueva);

	return espacioLibre;
}

void* atenderConexionesDeMemoria(void* data){//FUNCION IMPORTANTE.
	    infoHilos* info = (infoHilos*) data;

	    //Inicialización de contadores para saber cuantas paginas se escribieron/leyeron por mProc
        int pagsLeidas[200];
        int pagsEscritas[200];
        int i=0;

        for(i=0;i<200;i++){
        	pagsLeidas[i]=0;
        	pagsEscritas[i]=0;
        }


        t_paquete_con_pagina* envioMemoria = (t_paquete_con_pagina*) malloc(sizeof(t_paquete_con_pagina));
        t_paquete_resultado_instruccion* envioMemoria2 = (t_paquete_resultado_instruccion*) malloc(sizeof(t_paquete_resultado_instruccion));
        t_paquete_envio* comando;
		while(1)
		{
		    int instruccion=-1;
		    if(!queue_is_empty(info->cola)){
		    pthread_mutex_lock(&mutex);
		    comando = (t_paquete_envio*) queue_pop(info->cola);
		    pthread_mutex_unlock(&mutex);
            instruccion = deserializar_t_paquete_swap(comando);
		    }

			//VALIDACIONES PARA SABER POR DONDE DESERIALIZAR

		    if(instruccion!=-1){
            	if(instruccion==0){

            	int verifica;
            	mProcNuevo* mProc2= deserializarPorInicio(comando);
            	verifica= llegadaDeMProc(mProc2,info->listaLibres);

            	if(verifica==2){ //LLAMA DE NUEVO DESPUES DE REALIZAR COMPACTACION
            		log_info(info->logs,"Compactación iniciada por fragmentación externa. \n");
            		realizarCompactacion(info->listaLibres,info->listaOcupados,info->config, info->logs);
            		usleep(info->config->RETARDO_COMPACTACION*1000000);
            		log_info(info->logs,"Compactacion finalizada. \n");
            		agregarMProc(mProc2,info,comando,envioMemoria);
            	}
            	else if(verifica==1) {  //PUNTO DE ENTRADA PARA EL INICIO DE UN MPROC.
            		agregarMProc(mProc2,info,comando,envioMemoria);

            }
            	else if(verifica==0){
            		rechazarmProc(info->socket, envioMemoria2);
            		log_info(info->logs,"No se puede asignar espacio al mProc con pid %d por exceso de bytes. \n",mProc2->pid);
            	}
                    free(mProc2);
           	}

           else if(instruccion==1){ //PUNTO DE ENTRADA PARA LEER EN LA PARTICION.
        	    char* data;
        	    t_paquete_lectura* paqueteLectura = deserializarPorLectura(comando);
        	    data =leerEnParticion(info->config,paqueteLectura,info->listaOcupados);
        	    log_info(info->logs,"Lectura solicitada en particion --> PID: %d , Byte inicial: %d , Tamanio: %d, Contenido: %s \n",paqueteLectura->pid,buscarByteInicial2(info->config->TAMANIO_PAGINA,paqueteLectura,info->listaOcupados),strlen(data),data);
        	    pagsLeidas[paqueteLectura->pid]++;
        	    usleep(info->config->RETARDO_SWAP*1000000);
        	    free(paqueteLectura);
        	    if(data!=0){
        	    	envioMemoria2->contenido=string_duplicate(data);
        	    	envioMemoria2->error=0;
        	    	comando = envioA_CPU_Serializer(envioMemoria2);
        	    	SendAll(info->socket,comando);
        	    	free(data);

        	    }
        	    else {
        	    	envioMemoria2->contenido="nada";
        	    	envioMemoria2->error=-1;
        	    	comando = envioA_CPU_Serializer(envioMemoria2);
        	    	SendAll(info->socket, comando);
        	    	printf("No pude leer en la particion \n");
        	    }


           }
           else if(instruccion==2){  //PUNTO DE ENTRADA PARA ESCRIBIR EN LA PARTICION.
//        	            printf("1 Llegue hasta aca\n");
                       	t_paquete_escritura* paqueteEscritura = deserializarPorEscritura(comando);
                       	log_info(info->logs,"Escritura solicitada en particion --> PID: %d , Byte inicial: %d , Tamanio: %d, Contenido: %s \n",paqueteEscritura->pid,buscarByteInicial(info->config->TAMANIO_PAGINA,paqueteEscritura,info->listaOcupados),strlen(paqueteEscritura->textoAEscribir),paqueteEscritura->textoAEscribir);
                        int escrito=escribirEnParticion(info->config,paqueteEscritura,info->listaOcupados, info->logs);
                        if(escrito==1)
                        	pagsEscritas[paqueteEscritura->pid]++;
                       	usleep(info->config->RETARDO_SWAP*1000000);
                        free(paqueteEscritura);

          }
           else if(instruccion==3) { //PUNTO DE ENTRADA PARA FINALIZAR UN MPROC

        	  int pidFinal = deserializarPorFin(comando);
        	  mProc* mProcALiberar = eliminarmProc(pidFinal,info->listaOcupados,info->config);
        	  agregarEspacioLibre(mProcALiberar,info->listaLibres);
        	  printf("mProc pid: %d\n",mProcALiberar->pid);
         	  printf("Paginas leidas del mProc:  %d \n",pagsLeidas[mProcALiberar->pid]);
          	  printf("Paginas escritas del mProc:  %d \n",pagsEscritas[mProcALiberar->pid]);
          	  pagsLeidas[mProcALiberar->pid]=0;
          	  pagsEscritas[mProcALiberar->pid]=0;
              log_info(info->logs,"mProc liberado --> PID: %d ,Byte inicial: %d, Tamanio en bytes: %d \n",mProcALiberar->pid,(mProcALiberar->pagInicial)* (info->config->TAMANIO_PAGINA),(mProcALiberar->cantPags)*(info->config->TAMANIO_PAGINA));

        	   free(mProcALiberar);

           }
		   }


		   }

}


//FUNCION QUE MANEJA LA LLEGADA DE UN MPROC Y DERIVADAS.

int llegadaDeMProc(mProcNuevo* mProc, t_list* espacioLibre){

	if(verificarEspacioLibre(mProc,espacioLibre) && (hayXPaginasContiguas(mProc->cantPaginas, espacioLibre)!=-1)){

	return 1;
	}
	else if(verificarEspacioLibre(mProc,espacioLibre) && (hayXPaginasContiguas(mProc->cantPaginas, espacioLibre)==-1)){

		return 2;
	}

	else

	    return 0;
}

void agregarMProc(mProcNuevo* mProc2,infoHilos* info,t_paquete_envio* comando,t_paquete_con_pagina* envioMemoria){
	mProc* posta= malloc(sizeof(mProc));
	            		posta->pid = mProc2->pid;
	            		posta->pagInicial = asignarPaginaInicial(mProc2,info->listaLibres); //FUNCION QUE ASIGNA PAGINA INICIAL AL MPROC NUEVO Y ADEMAS ACTUALIZA EL ESPACIO LIBRE RESTANTE.
	            		posta->cantPags= mProc2->cantPaginas;
	            		list_add(info->listaOcupados,posta);
	            		log_info(info->logs,"mProc asignado --> PID %d, Byte inicial: %d , Tamanio en bytes %d. \n", posta->pid,posta->pagInicial*info->config->TAMANIO_PAGINA,posta->cantPags*info->config->TAMANIO_PAGINA);

	            		envioMemoria->contenido="nada";
	            		envioMemoria->error=0;
	            	    envioMemoria->pagInicial = posta->pagInicial;
	            		comando = serializarConPagina(envioMemoria);
	            		SendAll(info->socket,comando);
}


int verificarEspacioLibre(mProcNuevo* mProc, t_list* espacioLibre){


    t_list* listaPaginas = list_map(espacioLibre, (void*)obtenerPagsContiguas);

	int totalPaginasLibres = sumarPaginas(listaPaginas);
	printf("TOTAL PAGINAS: %d \n",totalPaginasLibres);
    if(totalPaginasLibres >= mProc->cantPaginas){
    	return 1;
    }
    else return 0;
}

int sumarPaginas(t_list* listaDePaginas){ //SIRVE SOLO PARA LISTAS DE INT
    int totalPaginas = 0;
    int i=0;
	for(i=0;i<list_size(listaDePaginas);i++){
		totalPaginas += (int) list_get(listaDePaginas,i);
	}
	return totalPaginas;
}

int obtenerPagsContiguas(seccionLibre* seccion){
	return seccion->pagsContiguas;

}

int hayXPaginasContiguas(int pagsAEscribir, t_list* espacioLibre){ //DEVUELVE INDICE DEL PRIMER ELEMENTO CON X PAGINAS CONTIGUAS

	t_list* nueva = list_map(espacioLibre, (void*)obtenerPagsContiguas);
	 int i=0;

	 for(i=0;i<list_size(nueva);i++){
     if ((int)list_get(nueva,i)>=pagsAEscribir)
    	 return i;
}

         return -1;
}

void realizarCompactacion(t_list* espacioLibre, t_list* espacioOcupado, swapConfig* config, t_log* logs){
	seccionLibre* seccionAux;
	mProc* mProcAux;
	mProc* primerProc;
	t_paquete_escritura* paqueteEscrituraAux;
	t_paquete_lectura* paqueteLecturaAux;
	int i = 0;
	int j = 0;
	int k = 0;
	int pagInicialEscritura = 0;
	int pagInicialLectura = 0;
	t_list* procesosReacomodados = list_create();

	for(i=0; i<list_size(espacioOcupado); i++){

		//Agarro el primer proceso no reacomodado
		primerProc = (mProc*) list_get(espacioOcupado, i);
		for(j=1; j<list_size(espacioOcupado); j++){
			mProcAux = (mProc*) list_get(espacioOcupado, j);
			if(primerProc->pagInicial > mProcAux->pagInicial && !reacomodado(mProcAux->pid, procesosReacomodados)){
				primerProc = (mProc*) list_get(espacioOcupado, j);
			}
		}

		//Administro el contenido de dicho proceso página a página
		char* contenidoPagina;
		pagInicialLectura = primerProc->pagInicial;
		for(k=0; k<primerProc->cantPags; k++){

			//Lectura
			primerProc->pagInicial = pagInicialLectura;
			paqueteLecturaAux = (t_paquete_lectura*) malloc(sizeof(t_paquete_lectura));
			paqueteLecturaAux->pid = primerProc->pid;
			paqueteLecturaAux->numPagina = k;
			contenidoPagina = leerEnParticion(config, paqueteLecturaAux, espacioOcupado);
			free(paqueteLecturaAux);

			//Escritura
			primerProc->pagInicial = pagInicialEscritura;
            paqueteEscrituraAux = (t_paquete_escritura*) malloc(sizeof(t_paquete_escritura));
			paqueteEscrituraAux->numPagina = k;
			paqueteEscrituraAux->pid = primerProc->pid;
			paqueteEscrituraAux->textoAEscribir = contenidoPagina;
			escribirEnParticion(config, paqueteEscrituraAux, espacioOcupado, logs);
			free(paqueteEscrituraAux);
		}
        log_info(logs,"Nueva primer pagina del pid %d --> %d \n",primerProc->pid,primerProc->pagInicial);
		pagInicialEscritura += primerProc->cantPags;
		list_add(procesosReacomodados, (void*) primerProc->pid);
	}

	//Borrar todos los elementos de espacioLibre
	//printf("El tamaño de la lista de libres es %d \n",list_size(espacioLibre));
	list_clean(espacioLibre);

	//Colocar un elemento en espacio que empiece después del total de páginas escritas
	seccionAux = (seccionLibre*) malloc(sizeof(seccionLibre));
	seccionAux->comienzoLibre = pagInicialEscritura;
	seccionAux->pagsContiguas = config->CANTIDAD_PAGINAS - pagInicialEscritura;
	list_add(espacioLibre,seccionAux);
	log_info(logs,"El comienzo de la seccion libre ahora esta en la pagina %d \n",seccionAux->comienzoLibre);

	//Rellenar lo demás con '\0'
//	char* barraCero = string_substring_until("                                                                                                                                ", config->TAMANIO_PAGINA);
//    FILE* file = fopen(config->NOMBRE_SWAP,"r+");
//    if(file==NULL)
//    		printf("No se pudo abrir la partición para borrar los datos del mproc a eliminar\n");
//    	else{
//
//    		fseek(file,seccionAux->comienzoLibre*config->TAMANIO_PAGINA,SEEK_SET);
//    		for(i=0;i<seccionAux->pagsContiguas;i++)
//    			fputs(barraCero,file);
//    		fflush(file);
//    		fclose(file);
//    	}

	//Remuevo y elimino la lista de reacomodados
	for(i=0; i<list_size(procesosReacomodados); i++){
		list_remove(procesosReacomodados, i);
	}//No se si es realmente necesario remover porque no son punteros entonces ni siquiera hago el free()
	list_destroy(procesosReacomodados);
	return;
}

void* seccionDestroy(seccionLibre* seccion){
	free(seccion);
	return 0;
}

int reacomodado(int pid, t_list* reacomodados){
	int fueReacomodado = 0;
	int i = 0;
	int pidAux;
	for(i=0; i<list_size(reacomodados); i++){
		pidAux = (int) list_get(reacomodados, i);
		if(pidAux == pid){
			fueReacomodado = 1;
			return fueReacomodado;
		}
	}
	return fueReacomodado;
}

void rechazarmProc(int socket, t_paquete_resultado_instruccion* envioMemoria){
	envioMemoria->contenido="nada";
	envioMemoria->error=-1;
    t_paquete_envio* paquete = envioA_CPU_Serializer(envioMemoria);
	SendAll(socket,paquete);
	return;

}

//FUNCIONES PARA EL MANEJO DE MENSAJES QUE RECIBO DE MEMORIA

//****ESCRITURA

int escribirEnParticion(swapConfig* swapConfig, t_paquete_escritura* paquete, t_list*ocupado, t_log* logs){
	log_info(logs,"Voy a escribir (%s) en la pagina (%d) del pid (%d)\n", paquete->textoAEscribir, paquete->numPagina, paquete->pid);
	int byteInicial,clave;
	FILE* file = fopen(swapConfig->NOMBRE_SWAP,"r+");
	if(file==NULL)
		printf("No se pudo abrir la partición para escritura\n");
	else{
		byteInicial = buscarByteInicial(swapConfig->TAMANIO_PAGINA,paquete,ocupado);
//		printf("Byte inicial escritura: %d\n", byteInicial);
		if(fseek(file,byteInicial,SEEK_SET)==0){
		if(strlen(paquete->textoAEscribir)<swapConfig->TAMANIO_PAGINA){
		clave=fputs(paquete->textoAEscribir,file);
		fflush(file);
		fclose(file);
		//log_info(logs,"Texto escrito con exito en la particion. --> %s \n",paquete->textoAEscribir);
		return 1;
		}
	    else log_info(logs,"No se puede escribir en la particion ya que el texto excede %d bytes el tamaño de la pagina",strlen(paquete->textoAEscribir)+1-swapConfig->TAMANIO_PAGINA);
		}
		else log_info(logs,"No se pudo posicionar el puntero en donde correspondia. \n");
		fclose(file);
	}
	return 0;
}

int buscarByteInicial(int tamanio,t_paquete_escritura* paquete, t_list*ocupado){

	int i=0;
	for(i=0;i<list_size(ocupado);i++){
	mProc* mProc = list_get(ocupado,i);
	if(mProc->pid==paquete->pid)
	return (mProc->pagInicial+ paquete->numPagina)*tamanio;
	}
    return -1;
}

int buscarByteInicial2(int tamanio,t_paquete_lectura* paquete, t_list*ocupado){

	int i=0;
	for(i=0;i<list_size(ocupado);i++){
		mProc* mProc = list_get(ocupado,i);
		if(mProc->pid==paquete->pid)
			return (mProc->pagInicial + paquete->numPagina)*tamanio;
	}
    return -1;
}


int buscarPaginaInicial(t_paquete_escritura*paquete,t_list* ocupado){
	int i=0;
	for(i=0;i<list_size(ocupado);i++){
	mProc* mProc = list_get(ocupado,i);
	if(mProc->pid==paquete->pid)
	return mProc->pagInicial;
	}
    return -1;
}

int buscarPaginaInicial2(mProcNuevo* inicial,t_list*ocupado){
	int i=0;
	for(i=0;i<list_size(ocupado);i++){
	mProc* mProc = list_get(ocupado,i);
	if(mProc->pid==inicial->pid)
	return mProc->pagInicial;
	}
    return -1;

}

//CREACION DE MPROC NUEVO
int asignarPaginaInicial(mProcNuevo* nuevo,t_list* libre){
     int indice = hayXPaginasContiguas(nuevo->cantPaginas,libre);
     seccionLibre* seccion= list_get(libre,indice);
     int pagInicial = seccion->comienzoLibre;
     achicarSeccionLibre(indice,nuevo->cantPaginas,seccion,libre);
     return pagInicial;


}

void achicarSeccionLibre(int indice,int cantidad,seccionLibre* seccion,t_list* libre){ //SIRVE PARA QUITAR ESPACIO LIBRE AL CREARSE UN NUEVO MPROC.
	if(seccion->pagsContiguas!= cantidad){
    seccion->comienzoLibre = seccion->comienzoLibre +cantidad;
    seccion->pagsContiguas = seccion->pagsContiguas - cantidad;
	}
	else list_remove(libre,indice);

}

int obtenerPid(mProc* mProc){
	return mProc->pid;
}

//**LECTURA

char* leerEnParticion(swapConfig* swapConfig, t_paquete_lectura* paquete, t_list* ocupado){
	printf("Voy a leer la pagina (%d) del pid (%d)\n", paquete->numPagina, paquete->pid);
	int byteInicial=0;
	char* data = malloc(swapConfig->TAMANIO_PAGINA);
	FILE* file = fopen(swapConfig->NOMBRE_SWAP,"r+");
	if(file==NULL)
		printf("No se pudo abrir la partición para lectura\n");
	else{
		byteInicial = buscarByteInicial2(swapConfig->TAMANIO_PAGINA,paquete,ocupado);
//		printf("Byte inicial lectura: %d\n", byteInicial);
		//int i = 0;
		//while(fgets(data,swapConfig->TAMANIO_PAGINA,file)){
			//printf("Pagina %d: (%s) \n", i, data);
			//i++;
	//	}
		fseek(file,byteInicial,SEEK_SET);
			   //int i=0;
	           /*while(i<=swapConfig->TAMANIO_PAGINA){
	        	  fgetc(file);
	              i++;
	    }*/
	    fgets(data,swapConfig->TAMANIO_PAGINA,file);
	   // printf("Lei la pagina (%d) del pid (%d): contenido (%s) \n", paquete->numPagina, paquete->pid, data);
		fclose(file);
		return data;
	}
	return NULL;
}



//ADMINISTRACION DE ESPACIOS LIBRE Y OCUPADO

mProc* eliminarmProc(int pid, t_list* ocupado,swapConfig* config){  //BUSCA AL MPROC POR SU PID Y LO ELIMINA DE LA LISTA DE OCUPADOS.

	int i= 0;
	int realizado = 0;
    //mProc* posta = malloc(sizeof(mProc));
    mProc* aux = malloc(sizeof(mProc));

    while((i<list_size(ocupado)) && !realizado){
         aux = (mProc*) list_get(ocupado,i);
         if(pid == aux->pid){
        	 aux = (mProc*)list_remove(ocupado,i);
        	// borrarDatosDeParticion(aux,config);
        	// printf("LLEGUE HASTA ACA \n");
//        	 posta->pid=aux->pid;
//        	 posta->pagInicial=aux->pagInicial;
//        	 posta->cantPags=aux->cantPags;
        	 //free(aux);
        	 realizado=1;
         }
         else i++;
   }
    //free(aux);
	return aux;
}

void borrarDatosDeParticion(mProc* mProc,swapConfig* config){
	int i=0;
	int byteInicial=mProc->pagInicial*config->TAMANIO_PAGINA;
	char* data = malloc(config->TAMANIO_PAGINA);
	data = string_substring_until("                                                                                                                                ",config->TAMANIO_PAGINA);
	FILE* file = fopen(config->NOMBRE_SWAP,"r+");
	if(file==NULL)
		printf("No se pudo abrir la partición para borrar los datos del mproc a eliminar\n");
	else{

		fseek(file,byteInicial,SEEK_SET);
		for(i=0;i<mProc->cantPags;i++)
			fputs(data,file);
		fflush(file);
		fclose(file);
		return;
	}
	return;
}

void agregarEspacioLibre(mProc* mProc,t_list* libres){ //AGREGA UN NUEVO ELEMENTO A LA LISTA DE LIBRES, A MENOS QUE EL MPROC LIBERADO GENERE DOS HUECOS CONTIGUOS, EN CUYO CASO LOS JUNTA.

	int i=0;
	int realizado=0;
	seccionLibre* seccion = malloc(sizeof(seccionLibre));
	seccionLibre* aux = malloc(sizeof(seccionLibre));
	while(i<list_size(libres) && !realizado){
	    seccion = list_get(libres,i);
		if(mProc->pagInicial==(seccion->comienzoLibre+seccion->pagsContiguas)){ // EJ: SI MPROC VA DE 10 A 20 Y LA SECCION LIBRE DE 0 A 9 ( SECCION->PAGS CONTIGUAS = 10)
			seccion->pagsContiguas= seccion->pagsContiguas + mProc->cantPags;// EN ESTE CASO PAGS CONTIGUAS = 21
		    realizado=1;
		}
		else if((seccion->comienzoLibre== mProc->pagInicial+mProc->cantPags)){
            seccion->comienzoLibre=mProc->pagInicial;
		    seccion->pagsContiguas= seccion->pagsContiguas + mProc->cantPags;
		    realizado=1;
		}
		else i++;
	}
	if(!realizado){
		aux->comienzoLibre=mProc->pagInicial;
		aux->pagsContiguas=mProc->cantPags;
		list_add(libres,aux);

	}
    return;
	}

void* recibirPedidosMemoria(void* data){
		infoHilos* info = (infoHilos*) data;
		int bytesRecibidos;
     	t_paquete_envio* comand = malloc(sizeof(t_paquete_envio));
		while(1){
            bytesRecibidos = 0;
			bytesRecibidos = RecvAll(info->socket,comand);
			if(bytesRecibidos>0){
				        pthread_mutex_lock(&mutex);
				        t_paquete_envio* comand2 = malloc(sizeof(t_paquete_envio));
				        comand2->data=comand->data;
				        comand2->data_size=comand->data_size;
				        queue_push(info->cola,comand2);
				        pthread_mutex_unlock(&mutex);
			}


			}

}








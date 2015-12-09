/*
 ============================================================================
 Name        : administradorDeSwap.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <commons/config.h>
#include "swapArranque.h"
#include "almacenamientoSwap.h"
#include "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.h"
#include <pthread.h>


int main(void) {
	 swapConfig* swapConfig = leerArchivoConfiguracion("configSwap");
     t_log* logs = crearArchivoLog();
     log_info(logs,"Administrador de Swap activo, listo para ejecutar.\n");
     t_list* espacioLibre = list_create();
     t_list* espacioOcupado = list_create();
     t_queue* colaPedidos = queue_create();
     pthread_mutex_init(&mutex,NULL);
     inicializarAlmacenamiento(swapConfig->CANTIDAD_PAGINAS,espacioLibre);
     inicializarParticion(swapConfig);


     infoHilos* info =  (infoHilos*) malloc(sizeof(infoHilos));
     info->cola= colaPedidos;
     info->config = swapConfig;
     info->listaLibres = espacioLibre;
     info->listaOcupados = espacioOcupado;
     info->logs = logs;
     pthread_t hilo1;
     pthread_t hilo2;

     int listenningSocket = Escuchar(info->config->PUERTO_ESCUCHA);
     int socketMemoria = Aceptar(listenningSocket);
     info->socket = socketMemoria;

     pthread_create(&hilo1,NULL,recibirPedidosMemoria,info);
     pthread_create(&hilo2,NULL,atenderConexionesDeMemoria,info);

     pthread_join(hilo1,NULL);
     pthread_join(hilo2,NULL);

	return EXIT_SUCCESS;
}







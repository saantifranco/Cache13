#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include "serializacion.h"

#define PathLength 256
typedef enum {PLANIFICADOR, CPU, MEMORIA, SWAP, DESCONOCIDO} Origen;

int Conectar(char* IP, int port);
int Aceptar(int socketID);
int Escuchar(int puerto_escucha);
int EnviarInt(int socketID, int contenido);
int Desconectar(int socketID);
Origen QuienSos(int socketID);
int Enviar(int socketID, char* contenido);
char* Recibir(int socketID);
int SendAll(int socketID, t_paquete_envio* package);
int RecvAll(int socketID, t_paquete_envio* package);
int RecvAll2(int socketID, t_paquete_envio* package);
#endif

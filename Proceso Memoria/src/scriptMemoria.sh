#!/bin/bash

cd
cd workspace/tp-2015-2c-operagwi2/Proceso\ Memoria/src

gcc MemoriaFunciones.c MemoriaArranque.c "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.c" "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.c" AdministradorDeMemoria.c -o memoria -lcommons -lpthread

./memoria

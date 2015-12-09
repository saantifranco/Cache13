#!/bin/bash

cd
cd workspace/tp-2015-2c-operagwi2/Proceso\ Swap/src

gcc swapArranque.c almacenamientoSwap.c "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.c" "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.c" administradorDeSwap.c -o swap -lcommons -lpthread

./swap

#!/bin/bash

cd
cd workspace/tp-2015-2c-operagwi2/Proceso\ CPU/src

gcc CPUArranque.c CPUFunciones.c "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.c" "/home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.c" CPU.c -o cpu -lcommons -lpthread

./cpu

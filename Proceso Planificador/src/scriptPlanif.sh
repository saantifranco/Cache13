#!/bin/bash

cd
cd workspace/tp-2015-2c-operagwi2/Proceso\ Planificador/src

gcc PlanificadorArranque.c PlanificadorFunciones.c /home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/sockets.c /home/utnso/workspace/tp-2015-2c-operagwi2/commonsPropias/src/serializacion.c Planificador.c -o planif -lcommons -lpthread

./planif

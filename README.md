# Cache13
Trabajo Práctico de Sistemas Operativos 2C año 2015.

Observaciones:
- En el manejo de colas se generaron esperas activas por haber utilizado estructuras del tipo:
while(1){
  if(!isEmpty(aQueue)){
    ...
  }
}
Reemplazar dichas estructuras por semáforos contadores que bloqueen al proceso cuando así corresponda.

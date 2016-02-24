# Cache13
Trabajo Práctico de Sistemas Operativos 2C año 2015.

Observaciones:

En el manejo de colas se generaron esperas activas por haber utilizado estructuras del tipo:

while(1){

  if(!isEmpty(aQueue)){
  
    ...
    
  }
  
}

Reemplazar dichas estructuras por semáforos contadores que bloqueen al proceso cuando así corresponda.

Los semáforos en C con el estándar POSIX.

Biblioteca "semaphore.h"

sem_t sem; //declaracion del tipo de dato

Operaciones:

int sem_init(sem_t *sem, int pshared, unsigned int value);

int sem_destroy(sem_t *sem);

int sem_wait(sem_t *sem);

int sem_trywait(sem_t *sem);

int sem_post(sem_t *sem);

int sem_getvalue(sem_t *sem, int *sval);

#include "swapArranque.h"



swapConfig* leerArchivoConfiguracion(char *ruta_archivo) {


	t_config* config = config_create(ruta_archivo);
	printf("Leyendo archivo de configuraciones: %s\n", ruta_archivo);


	swapConfig* elGranSwap = (swapConfig*) malloc(sizeof(swapConfig));
	elGranSwap->PUERTO_ESCUCHA = config_get_int_value(config, "PUERTO_ESCUCHA");
	elGranSwap->NOMBRE_SWAP = config_get_string_value(config, "NOMBRE_SWAP");
	elGranSwap->CANTIDAD_PAGINAS = config_get_int_value (config, "CANTIDAD_PAGINAS");
	elGranSwap->TAMANIO_PAGINA = config_get_int_value(config, "TAMANIO_PAGINA");
	elGranSwap->RETARDO_COMPACTACION = config_get_int_value(config, "RETARDO_COMPACTACION");
    elGranSwap->RETARDO_SWAP = config_get_double_value(config,"RETARDO_SWAP");



	printf("Puerto escucha: %d \n", elGranSwap->PUERTO_ESCUCHA);
	printf("Nombre Swap: %s \n", elGranSwap->NOMBRE_SWAP);
	printf("Cantidad de paginas: %d \n", elGranSwap->CANTIDAD_PAGINAS);
	printf("Tamanio de paginas: %d \n", elGranSwap->TAMANIO_PAGINA);
	printf("Retardo compactacion: %d \n", elGranSwap->RETARDO_COMPACTACION);
	printf("Retardo swap: %lf \n",elGranSwap->RETARDO_SWAP);

	puts("Listo.\n");

	return elGranSwap;
}

t_log* crearArchivoLog() {


	remove("logsSwap");

	puts("Creando archivo de logueo...\n");

	t_log* logs = log_create("logsSwap", "SwapLog", 0, LOG_LEVEL_TRACE);

	if (logs == NULL) {
		puts("No se pudo generar el archivo de logueo\n");
		return NULL;
	}


	log_info(logs, "INICIALIZACION DEL ARCHIVO DE LOGUEO");

	puts("Listo.\n");

	return logs;
}

void inicializarParticion(swapConfig* config){
	char* palabra=string_new();
	string_append(&palabra,"dd if=/dev/zero of=");
	string_append(&palabra,config->NOMBRE_SWAP);
	string_append(&palabra," bs=");
	string_append(&palabra,string_itoa(config->TAMANIO_PAGINA));
	string_append(&palabra," count=");
	string_append(&palabra,string_itoa(config->CANTIDAD_PAGINAS));
	system(palabra);
	return;
}










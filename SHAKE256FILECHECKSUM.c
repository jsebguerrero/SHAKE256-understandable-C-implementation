#include "funciones.h"		//Archivo de cabecera que contiene las funciones usadas en el main

//fin de funciones
int main(int arg, char *argv[])
{
	FILE *file;  //archivo
	long long int bytes;
	int bloques;
	uint64_t s[25]={0}; uint64_t s_uint64[25]={0};	//Se defininen los arreglos y se inicializan en 0
	uint8_t s_uint8[200]={0}; uint8_t *in_string;
	//bits de salida
	int d = atoi(argv[4]);		//Se convierte el char leido en consola y lo convierte en entero por medio de la funcion atoi()
	//abrir archivo, si no corresponde el programa se sale
	file = fopen(argv[2],"rb");   //-f filepath -d salida
	if (file==NULL)
	{
	  fputs ("Archivo no encontrado\n", stderr);
	  exit(1);
	}
	fseek (file,0,SEEK_END);	//Se lee hasta el final del archivo para saber cuantos bytes contiene
	bytes = ftell (file);		//se almacena en una variable
	rewind (file); 			//se devuelve el puntero de lectura al inicio del archivo
	bloques = (bytes/136); 		//numero de bloques multiplos de 1088 bits (136 bytes)
	in_string = (uint8_t*)malloc (sizeof(uint8_t)*136*(1+bloques));	//Se crea un arreglo el cual tendra un tamano dependiendo del numero de 									//bloques completos, si no hay como minimo se crea 1 
	//guardamos todo en el string
	fread(in_string,1,bytes,file);
	b2lendian(in_string,bytes);//cambiar endianess
	//ahora padding al bloque grande que hicimos arriba
	padding(in_string,bytes,bloques);
	//el padding agrega los 4 unos, despues un 1 seguido de ceros y uno al final
	//esponja y keccakf1600
	sponge_keccakf(in_string, s_uint8,s_uint64,s,bloques);
	//termina la esponja y se imprime el hash
	keyhex(s,d,25);
	//Se libera el espacio usado en memoria 
	free(in_string);
	fclose(file);
	//Se muestra el tiempo utilizado por el codigo
	return 0;
}

#include "funciones.h"		//Archivo de cabecera que contiene las funciones usadas en el main

//fin de funciones
int main(int arg, char *argv[])
{
	FILE *file;  //archivo
	long long int bytes;
	int bloques;
	uint64_t s[25]={0}; uint64_t s_uint64[25]={0};	//define arrays in stack to save time
	uint8_t s_uint8[200]={0}; uint8_t *in_string;
	//output bits
	int d = atoi(argv[4]); // get output bits
	file = fopen(argv[2],"rb");   //-f filepath -d output
	fseek (file,0,SEEK_END);	//Read file to see how bytes does it have
	bytes = ftell (file);		//store file bytes
	rewind (file); 	
	bloques = (bytes/136); 		// # of blocks of 1088 bits (136 bytes)
	in_string = (uint8_t*)malloc (sizeof(uint8_t)*136*(1+bloques));	//Array full of blocks
	//save all in string
	fread(in_string,1,bytes,file);
	b2lendian(in_string,bytes);//deal with byte order
	//padding to the last element
	padding(in_string,bytes,bloques);
	//send everything to sponge
	sponge_keccakf(in_string, s_uint8,s_uint64,s,bloques);
	//print hash
	keyhex(s,d,25);
	//free space
	free(in_string);
	fclose(file);
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <time.h>
//variables del keccak
#define c 512
#define r 1088
#define b 1600
//el nuevo modulo para el trabajo
static int modulo(int x, int y){
	if(x<0)
	{		//si el numero es negativo, el modulo se redefine
		x = x*-1;
		return (y-(x%y))%y;
	}
	else		//si el numero es positivo el modulo no cambia
	{
		return (x%y);
	}
}
//hace swap completo a un byte
static uint8_t bswap_8( uint8_t byte ){
return (byte & 0x80 ? 0x01 : 0) |
       (byte & 0x40 ? 0x02 : 0) |
       (byte & 0x20 ? 0x04 : 0) |
       (byte & 0x10 ? 0x08 : 0) |
       (byte & 0x08 ? 0x10 : 0) |
       (byte & 0x04 ? 0x20 : 0) |
       (byte & 0x02 ? 0x40 : 0) |
       (byte & 0x01 ? 0x80 : 0);
}

//conversion de endian par aun vector
static uint8_t b2lendian(uint8_t * vector,int sizev)
{
	int i;
	for(i=0;i<sizev;i++)
	{
		vector[i]=bswap_8(vector[i]);	//Nos permite reescribir el vector por medio de la funcion bswap_8
	}
}
//Funcion padding
static void padding(uint8_t * in_string, long long int bytes, int bloques)
{

        in_string[bytes]=(0b11111000);				//Agregar 5 bits de unos, estos son 1111 de la especificacion y el 										//primero solicitado para el padding
        for(int i=bytes+1; i< ((bloques+1)*136)-1; i++)		
	{
		in_string[i] = (0b00000000);			//Luego se concatenan i ceros, dependiendo de lo que se necesite para 										//completar 1079 bits
        }						
        in_string[(((bloques+1)*136)-1)]=0b00000001;		//y por ultimo un 1 para completar la cadena


}

//constantes RC en HEXADECIMAL
static const uint64_t RC[24] ={
0x8000000000000000,0x4101000000000000,0x5101000000000001,
0x0001000100000001,0xD101000000000000,0x8000000100000000,
0x8101000100000001,0x9001000000000001,0x5100000000000000,
0x1100000000000000,0x9001000100000000,0x5000000100000000,
0xD101000100000000, 0xD100000000000001, 0x9101000000000001,
0xC001000000000001, 0x4001000000000001, 0x0100000000000001,
0x5001000000000000, 0x5000000100000001, 0x8101000100000001,
0x0101000000000001, 0x8000000100000000, 0x1001000100000001};


//transformaciones para estado de 25 posiciones, deducciones sacadas de NIST FIPS 202 y [A]
//[A] https://github.com/coruus/keccak-tiny/blob/singlefile/keccak-tiny.c

static void theta(uint64_t *state){
	//Auxiliares C y D como pide NIST FIPS 202
	uint64_t C[5]={0};uint64_t D[5]={0};
	for(int i=0;i<5;i++){
		C[i]=state[i]^state[i+5]^state[i+10]^state[i+15]^state[i+20];	//el algoritmo es reducible a saltos de 5
	}
	for(int i=0;i<5;i++){
		D[i]=(C[modulo((i-1),5)])^(((C[modulo((i+1),5)])<<63)|((C[modulo((i+1),5)])>>1));
		//esta es cambiable por un salto de 63 hacia <- OR salto de 1 hacia ->
		//como estamos en little endian es de este modo, para big las flechas invierten sentido
	}
	for(int i=0;i<5;i++){
		for(int j=0;j<5;j++){
			state[i+5*j]=state[i+5*j]^D[i];	//del mismo modo que la primera se hace el xor cada 5
		}
	}
}
static void rho(uint64_t *state){
	int jbuffer;
	int i,j;i=1;j=0;
	for(int t=0; t<24;t++){
		state[i+5*j]=((((state[i+5*j])<<(64-(t+1)*(t+2)/2))) | (((state[i+5*j])>>((t+1)*(t+2)/2))));//Para hacer la transformacion rho 									//hay que tener en cuenta que al usar el operador shift se pueden perder datos
		jbuffer=j;
		j=modulo((2*i+3*j),5);
		i=jbuffer;
	}
}
static void pi(uint64_t *state){
	int i,j;
	uint64_t s_copy[25] = {0};	// Se crea copia de S para operar

	for(i=0;i<25;i++){
		s_copy[i]=state[i];
	}
	for(i=0;i<5;i++){
		for(j=0;j<5;j++){
	  		state[i+5*j]=s_copy[modulo((i+3*j),5)+5*i];
		}
	}
}
static void xci_iota(uint64_t *state,int ir){
	uint64_t s_copy[25] = {0};// Se crea copia de S para operar
	int i,j;
	for(i=0;i<25;i++){
		s_copy[i]=state[i];
	}
	for(i=0;i<5;i++){
		for(j=0;j<5;j++){
			state[i+5*j]=(s_copy[i+5*j])^((s_copy[(modulo((i+1),5))+5*j]^0xFFFFFFFFFFFFFFFFULL)	//Se aplica la transformacion 
		   	 & (s_copy[(modulo((i+2),5))+5*j]));  // Xci e iota en una sola funcion para generar mayor optimizacion 											
		}
	}
	state[0]^=RC[ir];
}
static void RND(uint64_t *state,int ir){
	theta(state);rho(state);pi(state);xci_iota(state,ir);	//Se hace el llamado a todas las funciones las cuales alteran al array state, 							//dado que esta dfeinido como un arreglo dinamico se puede laterar por medio de su direccion
}
// Fin de transformaciones
//esponja
static void sponge_keccakf(uint8_t *in_string,uint8_t s_uint8[],uint64_t s_uint64[],uint64_t s[],int bloques){
	int ir;
	for(int j=0;j<(bloques+1);j++){
		for(int i=0; i<(200);i++){
			if(i<136){ //desde 0 hasta 1087 bits (byte # 135), cada i es 8 bits llenados con la informacion del string
				s_uint8[i] = in_string[i+(136*j)];
			}
			else
				s_uint8[i] = 0b00000000;//agrega ceros desde 1088(byte # 136)
							//hasta 1599 (byte # 199)
			}
		//trabajar a 64 bits para eficiencia del algoritmo
		for(int k=0;k<25;k++){
			for(int l=0;l<8;l++){
		    		s_uint64[k] = (s_uint64[k]<<8)|(uint64_t)(s_uint8[l+(8*k)]); //Cada entrada del for se desplaza 8 bits y se 									//agregan 8 bits de informacion para obtener al final un arreglo en 64 bits
			}
			s[k]^=s_uint64[k];//se le hace xor con el 0 y se pasa a RND
		}
		//keccakf
		for(ir=0;ir<24;ir++){   //For para los rounds (rondas)
			RND(s,ir);
		}
	}
}
//conversion a hexa para mostrar hash
static void keyhex(uint64_t s[], int d,int slen){
	int count=56;
	int i=0;
		for(int j=0;j<d/8;j++)
		{
			printf("%02x",bswap_8(s[i]>>count));		//Se aplica la funcion swap para invertir los datos 
			count-=8;			//Se disminuye el contador en 8 hasta llegar a 0 en donde el
			if(count<0){
			count=56;
			i++;				//contador se reinicia a 56 y se aplica la impresion para otro bloque del hash 
				if(i>slen){		
					break;
				}
			}	 									
		}
	 //Impresion hash de salida
}


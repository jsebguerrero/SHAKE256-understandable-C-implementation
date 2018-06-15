#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <time.h>
//keccakf variables
#define c 512
#define r 1088
#define b 1600
//redefine modulo operator
static int modulo(int x, int y){
	if(x<0)
	{		//it only changes for negative values
		x = x*-1;
		return (y-(x%y))%y;
	}
	else		
	{
		return (x%y);
	}
}
//swap an entire byte
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

//swap an entire vector of uint8_t type
static uint8_t b2lendian(uint8_t * vector,int sizev)
{
	int i;
	for(i=0;i<sizev;i++)
	{
		vector[i]=bswap_8(vector[i]);
	}
}
//padding
static void padding(uint8_t * in_string, long long int bytes, int bloques)
{

        in_string[bytes]=(0b11111000);	//Add 4 ones and 1 extra one requested by the standard
        for(int i=bytes+1; i< ((bloques+1)*136)-1; i++)		
	{
		in_string[i] = (0b00000000);	//complete with zeros to fill 1079 bits
        }						
        in_string[(((bloques+1)*136)-1)]=0b00000001;	//and the last 1 to complete the chain


}

//RC constants to save time
static const uint64_t RC[24] ={
0x8000000000000000,0x4101000000000000,0x5101000000000001,
0x0001000100000001,0xD101000000000000,0x8000000100000000,
0x8101000100000001,0x9001000000000001,0x5100000000000000,
0x1100000000000000,0x9001000100000000,0x5000000100000000,
0xD101000100000000, 0xD100000000000001, 0x9101000000000001,
0xC001000000000001, 0x4001000000000001, 0x0100000000000001,
0x5001000000000000, 0x5000000100000001, 0x8101000100000001,
0x0101000000000001, 0x8000000100000000, 0x1001000100000001};


static void theta(uint64_t *state){
	// C y D as NIST FIPS 202
	uint64_t C[5]={0};uint64_t D[5]={0};
	for(int i=0;i<5;i++){
		C[i]=state[i]^state[i+5]^state[i+10]^state[i+15]^state[i+20];	
	}
	for(int i=0;i<5;i++){
		D[i]=(C[modulo((i-1),5)])^(((C[modulo((i+1),5)])<<63)|((C[modulo((i+1),5)])>>1));
		//theta swaps
	}
	for(int i=0;i<5;i++){
		for(int j=0;j<5;j++){
			state[i+5*j]=state[i+5*j]^D[i];	//theta xor's
		}
	}
}
static void rho(uint64_t *state){
	int jbuffer;
	int i,j;i=1;j=0;
	for(int t=0; t<24;t++){
		state[i+5*j]=((((state[i+5*j])<<(64-(t+1)*(t+2)/2))) | (((state[i+5*j])>>((t+1)*(t+2)/2))));//rho transformation
		jbuffer=j;
		j=modulo((2*i+3*j),5);
		i=jbuffer;
	}
}
static void pi(uint64_t *state){
	int i,j;
	uint64_t s_copy[25] = {0};	// copy of state is needed in order to operate

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
	uint64_t s_copy[25] = {0}; // same here
	int i,j;
	for(i=0;i<25;i++){
		s_copy[i]=state[i];
	}
	for(i=0;i<5;i++){
		for(j=0;j<5;j++){
			state[i+5*j]=(s_copy[i+5*j])^((s_copy[(modulo((i+1),5))+5*j]^0xFFFFFFFFFFFFFFFFULL)
		   	 & (s_copy[(modulo((i+2),5))+5*j]));  // this includes xci and iota 											
		}
	}
	state[0]^=RC[ir];
}
static void RND(uint64_t *state,int ir){
	theta(state);rho(state);pi(state);xci_iota(state,ir);	//call every function
}
// end of transformations
//sponge
static void sponge_keccakf(uint8_t *in_string,uint8_t s_uint8[],uint64_t s_uint64[],uint64_t s[],int bloques){
	int ir;
	for(int j=0;j<(bloques+1);j++){
		for(int i=0; i<(200);i++){
			if(i<136){ //from 0 - 1087 bits (byte # 135), every i is 8 bits filled w/ string info
				s_uint8[i] = in_string[i+(136*j)];
			}
			else
				s_uint8[i] = 0b00000000;//add zeros from 1088(byte # 136)
							//stop at bit 1599 (byte # 199)
			}
		//switch to 64 bits 
		for(int k=0;k<25;k++){
			for(int l=0;l<8;l++){
		    		s_uint64[k] = (s_uint64[k]<<8)|(uint64_t)(s_uint8[l+(8*k)]); //Every for input is shifted 8 bits & 8 bitts are added, at the end you should have 64 bits
			}
			s[k]^=s_uint64[k];//xor with 0 for first iteration, as standard describes
		}
		//keccakf
		for(ir=0;ir<24;ir++){   //iterate every transformation
			RND(s,ir);
		}
	}
}
//show your hash
static void keyhex(uint64_t s[], int d,int slen){
	int count=56;
	int i=0;
		for(int j=0;j<d/8;j++)
		{
			printf("%02x",bswap_8(s[i]>>count));		//apply swap to every byte
			count-=8;			//counter to switch to next block
			if(count<0){
			count=56;
			i++;				//restar counter for next block
				if(i>slen){		
					break;
				}
			}	 									
		}
	 //done
}


/*
	File:	pocsag_bch.c
	Author:	(C) Alexey Kuznetsov, avk@itn.ru

	This is BCH(31:21) encoder implementation mainly intended for POCSAG encoders.
	Therefore, the input argument 'dw' should be properly formatted as POCSAG codeword (please refer to ITU-R M.584 for additional detail).
	
	This code can be freely used for any personal and non-commercial purposes provided this copyright notice is preserved.
	For any other purposes please contact me at e-mail above or any other e-mail listed at https://github.com/avk-sw/pocsag2sdr
*/

#include <stdint.h>

#define	K	21
#define	G_POLY	03551

uint32_t pocsag_bch( uint32_t dw )
{
	int i;
	uint32_t cp,mask_mp;

	for( i=0,cp=0,mask_mp=0x80000000; i<K; i++,mask_mp>>=1 ) {
		cp <<= 1;
		if( (dw & mask_mp ? 1 : 0) ^ (cp & 0x400 ? 1 : 0) ) {
			cp ^= G_POLY; cp |= 1;
		} else {
			cp &= 0x3FE;
		}
	}
	cp &= 0x3FF; dw &= 0xFFFFF800; dw |= (cp<<1);
	return dw;
}

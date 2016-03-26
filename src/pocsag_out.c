/*
File:	pocsag_out.c
Author:	(C) Alexey Kuznetsov, avk@itn.ru

This code can be freely used for any personal and non-commercial purposes provided this copyright notice is preserved.
For any other purposes please contact me at e-mail above or any other e-mail listed at https://github.com/avk-sw/pocsag2sdr
*/

#include <stdint.h>
#include <stdio.h>

#include "pocsag2sdr.h"

static uint32_t cycles;

int pocsag_out( POCSAG_tx *p_tx,int (*output_bit)(int bit),int inv,int verbose ) {
	uint32_t p_cw;
	int i;
	for (i = 0; get_cws(p_tx, &p_cw, 4) == 4; i++) {
		int j;
		uint32_t mask;
		if (i == 18 || (i > 18 && (i - 18) % 17 == 0)) {
			if (verbose>1) printf("\n");
		}
		if (verbose>1) printf("%08lX ", p_cw);
		for (j = 0, mask = 0x80000000; j < 32; j++, mask >>= 1) {
			if (output_bit(((p_cw & mask) != 0) ^ inv) == (-1)) {
				return (-1);
			}
		}
	}
	if (verbose>1) printf("\n");
	return 0;
}
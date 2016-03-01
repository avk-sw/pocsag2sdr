/*
File:	fsk.c
Author:	(C) Alexey Kuznetsov, avk@itn.ru

This code can be freely used for any personal and non-commercial purposes provided this copyright notice is preserved.
For any other purposes please contact me at e-mail above or any other e-mail listed at https://github.com/avk-sw/pocsag2sdr
*/

#include <stdint.h>
#include <stdlib.h>

#define	_USE_MATH_DEFINES
#include <math.h>

#include "fsk.h"

int init_fsk(FSK_params *fsk_p, uint32_t sample_rate, uint32_t dev, uint32_t bps,uint32_t ampl) {
	int i;

	fsk_p->sample_rate = sample_rate;
	fsk_p->dev = dev;
	fsk_p->bit_rate = bps;

	fsk_p->divider_d = (double)sample_rate / (double)dev;
	fsk_p->divider = lrint(fsk_p->divider_d);
	fsk_p->cycles_per_bit_d = (double)sample_rate / (double)bps;
	fsk_p->cycles_per_bit = lrint(fsk_p->cycles_per_bit_d);

	fsk_p->sins = malloc(fsk_p->divider*sizeof(double));
	if (fsk_p->sins == NULL) return (-1);
	fsk_p->coss = malloc(fsk_p->divider*sizeof(double));
	if (fsk_p->coss == NULL) return (-1);

	for (i = 0; i < fsk_p->divider; i++) {
		double t;
		t = (double)i / (double)fsk_p->divider;
		t *= 2 * M_PI;
		fsk_p->sins[i] = (double)ampl*sin(t);
		fsk_p->coss[i] = (double)ampl*cos(t);
	}

	return 0;
}
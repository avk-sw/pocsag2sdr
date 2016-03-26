/*
File:	fsk.c
Author:	(C) Alexey Kuznetsov, avk@itn.ru

This code can be freely used for any personal and non-commercial purposes provided this copyright notice is preserved.
For any other purposes please contact me at e-mail above or any other e-mail listed at https://github.com/avk-sw/pocsag2sdr
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#define	_USE_MATH_DEFINES
#include <math.h>

#include "fsk.h"
#include "my_strerror.h"

static uint32_t cycles;
static FILE *output_file;
static FSK_params *fsk_p;

int init_fsk(uint32_t sample_rate, uint32_t dev, uint32_t bps,uint32_t ampl,FILE *ofp) {
	uint32_t i;

	fsk_p = malloc(sizeof(FSK_params));
	if (fsk_p == NULL) {
		set_error(ERR_ERRNO, "[malloc]");
		return (-1);
	}

	fsk_p->sample_rate = sample_rate;
	fsk_p->dev = dev;
	fsk_p->bit_rate = bps;

	fsk_p->divider_d = (double)sample_rate / (double)dev;
	fsk_p->divider = lrint(fsk_p->divider_d);
	fsk_p->cycles_per_bit_d = (double)sample_rate / (double)bps;
	fsk_p->cycles_per_bit = lrint(fsk_p->cycles_per_bit_d);

	fsk_p->sins = malloc(fsk_p->divider*sizeof(double));
	if (fsk_p->sins == NULL) {
		set_error(ERR_ERRNO, "[malloc]");
		return (-1);
	}
	fsk_p->coss = malloc(fsk_p->divider*sizeof(double));
	if (fsk_p->coss == NULL) {
		set_error(ERR_ERRNO, "[malloc]");
		return (-1);
	}

	for (i = 0; i < fsk_p->divider; i++) {
		double t;
		t = (double)i / (double)fsk_p->divider;
		t *= 2 * M_PI;
		fsk_p->sins[i] = (double)ampl*sin(t);
		fsk_p->coss[i] = (double)ampl*cos(t);
	}
	cycles = 0;
	output_file = ofp;

	return 0;
}

int fsk_output_bit(int bit) {
	unsigned int t;
	for (t = 0; t < fsk_p->cycles_per_bit; t++) {
		int8_t buf[2];

		buf[0] = bit ? (uint8_t)fsk_p->sins[cycles] : (uint8_t)((-1)*fsk_p->sins[cycles]);
		buf[1] = (uint8_t)fsk_p->coss[cycles];
		fwrite(buf, 1, 2, output_file);
		if (ferror(output_file)) {
			set_error(ERR_ERRNO, "[fwrite]");
			return (-1);
		}
		if (++cycles >= fsk_p->divider) cycles = 0;
	}
	return 0;
}

FSK_params *get_fsk_params(void) {
	return fsk_p;
}

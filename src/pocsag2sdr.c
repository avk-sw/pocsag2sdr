/*
File:	pocsag2sdr.c
Author:	(C) Alexey Kuznetsov, avk@itn.ru

This code can be freely used for any personal and non-commercial purposes provided this copyright notice is preserved.
For any other purposes please contact me at e-mail above or any other e-mail listed at https://github.com/avk-sw/pocsag2sdr
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define	_USE_MATH_DEFINES
#include <math.h>

#include "pocsag2sdr.h"
#include "fsk.h"

static void usage(void) {
	printf(
"\nPOCSAG2SDR v.0.1 (C) Alexey Kuznetsov, avk@itn.ru, https://github.com/avk-sw/pocsag2sdr\n\
This program creates I/Q files suitable to transmit with SDR utlities like hackrf_transfer\n\
\n\
Usage: pocsag2sdr [-s <sample rate>] [-r <POCSAG baud rate>] [-d <deviation>] [-a <amplitude>] [-w <output file>] [-i] [-v] <cap code> <func> <message>\n\
-s <sample rate>: sample rate in samples per second, 8000000 by default; consult your SDR docs for the optimal values\n\
-r <POCSAG baud rate>: common values are 512, 1200 and 2400; though actually can be any integer. Default value is 1200\n\
-d <deviation>: frequency deviation; 4500 by default\n\
-a <amplitude>: maximum amplitude for I/Q components; 64 by default\n\
-w <output file>: output file name; by default automatically generated\n\
-i : turn on signal inversion; turned off by default\n\
-v : turn on verbose mode\n\
\n\
Destination parameters:\n\
<cap code> : pager CAP code\n\
<func> : function code; valid values from 0 to 3\n\
<message> : alphanumeric message; numeric messages aren't currently supported\n\
");
}

int main( int argc, char *argv[] )
{
	uint32_t sample_rate = 8000000ul;
	uint32_t baud_rate = 1200;
	uint32_t dev = 4500;
	uint32_t amplitude = 0x40;
	uint8_t *ofile = NULL;
	uint8_t ofile_name[_MAX_PATH + 1];
	int inv = 0,verbose = 0;

	POCSAG_tx *p_tx;
	FSK_params fsk_p;
	uint32_t cap_code, func;
	uint8_t *msg;

	FILE *ofp;
	uint32_t p_cw;
	uint32_t cycles;
	int i;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') break;
		switch (argv[i][1]) {
		case 'i': inv = 1;	break;
		case 'v': verbose = 1;	break;
		case 's':
			if (++i >= argc) {
				fprintf(stderr, "No argument for -s option\n");
				usage();
				return 1;
			}
			sample_rate = atoi(argv[i]);
			break;
		case 'r':
			if (++i >= argc) {
				fprintf(stderr, "No argument for -r option\n");
				usage();
				return 1;
			}
			baud_rate = atoi(argv[i]);
			break;
		case 'd':
			if (++i >= argc) {
				fprintf(stderr, "No argument for -d option\n");
				usage();
				return 1;
			}
			dev = atoi(argv[i]);
			break;
		case 'a':
			if (++i >= argc) {
				fprintf(stderr, "No argument for -a option\n");
				usage();
				return 1;
			}
			amplitude = atoi(argv[i]);
			break;
		case 'w':
			if (++i >= argc) {
				fprintf(stderr, "No argument for -a option\n");
				usage();
				return 1;
			}
			ofile = argv[i];
			break;
		default:
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			usage();
			return 1;
		}
	}

	if ( (argc-i)<3 ) {
		fprintf(stderr, "No destination specified\n");
		usage();
		return 1;
	}
	cap_code = atoi(argv[i]);
	func = atoi(argv[i + 1]) & 3;
	msg = argv[i + 2];
	if (ofile) {
		strncpy(ofile_name, ofile, _MAX_PATH);
	} else {
		snprintf(ofile_name, _MAX_PATH, "POCSAG_%ld_%ld_%ld_%ld_%ld%s.bin",cap_code,func,baud_rate,dev,sample_rate,inv ? "_inv" : "");
	}
	ofp = fopen(ofile_name, "wb");
	if (ofp == NULL) {
		fprintf(stderr, "Can't open output file '%s': %s\n",ofile_name,strerror(errno));
		return 1;
	}

	init_fsk(&fsk_p, sample_rate, dev, baud_rate, amplitude);

	if (verbose) {
		printf("Sample rate: %ld\n", sample_rate);
		printf("Samples per bit: %ld/%lf\n", fsk_p.cycles_per_bit, fsk_p.cycles_per_bit_d);
		printf("Samples per freq cycle: %ld/%lf\n", fsk_p.divider, fsk_p.divider_d);
	}

	p_tx = create_preamble();
	if (p_tx == NULL) {
		fprintf(stderr, "Can't allocate memory for POCSAG preamble: %s\n", strerror(errno));
		return 1;
	}

	add_message(p_tx,cap_code,func,msg);

	cycles = 0;
	for (i=0; get_cws(p_tx, &p_cw, 4) == 4; i++) {
		int j;
		uint32_t mask;
		if (i == 18 || (i>18 && (i - 18) % 17 == 0) ) {
			if( verbose ) printf("\n");
		}
		if (verbose) printf("%08lX ", p_cw);
		for (j = 0, mask = 0x80000000; j < 32; j++,mask>>=1) {
			int t;
			for (t = 0; t < fsk_p.cycles_per_bit; t++) {
				int8_t buf[2];

				buf[0] = (p_cw & mask) ? (uint8_t)fsk_p.sins[cycles] : (uint8_t)((-1)*fsk_p.sins[cycles]);
				if (inv) buf[0] *= (-1);
				buf[1] = (uint8_t)fsk_p.coss[cycles];
				fwrite(buf, 1, 2, ofp);
				if (++cycles >= fsk_p.divider) cycles = 0;
			}
		}
	}
	fclose(ofp);
	printf("I/Q has been successfully written\n");
    return 0;
}

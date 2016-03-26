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

#ifdef WIN32
#include <Windows.h>
#endif // WIN32


#include "pocsag2sdr.h"
#include "fsk.h"
#include "serial.h"
#include "code_tables.h"

static void usage(void) {
	PAGER_codetable *ptbl;
	printf(
"\nPOCSAG2SDR v.0.2 (C) Alexey Kuznetsov, avk@itn.ru, https://github.com/avk-sw/pocsag2sdr\n\
This program creates I/Q files suitable to transmit with SDR utlities like hackrf_transfer\n\
\n\
Usage: pocsag2sdr [-s <sample rate>] [-r <POCSAG baud rate>] [-d <deviation>] [-a <amplitude>] [-w <output file>] [-i] [-v] <cap code> <func> <message>\n\
-s <sample rate>: sample rate in samples per second, 8000000 by default; consult your SDR docs for the optimal values\n\
-r <POCSAG baud rate>: common values are 512, 1200 and 2400; though actually can be any integer. Default value is 1200\n\
-d <deviation>: frequency deviation; 4500 by default\n\
-a <amplitude>: maximum amplitude for I/Q components; 64 by default\n\
-w <output file>: output file name; by default automatically generated. If starts with '\\\\.\\', then it's treated as COM port name\n\
-t <delay> : PTT delay in milliseconds in case of COM port encoder mode\n\
-c <code_tables> : code table for message recoding\n\
-i : turn on signal inversion; turned off by default\n\
-v <number>: turn on verbose mode with the optional level <number>\n\
\n\
Destination parameters:\n\
<cap code> : pager CAP code\n\
<func> : function code; valid values from 0 to 3\n\
<message> : alphanumeric message; numeric messages aren't currently supported\n\
");
	printf("\nSupported code tables: ");
	for (ptbl = code_tables; ptbl->name != NULL; ptbl++) {
		printf("%s ", ptbl->name);
	}
	printf("\n");
}

char *optarg = NULL;
int optind = 1;

static int getopt(int argc, char *argv[], char *sw)
{
	char *s, sym;
	if (optind >= argc) return (-1);
	if (argv[optind][0] != '-' && argv[optind][0] != '/') return (-1);
	sym = argv[optind][1];
	s = strchr(sw, sym);
	optind++;
	if (s == NULL || *s == 0) {
		return sym;
	}
	if (s[1] == ':') {
		if (optind >= argc) {
			optarg = NULL;
		}
		else {
			optarg = argv[optind];
			optind++;
		}
	}
	return sym;
}

static void no_optarg(int opt, unsigned char *oarg ) {
	if (oarg != NULL) return;
	fprintf(stderr, "No optional argument for option '%c'\n", (unsigned char)opt);
	exit(1);
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
	PAGER_codetable *p_tbl=NULL;
	uint32_t cap_code, func;
	uint8_t *msg;

	FILE *ofp;

	int rc,isSerial=0,PTTdelay=0;

	while ((rc = getopt(argc, argv, "iv:t:s:r:d:a:w:c:")) != (-1)) {
		switch (rc) {
		case 'i': inv = 1;	break;
		case 'v': verbose = 1;
			if (optarg != NULL) verbose = atoi(optarg);
			break;
		case 't': no_optarg(rc, optarg);
			PTTdelay = atoi(optarg);	break;
		case 's': no_optarg(rc, optarg);
			sample_rate = atoi(optarg); break;
		case 'r': no_optarg(rc, optarg);
			baud_rate = atoi(optarg);	break;
		case 'd': no_optarg(rc, optarg);
			dev = atoi(optarg);	break;
		case 'a': no_optarg(rc, optarg);
			amplitude = atoi(optarg); break;
		case 'w': no_optarg(rc, optarg);
			ofile = optarg; break;
		case 'c': no_optarg(rc, optarg);
			for (p_tbl = code_tables; p_tbl->name != NULL; p_tbl++) {
				if (!strcmp(p_tbl->name, optarg)) break;
			}
			if (p_tbl->name == NULL) {
				fprintf(stderr, "Unsupported code table: %s\n", optarg);
				usage();
				return 1;
			}
			break;
		default:
			fprintf(stderr, "Unknown option: '%c'\n", (unsigned char)rc);
			usage();
			return 1;
		}
	}

	argc -= optind; argv += optind;
	if ( argc<3 ) {
		fprintf(stderr, "No destination specified\n");
		usage();
		return 1;
	}
	cap_code = atoi(argv[0]);
	func = atoi(argv[1]) & 3;
	msg = argv[2];
	if (ofile) {
		if (!strncmp(ofile, "com", 3) || !strncmp(ofile,"\\\\.\\",4) ) {
			isSerial = 1;
		} else {
			strncpy(ofile_name, ofile, _MAX_PATH);
		}
	} else {
		snprintf(ofile_name, _MAX_PATH, "POCSAG_%ld_%ld_%ld_%ld_%ld%s.bin",cap_code,func,baud_rate,dev,sample_rate,inv ? "_inv" : "");
	}

	if (!isSerial) {
		printf("*** START *** SDR I/Q file generation mode\n");
		ofp = fopen(ofile_name, "wb");
		if (ofp == NULL) {
			fprintf(stderr, "Can't open output file '%s': %s\n", ofile_name, strerror(errno));
			return 1;
		}

		if (init_fsk(sample_rate, dev, baud_rate, amplitude, ofp) == (-1)) {
			fprintf(stderr, "[init_fsk]%s\n", my_strerror());
			return 1;
		}

		if (verbose) {
			FSK_params *fsk_p = get_fsk_params();
			printf("Sample rate: %ld\n", sample_rate);
			printf("Samples per bit: %ld/%lf\n", fsk_p->cycles_per_bit, fsk_p->cycles_per_bit_d);
			printf("Samples per freq cycle: %ld/%lf\n", fsk_p->divider, fsk_p->divider_d);
		}
	} else {
		printf("*** START *** COM port encoder mode\n");
		if (init_serial(ofile, baud_rate, PTTdelay) == (-1)) {
			fprintf(stderr, "[init_serial]%s\n", my_strerror());
			return 1;
		}
		if (verbose) {
			COM_params *com_p = get_serial_params();
			printf("Ticks per second: %lld\n", com_p->ticks_per_second.QuadPart);
			printf("Ticks per bit: %lld\n", com_p->ticks_per_bit);
		}
	}
	p_tx = create_preamble();
	if (p_tx == NULL) {
		fprintf(stderr, "[create_preamble]%s\n", my_strerror());
		return 1;
	}

	if (p_tbl != NULL) {
		uint8_t *recoded_msg = malloc(strlen(msg) + 1);
		int i;
		if (recoded_msg == NULL) {
			fprintf(stderr, "Can't allocate memory for recoded message\n");
			return 1;
		}
		for (i = 0; msg[i]; i++) {
			recoded_msg[i] = p_tbl->table[msg[i]];
		}
		recoded_msg[i] = 0;
		if (verbose) {
			printf("Original message: '%s'\n Recoded message: '%s'\n", msg, recoded_msg);
		}
		msg = recoded_msg;
	}
	if (add_message(p_tx, cap_code, func, msg) == (-1)) {
		fprintf(stderr, "[add_message]%s\n", my_strerror());
		return 1;
	}

	if (pocsag_out(p_tx, isSerial ? serial_output_bit : fsk_output_bit, inv, verbose) == (-1)) {
		fprintf(stderr, "[pocsag_out]%s\n", my_strerror());
	}

	if (isSerial) {
		COM_params *com_p = get_serial_params();
		end_serial();
		printf("*** FINISH *** %ld bits have been sent, frequency: %lld, calculated # of ticks per bit: %lld, average # of ticks per bit: %lld\n", com_p->total_bits_sent,com_p->ticks_per_second.QuadPart,com_p->ticks_per_bit,(com_p->last_bit_ts.QuadPart - com_p->first_bit_ts.QuadPart)/com_p->total_bits_sent);
		if (com_p->bits_with_delays) {
			printf("*** WARNING *** %ld bits have been sent with delays, maximum delay is %lld ticks (%lf seconds)\n", com_p->bits_with_delays, com_p->max_delay,(double)com_p->max_delay/(double)com_p->ticks_per_second.QuadPart);
		}
		// printf("GetTickCount stats: %ld msecs has elapsed, %lf msecs per bit\n", com_p->dwEnd - com_p->dwStart, (double)(com_p->dwEnd - com_p->dwStart) / (double)com_p->total_bits_sent);
	} else {
		fclose(ofp);
		printf("*** FINISH *** I/Q data have been successfully written to '%s'\n",ofile_name);
	}
    return 0;
}

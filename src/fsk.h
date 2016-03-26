#include <stdint.h>

typedef struct FSK_params {
	// initial parameters
	uint32_t sample_rate;	// N of samples per second
	uint32_t dev;			// deviation in Hz
	uint32_t bit_rate;		// bit rate in bits per second

	// calculated parameters
	uint32_t divider;
	uint32_t cycles_per_bit;
	double divider_d;
	double cycles_per_bit_d;

	double *sins;
	double *coss;
} FSK_params;

int init_fsk(uint32_t sample_rate, uint32_t dev, uint32_t bps, uint32_t ampl,FILE *ofp);
int fsk_output_bit(int bit);
FSK_params *get_fsk_params(void);

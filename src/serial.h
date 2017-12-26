#ifdef WIN32
#include <stdint.h>
#include <windows.h>

typedef struct COM_params {
	HANDLE serial_dev;
	LARGE_INTEGER ticks_per_second;
	uint64_t ticks_per_bit;
	int PTTdelay,PTTinv;
	int DtrRtsX;
	DWORD PTTon, PTToff;
	DWORD BITon, BIToff;
	LARGE_INTEGER first_bit_ts;
	LARGE_INTEGER last_bit_ts;
	LARGE_INTEGER next_bit_ts;
	uint32_t total_bits_sent;
	uint32_t bits_with_delays;
	uint64_t max_delay;
	DWORD dwStart, dwEnd;
} COM_params;

int init_serial(char *tty_name, uint32_t bps, int PTTdelay, int DtrRtsX, int PTTinv, int KeepPTT );
COM_params *get_serial_params(void);
int serial_output_bit(int bit);
int start_serial(void);
int end_serial(void);
#endif
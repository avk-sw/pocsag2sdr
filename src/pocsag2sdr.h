#include <stdint.h>
#include "my_strerror.h"

#define	CW_PREAMBLE	0xAAAAAAAA
#define	CW_SYNC		0x7CD215D8
//#define	CW_IDLE		0x7AC9A197
#define	CW_IDLE	0x7A89C197

typedef struct POCSAG_batch {
	uint32_t data[17];
	struct POCSAG_batch *next;
} POCSAG_batch;

typedef struct POCSAG_tx {
	uint32_t preamble[18];
	int cur_idx,isEOL;
	POCSAG_batch *cur_btch;
	POCSAG_batch *first;
	POCSAG_batch *last;
} POCSAG_tx;

POCSAG_batch *create_batch(void);
POCSAG_tx *create_preamble(void);
uint32_t make_csum(uint32_t dw);
int add_message(POCSAG_tx *p_tx, uint32_t capcode, uint32_t func, uint8_t *msg, int isNum);
uint32_t get_cws(POCSAG_tx *p_tx, uint32_t *buf, uint32_t len);

uint32_t pocsag_bch(uint32_t dw);

int pocsag_out(POCSAG_tx *p_tx, int (*output_bit)(int bit), int inv, int verbose);

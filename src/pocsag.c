/*
File:	pocsag.c
Author:	(C) Alexey Kuznetsov, avk@itn.ru

This code can be freely used for any personal and non-commercial purposes provided this copyright notice is preserved.
For any other purposes please contact me at e-mail above or any other e-mail listed at https://github.com/avk-sw/pocsag2sdr
*/

#include <stdlib.h>

#include "pocsag2sdr.h"

POCSAG_batch *create_batch(void) {
	POCSAG_batch *batch;
	int i;
	batch = malloc(sizeof(POCSAG_batch));
	if (batch == NULL) return NULL;
	batch->data[0] = CW_SYNC;
	for (i = 1; i < sizeof(batch->data) / sizeof(batch->data[0]); i++) {
		batch->data[i] = CW_IDLE;
	}
	batch->next = NULL;
	return batch;
};

POCSAG_tx *create_preamble(void) {
	POCSAG_tx *tx;
	int i;
	tx = malloc(sizeof(POCSAG_tx));
	if (tx == NULL) return NULL;
	for (i = 0; i < sizeof(tx->preamble) / sizeof(tx->preamble[0]); i++) {
		tx->preamble[i] = CW_PREAMBLE;
	}
	tx->first = tx->last = create_batch();
	tx->cur_idx = tx->isEOL = 0;
	tx->cur_btch = NULL;
	if (tx->first == NULL) {
		free(tx);
		tx = NULL;
	}
	return tx;
}

uint32_t make_csum(uint32_t dw) {
	uint32_t p;
	dw = pocsag_bch(dw);
	p = dw;
	p ^= p >> 16;	p ^= p >> 8; p ^= p >> 4;
	p &= 0xF;
	dw |= (0x6996 >> p) & 1;
	return dw;
}

int add_message(POCSAG_tx *p_tx,uint32_t capcode,uint32_t func,uint8_t *msg) {
	int frame,cw_bit;
	int i;
	uint32_t cw_capcode,cw_mask;
	POCSAG_batch *cur_btch;

	frame = ((capcode & 7) * 2)+1;
	cw_capcode = capcode >> 3;
	cw_capcode <<= 13;
	func &= 3;
	cw_capcode |= func << 11;
	cw_capcode &= 0x7FFFF800;
	cw_capcode=make_csum(cw_capcode);
	cur_btch = p_tx->last;
	cur_btch->data[frame]=cw_capcode;

	frame++;
	cur_btch->data[frame] = 0x80000000;
	cw_mask = 0x40000000;
	cw_bit = 0;

	for (i = 0; msg[i]; i++) {
		uint8_t mask;
		for (mask = 1; mask != 0x80; mask <<= 1) {
			if (msg[i] & mask) {
				cur_btch->data[frame] |= cw_mask;
			}
			cw_mask >>= 1;
			if (++cw_bit == 20) {
				cur_btch->data[frame] = make_csum(cur_btch->data[frame]);
				if (++frame == sizeof(cur_btch->data) / sizeof(cur_btch->data[0]) ) {
					POCSAG_batch *new_btch = create_batch();
					if (new_btch == NULL) return (-1);
					p_tx->last = cur_btch->next = new_btch;
					cur_btch = new_btch;
					frame = 1;
				}
				cur_btch->data[frame] = 0x80000000;
				cw_mask = 0x40000000;
				cw_bit = 0;
			}
		}
	}
	if( cw_bit ) cur_btch->data[frame] = make_csum(cur_btch->data[frame]);
	cur_btch->next = p_tx->last = create_batch();

	return 0;
}

uint32_t get_cws(POCSAG_tx *p_tx, uint32_t *buf, uint32_t len) {
	uint32_t ret_len = 0;
	if (p_tx->isEOL) return 0;
	
	for (; len >= 4; len -= 4,buf++) {
		if (p_tx->cur_btch == NULL) {
			buf[0] = p_tx->preamble[p_tx->cur_idx++];
			ret_len += 4;
			if (p_tx->cur_idx == (sizeof(p_tx->preamble) / sizeof(p_tx->preamble[0]))) {
				p_tx->cur_idx = 0;
				p_tx->cur_btch = p_tx->first;
			}
		} else {
			buf[0] = p_tx->cur_btch->data[p_tx->cur_idx++];
			ret_len += 4;
			if (p_tx->cur_idx == sizeof(p_tx->cur_btch->data) / sizeof(p_tx->cur_btch->data[0])) {
				p_tx->cur_idx = 0;
				p_tx->cur_btch = p_tx->cur_btch->next;
				if (p_tx->cur_btch == NULL) {
					p_tx->isEOL = 1;
				}
			}

		}
	}
	return ret_len;
}

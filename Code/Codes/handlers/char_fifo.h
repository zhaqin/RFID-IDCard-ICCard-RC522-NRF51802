//by tang
#ifndef CHAR_FIFO_H
#define CHAR_FIFO_H
#include<stdint.h>
typedef struct _CHAR_FIFO{
	uint8_t *pbuf;
	volatile uint32_t in;
	volatile uint32_t out;
	uint32_t mask;			/** the mask must equal to 2^N -1,N is 1~32 */
}CHAR_FIFO;

#define CHAR_FIFO_DEF(_char_fifo_name_,_char_fifo_len_) \
static uint8_t _char_fifo_name_##buffer[_char_fifo_len_];\
CHAR_FIFO _char_fifo_name_ = {_char_fifo_name_##buffer,0,0,_char_fifo_len_ - 1}

extern int32_t char_fifo_in_one(CHAR_FIFO *pchar_fifo,uint8_t data);
extern int32_t char_fifo_in_all(CHAR_FIFO *pchar_fifo,uint8_t *pdata,uint32_t len);
extern int32_t char_fifo_out_one(CHAR_FIFO *pchar_fifo,uint8_t *pdata);
extern int32_t char_fifo_out_all(CHAR_FIFO *pchar_fifo,uint8_t *pdata,uint32_t *plen);

extern uint32_t char_fifo_get_count(CHAR_FIFO *pchar_fifo);
extern int32_t char_fifo_r_clear(CHAR_FIFO *pchar_fifo);
#endif

//by tang
#include"char_fifo.h"
#include<string.h>
int32_t char_fifo_in_one(CHAR_FIFO *pchar_fifo,uint8_t data){
	uint32_t l;
	l = pchar_fifo->in - pchar_fifo->out;
	l = pchar_fifo->mask + 1 - l;
	if(l > 0u){
		pchar_fifo->pbuf[pchar_fifo->mask & pchar_fifo->in] = data;
		pchar_fifo->in++;
		return 0;
	}else{
		return -1;
	}
}
int32_t char_fifo_in_all(CHAR_FIFO *pchar_fifo,uint8_t *pdata,uint32_t len){
	uint32_t l,rest;
	l = pchar_fifo->in - pchar_fifo->out;
	l = pchar_fifo->mask + 1 - l;
	if(l >= len){
		rest = pchar_fifo->mask + 1 - (pchar_fifo->in & pchar_fifo->mask);
		if(rest >= len){
			memcpy(pchar_fifo->pbuf + (pchar_fifo->in & pchar_fifo->mask),pdata,len);
		}else{
			memcpy(pchar_fifo->pbuf + (pchar_fifo->in & pchar_fifo->mask),pdata,rest);
			memcpy(pchar_fifo->pbuf,pdata + rest,len - rest);
		}
		pchar_fifo->in += len;
		return 0;
	}else{
		return -1;
	}
}
int32_t char_fifo_out_one(CHAR_FIFO *pchar_fifo,uint8_t *pdata){
	uint32_t l;
	l = pchar_fifo->in - pchar_fifo->out;
	if(l > 0u){
		*pdata = pchar_fifo->pbuf[pchar_fifo->mask & pchar_fifo->out];
		pchar_fifo->out++;
		return 0;
	}else{
		return -1;
	}
}
int32_t char_fifo_out_all(CHAR_FIFO *pchar_fifo,uint8_t *pdata,uint32_t *plen){
	uint32_t l,rest;
	l = pchar_fifo->in - pchar_fifo->out;
	if(l > 0u){
		rest = pchar_fifo->mask + 1 - (pchar_fifo->out & pchar_fifo->mask);
		if(rest >= l){
			memcpy(pdata,pchar_fifo->pbuf + (pchar_fifo->out & pchar_fifo->mask),l);
		}else{
			memcpy(pdata,pchar_fifo->pbuf + (pchar_fifo->out & pchar_fifo->mask),rest);
			memcpy(pdata + rest,pchar_fifo->pbuf,l - rest);
		}
		pchar_fifo->out += l;
		*plen = l;
		return 0;
	}else{
//		*plen = 0;
		return -1;
	}
}


uint32_t char_fifo_get_count(CHAR_FIFO *pchar_fifo) {
    return pchar_fifo->out - pchar_fifo->in;
}

int32_t char_fifo_r_clear(CHAR_FIFO *pchar_fifo) {
    pchar_fifo->out = pchar_fifo->in;
    return 0;
}


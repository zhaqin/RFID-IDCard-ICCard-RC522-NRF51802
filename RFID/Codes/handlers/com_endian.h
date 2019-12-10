#ifndef _COM_ENDIAN_H_
#define _COM_ENDIAN_H_
#ifdef __cplusplus
extern "C" {
#endif
#if (PSP_ENDIAN == MQX_BIG_ENDIAN)
#define OS_LITTLE_ENDIAN //OS_BIG_ENDIAN
#endif
#define OS_BIG_ENDIAN
#ifdef OS_BIG_ENDIAN
#define _OS_SWAP2BYTE_CONST(n) (n)
#define _OS_SWAP4BYTE_CONST(n) (n)
#else
#define _OS_SWAP2BYTE_CONST(n) ((((n) & 0x00FF) << 8) | (((n) & 0xFF00) >> 8))
#define _OS_SWAP4BYTE_CONST(n) ((((n) \
                & 0x000000FF)         \
            << 24) | (((n) & 0x0000FF00) << 8) | (((n) & 0x00FF0000) >> 8) | (((n) & 0xFF000000) >> 24))
#endif

/*从buffer获取一个long/short/char型数据*/
#define GET_BUF_LONG(p) _OS_SWAP4BYTE_CONST(*(unsigned int *)(p))
#define GET_BUF_SHORT(p) _OS_SWAP2BYTE_CONST(*(unsigned short *)(p))
#define GET_BUF_CHAR(p) *(unsigned char *)(p)

/*将一个long/short/char型数据x 填入buffer p*/
#define PIN_LONG(p, x) { *(unsigned int *)(p) = _OS_SWAP4BYTE_CONST(x); }
#define PIN_SHORT(p, x) { *(unsigned short *)(p) = _OS_SWAP2BYTE_CONST(x); }
#define PIN_CHAR(p, x) { *(unsigned char *)(p) = (x); }

/*从buffer p 获取一个long/short/char型数据x*/
#define POUT_LONG(p, x) {(x) = GET_BUF_LONG(p); }
#define POUT_SHORT(p, x) {(x) = GET_BUF_SHORT(p); }
#define POUT_CHAR(p, x) {(x) = GET_BUF_CHAR(p); }

/*buffer/数据拷贝接口*/
#define COPY_BUF_LONG(p1, p2, _len) { *(unsigned int *)(p1) = GET_BUF_LONG(p2); }
#define COPY_BUF_SHORT(p1, p2, _len) { *(unsigned short *)(p1) = GET_BUF_SHORT(p2); }
#define COPY_BUF_CHAR(p1, p2, _len) { *(unsigned char *)(p1) = GET_BUF_CHAR(p2); }
#define COPY_BUF_STRING(p1, p2, _len)  memcpy(p1, p2, _len)

/* BEGIN: Added by guofan, 2013/12/2 */

/*封装一下数据偏移函数*/
#define PIN_STORE_CHAR(p, x, offset)      { \
        PIN_CHAR(p + offset, x);            \
        offset += sizeof(unsigned char);    \
}

#define PIN_STORE_SHORT(p, x, offset)     { \
        PIN_SHORT(p + offset, x);           \
        offset += sizeof(unsigned short);   \
}

#define PIN_STORE_LONG(p, x, offset)      { \
        PIN_LONG(p + offset, x);            \
        offset += sizeof(unsigned long);    \
}

#define PIN_STORE_BUFF(p, x, offset, len)  { \
        COPY_BUF_STRING(p + offset, x, len); \
        offset += len;                       \
}

#define POUT_STORE_CHAR(p, x, offset)     { \
        POUT_CHAR(p + offset, x);           \
        offset += sizeof(unsigned char);    \
}

#define POUT_STORE_SHORT(p, x, offset)    { \
        POUT_SHORT(p + offset, x);          \
        offset += sizeof(unsigned short);   \
}

#define POUT_STORE_LONG(p, x, offset)     { \
        POUT_LONG(p + offset, x);           \
        offset += sizeof(unsigned long);    \
}

#define POUT_STORE_BUFF(p, x, offset, len)  { \
        COPY_BUF_STRING(x, p + offset, len);  \
        offset += len;                        \
}

#ifdef __cplusplus
}
#endif
#endif

/*******************************************************************************
 * file end
 ******************************************************************************/



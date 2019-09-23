#ifndef __TLV_H
#define __TLV_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONSTRUCTED(tag) (*tag & (1 << 5))

typedef bool BOOL;

typedef struct
{
	size_t freeSpace;
	uint8_t * buf;
	const uint8_t * tag;
	const uint8_t * len;
	const uint8_t * val;
} Tlv_t;

size_t vallength(const uint8_t* const len);
typedef BOOL (*predicate_t)(Tlv_t*, void*, int);
BOOL TlvTraverseTags(const uint8_t* buffer, size_t length, predicate_t p, void * ctx, BOOL recursive, Tlv_t* tlv, int level);
BOOL TlvSearchTag(const uint8_t* buffer, size_t length, uint16_t tag, BOOL recursive, Tlv_t* tlv);
BOOL TlvCreate(Tlv_t* tlv, uint16_t tag, uint8_t* buffer, size_t length);
BOOL TlvAddData(Tlv_t* tlv, uint16_t tag, const uint8_t* value, size_t valueLen);
#endif  //__TLV_H

#include <stdio.h>
#include <string.h>
#include "tlv.h"

/* calculate lengths of TLV parts */
size_t taglength(const uint8_t* const tag) {
	size_t length = 1;

	if ((*tag & 0x1f) == 0x1f)
		while (tag[++length] & (1 << 7));

	return length;
}

size_t lenlength(const uint8_t* const len) {
	if (*len & (1 << 7))
		return 1 + (*len & 0x7f);

	return 1;
}

size_t vallength(const uint8_t* const len) {
	if (*len & (1 << 7)) {
		size_t length = 0;
		size_t lenlen = lenlength(len);
		for (size_t i = 1; i < lenlen; i++) {
			length <<= 8;
			length |= len[i];
		}

		return length;
	}

	return *len & 0x7f;
}
/* end calculate lengths of TLV parts */

/* write data in reverse */
BOOL writeByte(Tlv_t* tlv, uint8_t byte) {
	if (tlv->freeSpace == 0) return false;
	tlv->buf[--tlv->freeSpace] = byte;
	return true;
}

BOOL writeBytes(Tlv_t* tlv, const uint8_t* bytes, size_t length) {
	while (length)
		if (!writeByte(tlv, bytes[--length])) return false;
	return true;
}
/* end write data in reverse */

/* encoding of TLV parts */
#define SEVENBITS ((1 << 7) -1)
BOOL encodeLength(Tlv_t* tlv, size_t length) {
	if (length < SEVENBITS) {
		return writeByte(tlv, length);
	}
	
	uint8_t i = 0;
	while (length > 0) {
		if (!writeByte(tlv, length & 0xff)) return false;
		i++;
		length >>= 8;
	}
	if (i > SEVENBITS) return false;  // length length too big to encode
	return writeByte(tlv, i | (1 << 7));
}

BOOL encodeTag(Tlv_t* tlv, uint16_t tag) {
	return writeByte(tlv, tag & 0xff) &&
		((tag > 0xff) ? writeByte(tlv, tag >> 8) : true);
}

uint16_t decodeTag(const Tlv_t* tlv) {
	uint16_t tag = tlv->tag[0];
	if ((tag & 0x1f) == 0x1f) {
		tag <<= 8;
		tag |= tlv->tag[1];
	}
	return tag;
}
/* end encoding of TLV parts */

// Parse the data in buffer to a TLV object. If the buffer contains more than one
// TLV objects at the same level in the buffer, only the first TLV object is parsed.
// This function should be called before any other TLV function calls.
BOOL TlvParse(const uint8_t* buffer, size_t length, Tlv_t* tlv) {
	for(size_t i = 0; i < length; i++) {
		if (buffer[i] == 0x00)
			continue;
		tlv->tag = buffer + i;
		tlv->len = tlv->tag + taglength(tlv->tag);
		tlv->val = tlv->len + lenlength(tlv->len);
		return true;
	}
	return false;
}

BOOL TlvTraverseTags(const uint8_t* buffer, size_t length, predicate_t p, void * ctx, BOOL recursive, Tlv_t* tlv, int level) {
	while(TlvParse(buffer, length, tlv)) {
		if(p(tlv, ctx, level))
			return true;

		size_t vallen = vallength(tlv->len);
		size_t buf_used = (tlv->val + vallen) - buffer;
		buffer += buf_used;
		length -= buf_used;

		if (recursive && CONSTRUCTED(tlv->tag))
			if(TlvTraverseTags(tlv->val, vallen, p, ctx, true, tlv, level+1))
				return true;
	}
	return false;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
BOOL isTagTraverse(Tlv_t* tlv, void * ctx, int _level) {
	return decodeTag(tlv) == *(uint16_t*)ctx;
}
#pragma GCC diagnostic pop

// Locate a TLV encoded data object in buffer of given length (if recursively in depth-first order)
//
// Parameters:
// buffer [IN]: The input buffer.
// length [IN]: The length of input buffer.
// tag [IN]: tag ID (up to 2 bytes) to find.
// recursive [IN]: search sub-nodes or not
// tlv [OUT]: A pointer to the found TLV object.
//
// Return value:
// TRUE: if the tag is found.
// FALSE: otherwise
//
// This function only supports Tag ID up to 2 bytes.
BOOL TlvSearchTag(const uint8_t* buffer, size_t length, uint16_t tag, BOOL recursive, Tlv_t* tlv) {
	return TlvTraverseTags(buffer, length, isTagTraverse, &tag, recursive, tlv, 0);
}

BOOL writeTagAndLength(Tlv_t* tlv, uint16_t tag, size_t length) {
	return encodeLength(tlv, length) && encodeTag(tlv, tag);
}

// Create a TLV container
//
// Parameters:
// tlv [IN]: Pointer to an un-initialized TLV structure. Upon return, it is
// initialized to represent a TLV container with given tag
// tag [IN]: The tag of TLV container
// buffer [IN]: The buffer to store entire TLV object

// length [IN]: The length of the buffer
BOOL TlvCreate(Tlv_t* tlv, uint16_t tag, uint8_t* buffer, size_t length) {
	tlv->buf = buffer;
	tlv->freeSpace = length;
	return writeTagAndLength(tlv, tag, 0) && TlvParse(buffer, length, tlv);
}

////
////// Add TLV data to the TLV container
BOOL TlvAddData(Tlv_t* tlv, uint16_t tag, const uint8_t* value, size_t valueLen) {
	tlv->freeSpace += (tlv->val - tlv->tag);
	size_t origBufLen = tlv->freeSpace;
	size_t parentLen = vallength(tlv->len);
	uint16_t parentTag = decodeTag(tlv);
	if (writeBytes(tlv, value, valueLen))
		if (writeTagAndLength(tlv, tag, valueLen))
			if(writeTagAndLength(tlv, parentTag, parentLen + (origBufLen - tlv->freeSpace)))
				return TlvParse(tlv->buf, parentLen + origBufLen, tlv);

	if (tlv->freeSpace != origBufLen) {
		// no room, but we wrote optimistically,
		// restore the originals, else the buffer is corrupt
		memset(tlv->buf, 0, origBufLen);
		tlv->freeSpace = origBufLen;
		encodeLength(tlv, parentLen);
		encodeTag(tlv, parentTag);
	}
	return false;
}

//// Add a TLV object to the TLV container
BOOL TlvAdd(Tlv_t* tlv, const Tlv_t* childTlv) {
	return TlvAddData(tlv, decodeTag(childTlv), childTlv->val, vallength(childTlv->len));
}


/* Bit array. Including regular (dense) bit array
 * and chunked bit array. Dense bit arrays cannot be
 * resized; chunked bit arrays can.
 * TODO: use bit array as default; fall back to
 * chunked bit array if domain is too big and
 * positive bits are sparse. */

#ifndef H_BITARRAY
#define H_BITARRAY

#include <stdlib.h>
#include "array.h"
#include "arena.h"
#include "dll.h"

#define BITCHUNK_SIZE 256

/* Capping the domain size to ~16M objects.
 * Each chunk consists of 256 bits, and a
 * bit array or chunked bit array can have
 * 2^16 chunks. */
typedef struct {
    uint16_t ind;       // Index in each (chunked) bit array
    uint64_t bits[4];   // 256 bits
} BitChunk;

typedef struct {
    size_t domain;
    size_t size;  // num of chunks
    BitChunk *head;
} BitArray;

typedef struct {
    size_t domain;
    DLL *chunks;
} ChunkedBitArray;

/* APIs */
/* TODO: chunked bit array stuff */
BitArray *bitarray(size_t domain, Arena *pool);  // returned bitarray has pointer to arena
ChunkedBitArray *cbitarray(size_t init_domain, size_t num_chunks); // Domain = 0 if unknown; Input is init number of chunks;

void set_bitarray(size_t ind, bool val, BitArray *bitarray);
void set_cbitarray(size_t ind, bool val, ChunkedBitArray *cbitarray);

bool eq_bitarray(BitArray *a, BitArray *b);
bool eq_cbitarray(ChunkedBitArray *a, ChunkedBitArray *b);

void reset_bitarray(BitArray *bitarray);
void reset_cbitarray(ChunkedBitArray *cbitarray);

void ones_bitarray(BitArray *bitarray);

void union_bitarray(BitArray *a, BitArray *b, BitArray *orig);
void union_cbitarray(ChunkedBitArray *a, ChunkedBitArray *b, ChunkedBitArray *orig);

void intersect_bitarray(BitArray *a, BitArray *b, BitArray *orig);
void intersect_cbitarray(ChunkedBitArray *a, ChunkedBitArray *b, ChunkedBitArray *orig);

void diff_bitarray(BitArray *a, BitArray *b, BitArray *orig);
void diff_cbitarray(ChunkedBitArray *a, ChunkedBitArray *b, ChunkedBitArray *orig);

void free_bitarray(BitArray *bitarray);  // Just a simple free
void free_cbitarray(ChunkedBitArray *bitarray); // free_dll, then free
#endif

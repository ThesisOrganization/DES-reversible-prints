#ifndef HEAP_H
#define	HEAP_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef int HEAP_TYPE;
#define MIN_HEAP 1
#define MAX_HEAP 0

#include "non_blocking_list.h"

typedef struct io_heap_entry{
	double key;
	nblist* payload;
	int position;
} io_heap_entry;

typedef struct {
	io_heap_entry * array;
	int size;
	int used;
	int is_min_heap;
} io_heap;

io_heap * io_heap_new(HEAP_TYPE type, int capacity);
HEAP_TYPE io_heap_type(io_heap * h);
double io_heap_peek(io_heap * h);
nblist_elem* io_heap_poll(io_heap* h);
io_heap_entry * io_heap_add(io_heap * h, nblist* payload);
double get_key_entry(io_heap_entry* ee);
int io_heap_size(io_heap * h);
void io_heap_delete(io_heap * h);
void io_heap_print(io_heap * h);

io_heap * array2io_heap(int * array, int size, HEAP_TYPE type);

void io_heap_sort(int * array, int size);

void io_heap_update_key(io_heap * h, io_heap_entry * e, double key);

#ifdef	__cplusplus
}
#endif

#endif	/* HEAP_H */


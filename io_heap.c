#include <stdlib.h>
#include <stdio.h>
#include "io_heap.h"
#include "iobuffer.h"
#include "dymelor.h"

io_heap * io_heap_new(HEAP_TYPE is_min_heap, int capacity) {

	io_heap * h = rsalloc(sizeof(io_heap));
	h->size = capacity;
	h->used = 0;
	h->array = rsalloc(sizeof(io_heap_entry)*h->size);
	h->is_min_heap = is_min_heap;

	return h;
}

HEAP_TYPE io_heap_type(io_heap * hh) {
	io_heap * h = hh;
	return h->is_min_heap;
}

double io_heap_peek(io_heap * hh) {
	io_heap * h = hh;
	return h->array[0].key;
}

static void io_heap_grow(io_heap * h) {
	h->array = rsrealloc(h->array, sizeof(io_heap_entry) * h->size * 2);
	h->size = h->size * 2;
}

static int get_parent_index(int i) {
	return (i - 1) / 2;
}

static int is_root_index(int i) {
	return i == 0 ? 1 : 0;
}

static int get_left_index(int i) {
	return 2 * i + 1;
}

static int get_right_index(int i) {
	return get_left_index(i) + 1;
}

static int is_leaf(io_heap * h, int i) {
	return get_left_index(i) >= h->used;
}

static int compare(io_heap * h, int i, int j) {

	if (h->is_min_heap == MIN_HEAP)
		return h->array[i].key < h->array[j].key;
	else
		return h->array[i].key > h->array[j].key;
}

static void exchange(io_heap * h, int i, int j) {

	//printf("Swap element index %d (%d) with index %d (%d)\n", i, h->array[i]->key, j, h->array[j]->key);

	io_heap_entry temp = h->array[i];
	h->array[i] = h->array[j];
	h->array[j] = temp;

	h->array[i].position = i;
	h->array[j].position = j;
}

io_heap_entry* io_heap_add(io_heap* h, nblist* payload)
{
	double key=((iobuffer*) payload->head->content)->timestamp;

	if(h->used >= h->size)
		io_heap_grow(h);

	io_heap_entry e;
	e.key = key;
  e.payload = payload;
	e.position = h->used;
	h->array[h->used] = e;

	int i = h->used;
	int j = get_parent_index(i);
	while(!is_root_index(i) && compare(h, i, j)) {
		exchange(h, i, j);
		i = j;
		j = get_parent_index(i);
	}

	h->used += 1;
	return &h->array[h->used - 1];
}

double get_key_entry(io_heap_entry * e) {
	return e->key;
}

int io_heap_size(io_heap * h) {
	return h->used;
}

static int get_max_child_index(io_heap * h, int k) {

	if(is_leaf(h, k))
		return -1;

	else {

		int l = get_left_index(k);
		int r = get_right_index(k);
		int max = l;
		if(r < h->used && compare(h, r, l))
			max = r;

		return max;
	}
}

static void io_heapify(io_heap * h, int i) {

	//printf("Heapify on %d\n", i);

	if(is_leaf(h, i))
		return;

	else {

		int j = get_max_child_index(h, i);
		//printf("max child is %d (%d)\n", j, h->array[j]->key);
		if (j < 0)
			return;

		if(!compare(h, i, j)) {
			exchange(h, i, j);
			io_heapify(h, j);
		}
	}
}

nblist_elem* io_heap_poll(io_heap * h) {

	//int key = -1;
	nblist_elem* payload;
	payload=NULL;

	if(h->used > 0) {
		io_heap_entry *e = &h->array[0];
		h->array[0] = h->array[h->used - 1];
		h->used -= 1;
		io_heapify(h, 0);
		//key = e->key;
		payload = nblist_pop(e->payload);
		//rsfree(e);
		io_heap_update_key(h,e,((iobuffer *) e->payload->head->content)->timestamp);
	}

	return payload;
}

void io_heap_delete(io_heap * hh) {

	io_heap * h = hh;

	//int i;
	//for (i = 0; i < h->used; i++)
		//rsfree(h->array[i]);

	rsfree(h->array);
	rsfree(h);

	return;
}
/*
io_heap * array2io_heap(int * array, int size, HEAP_TYPE is_min_io_heap) {

	_io_heap * h = io_heap_new(is_min_io_heap, size);

	//printf("Is Min Heap? %d\n", io_heap_is_min(h));

	int k;
	for (k = 0; k < size; k++) {

		_io_heap_entry * e = rsalloc(sizeof(_io_heap_entry));
		e->key = array[k];
		e->position = k;
		h->array[k] = e;

	}

	h->used = size;

	for(k = get_parent_index(size - 1); k >= 0; k--) {
		printf("iteration: %d\n", k);
		io_heapify(h, k);
	}

	return h;
}
*/
void io_heap_print(io_heap * hh) {

	io_heap * h = hh;
	int k;
	for (k = 0; k < h->used; k++)
		printf(" %lf", h->array[k].key);
	printf("\n\n");
}
/*
void io_heap_sort(int * array, int size) {

	_io_heap * h = array2io_heap(array, size, 0);
	io_heap_print(h);
	int i;
	for(i = size - 1; i > 0; i--) {

		exchange(h, 0, i);
		array[i] = h->array[i]->key;
		h->used -= 1;
		io_heapify(h, 0);

	}

	array[0] = h->array[0]->key;

	h->used = size;
	io_heap_delete(h);
}
*/
static void sift_up(io_heap * h, int i){

	while (i > 0) {

		int p = get_parent_index(i);
		if (compare(h, i, p)) {

			exchange(h, i, p);
			i = p;

		} else
			return;
	}
}

void io_heap_update_key(io_heap * hh, io_heap_entry * ee, double key) {

	io_heap * h = hh;
	io_heap_entry * e = ee;

	double oldKey = e->key;
	e->key = key;

	//printf("position: %d - oldKey: %d\n", e->position, oldKey);

	int keyDecrease = key < oldKey;

	if (io_heap_type(h) == MIN_HEAP) {

		if (keyDecrease)
			sift_up(h, e->position);
		else
			io_heapify(h, e->position);

	} else {

		if (!keyDecrease)
			sift_up(h, e->position);
		else
			io_heapify(h, e->position);

	}

}

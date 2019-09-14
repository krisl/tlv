#include <assert.h>

#define START_IDX (-1)
#define SIZE (1 << 1)
typedef struct { char a; } element_t;

element_t x = { .a = 'a' };
element_t * UART_READ_ADDR = &x;

int read = START_IDX, write = START_IDX;
element_t array[SIZE];

int used() {
	return write - read;
}

int full() {
	return used() == SIZE;
}

int empty() {
	return read == write;
}

int mask(int idx) {
	return idx & (SIZE - 1);
}

void push(element_t el)  {
	array[mask(write++)] = el;
}

element_t shift() {
	return array[mask(read++)];
}

element_t make_element() {
	element_t element = *UART_READ_ADDR;
	return element;
}



int main () {
	assert(!full()); assert(empty()); 

	for(int i = 0; i < SIZE - 1; i++) {
		push(make_element());
		assert(!full()); assert(!empty()); 
	}

	push(make_element());
	assert(full()); assert(!empty()); 

	shift();
	assert(!full()); assert(!empty()); 

	push(make_element());
	assert(full()); assert(!empty()); 

	for(int i = 0; i < SIZE - 1; i++) {
		shift();
		assert(!full()); assert(!empty()); 
	}

	shift();
	assert(!full()); assert(empty()); 
}

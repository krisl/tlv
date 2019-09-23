tlv: main.c tlv.c tlv.h
	gcc main.c tlv.c -I. -Wall -Werror -Wextra -Wshadow -std=c11 -Wpedantic -o tlv

ringbuffer:
	gcc ringbuffer.c -Wall -Werror -Wextra -Wshadow -std=c11 -Wpedantic 


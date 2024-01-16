zex: zex.c
	$(CC) zex.c -o zex -Wall  -Wextra -pedantic -std=c99

test: zex
		./test.sh

clean: 
		rm -f zex *.o *~ tmp*

.PHONY: test clean


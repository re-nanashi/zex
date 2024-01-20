zex: zex.c
	$(CC) zex.c -o zex -pthread -Wall -Wextra -pedantic  -std=c99

test: test.c
	$(CC) test.c -o test -Wall -Wextra -pedantic -std=c99


clean: 
		rm -f zex *.o *~ tmp* dump test *.o *~ tmp*

.PHONY: test clean


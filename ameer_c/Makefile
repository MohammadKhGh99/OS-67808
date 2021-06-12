FLAGS = -Wall -Wextra -Wvla -Werror -g -lm -std=c99


.PHONY: all clean

all: libhashmap.a libhashmap_tests.a

libhashmap.a: pair.o vector.o hashmap.o
	ar rcs libhashmap.a pair.o vector.o hashmap.o

libhashmap_tests.a: test_suite.o
	ar rcs libhashmap_tests.a test_suite.o

pair.o: pair.h pair.c
	gcc $(FLAGS) -c pair.c

vector.o: vector.h vector.c
	gcc $(FLAGS) -c vector.c

hashmap.o: pair.h vector.h hashmap.h hashmap.c
	gcc $(FLAGS) -c hashmap.c

test_suite.o: hash_funcs.h hashmap.h test_suite.h test_suite.c
	gcc $(FLAGS) -c test_suite.c

clean:
	rm -f *.o *.a
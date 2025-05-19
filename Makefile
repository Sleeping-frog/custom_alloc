TESTS_OBJ := $(wildcard test*.o)
TESTS_C := $(wildcard tests/test*.c)

.PHONY: tests.o

all: main.c alloc.h liballoc.a tests/tests.h tests.o
	gcc main.c $(TESTS_OBJ) -L. -lalloc -o program -g

liballoc.a: alloc.h alloc.o vector.o hashTable.o
	ar rcs $@ $^

alloc.o: alloc.h alloc.c
	gcc -c alloc.c -o alloc.o -g

vector.o: Containers/vector.h Containers/vector.c
	gcc -c Containers/vector.c -o vector.o -g

hashTable.o: Containers/hashTable.h Containers/hashTable.c
	gcc -c Containers/hashTable.c -o hashTable.o -g

tests.o: tests/tests.h $(TESTS_C)
	gcc -c $^ -g

clean:
	rm *.o liballoc.a program
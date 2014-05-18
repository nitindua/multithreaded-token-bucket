tokenbucket: tokenbucket.o my402list.o
	gcc -o tokenbucket -g -pthread tokenbucket.o my402list.o -lm -lpthread

tokenbucket.o: tokenbucket.c my402list.c
	gcc -g -c -Wall tokenbucket.c -D_POSIX_PTHREAD_SEMANTICS -pthreads

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c
	
clean:
	rm *.o tokenbucket	

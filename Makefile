all:
	gcc config.h lrustack.h lrustack.c mshr.h mshr.c cacheset.h cacheset.c cachebank.h cachebank.c -o cachesim
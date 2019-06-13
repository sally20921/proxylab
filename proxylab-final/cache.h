#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


struct cache_block
{
	struct cache_block *next, *prev;
	long LRU;
	char *block; 
	ssize_t block_size;
	char *request;
};

struct CacheList {
	struct cache_block *head;
	ssize_t size;
};


void cache_init();
int remove_block(struct cache_block * blo);
struct cache_block * alloc_block(int size);
int read_cache(char *request, char *block);
void write_cache(char *request, char *block, int block_size);

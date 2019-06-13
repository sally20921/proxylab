#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400


struct cache_block
{
    struct cache_block *next, *prev;
    long LRU;
    char *block; ssize_t block_size;
    char *request;
};


struct cache_block * alloc_block(int size);
void cache_init();
int reader_check(char *request, char *block);
int remove_block(struct cache_block * blo);
void writer_check(char *request, char *block, int block_size);

#include "cache.h"

//multiple threads must be able to simultaneously read from the cache
//only one thread should be permitted to write to the cache at a time

//Implements First Readers-Writers Problem solution

static long lru;
struct CacheList *cache;

static int readcnt;
sem_t mutex,w;

void cache_init()
{
    lru = 0;
    
    cache = (struct CacheList *)Malloc(sizeof(struct CacheList));
    cache->head = NULL;
    cache->size = 0;
    
    Sem_init(&mutex, 0, 1);
    Sem_init(&w, 0, 1);
    readcnt = 0;
}

struct cache_block *alloc_block(int size)
{
    struct cache_block *temp;
    temp = (struct cache_block *)Malloc(sizeof (struct cache_block));
    temp->LRU = ++lru;
    
    temp->block = (char *)Malloc(size);
    temp->block_size = size;
    return temp;
}

int remove_block(struct cache_block *temp)
{
    Free(temp->block);
    Free(temp->request);
    Free(temp);
    return 1;
}

int read_cache(char *request, char *buf)
{
    int len = 0;
    
    P(&mutex);
    readcnt++;
    if (readcnt == 1) { //first in
        P(&w);
    }
    V(&mutex);
    
    //critical section, reading happens
    struct cache_block *temp = cache->head;
    while (temp != NULL) {//traverse through cache list
        if (!strncasecmp(temp->request, request, strlen(request)))
        {
            //found it, read it, copy it into buf
            temp->LRU = ++lru;
            memcpy(buf, temp->block, temp->block_size);
            len = temp->block_size;
            break;
        }
        temp = temp->next;
    }
    
    P(&mutex);
    readcnt--;
    if (readcnt == 0) { //last out
        V(&w);
    }
    V(&mutex);
    
    return len;
}

void write_cache(char *request, char *block_data, int block_size)
{
    struct cache_block *temp;
    P(&w);
    while (cache->size + block_size >= MAX_CACHE_SIZE)
    {
        //evict according to LRU policy if it exceeds MAX_CACHE_SIZE
        struct cache_block *evict = NULL;
        
        //for (temp = head; temp; temp = temp->next)
        temp = cache->head;
        while ( temp != NULL)
        {
            if (temp->LRU < evict->LRU) {
                evict = temp;
                //find the least recently used block
            }
            temp = temp->next;
        }
        
        cache->size -= evict->block_size;
        
        //evicting least recently used block
        if (evict == cache->head) {
            cache->head = evict->next;
            if (cache->head) {
                cache->head-> prev = NULL;
            }
        }
        else {
            evict->prev->next = evict->next;
            if (evict->next) {
                evict->next->prev = evict->prev;
            }
        }
        
        (void) remove_block(evict);
    }
    
    //adding new cache block
    cache->size += block_size;
    
    temp = alloc_block(block_size);
    
    //void *memcpy(void *destination, const void *source, size_t num);
    memcpy(temp->block, block_data, block_size);
    
    temp->request = (char *) Malloc(strlen(request)+1);
    strncpy(temp->request, request, strlen(request));
    
    if (cache->head) {
        cache->head->prev = temp;
        temp->next = cache->head;
        temp->prev = NULL;
        cache->head = temp;
    }
    else { //if it is the first item
        cache->head = temp;
        cache->head->next = cache->head->prev = NULL;
    }
    
    V(&w);
}

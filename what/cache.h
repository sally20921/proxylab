/*
 * Name:Yuchen Tian,Andrew ID: yuchent
 * Name:Guanyu Wang,Andrew ID: guanyuw
 */


#ifndef CACHE_H_INCLUDED
#define CACHE_H_INCLUDED
#endif

#include "csapp.h"
#include <time.h>

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct cb{
    char *content;
    unsigned blocksize;
    char *id;
    time_t timestamp;
    struct cb* next;
    struct cb* prev;
}cache_block;

typedef struct {
    unsigned whole_cache_size;
    cache_block* head;
    cache_block* tail;
}cache_list;

cache_block* build_cache(char* id, char* content, unsigned blocksize);
void insert_cache(cache_list* cl,cache_block* block);
void delete_cache(cache_list* cl);
void evict_cache(cache_list* cl, unsigned blocksize);
void init_cache_list(cache_list* cl);
cache_block* find_cache(cache_list* cl,char* id);
void print_list(cache_list* cl);

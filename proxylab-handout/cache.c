#include "csapp.h"
#include "cache.h"

/*#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct CachedItem CachedItem;
struct CachedItem {
    char *url;
    void *item;
    size_t size;
    struct CachedItem *next;
    struct CachedItem *prev;
};
typedef struct {
    size_t size;
    CachedItem *head;
    CachedItem *tail;
}CacheList;

//static struct CacheList* cache;
int readcnt;
sem_t m, w, r;

extern void cache_init(CacheList *list);
extern void cache_url(char *url, void *item, size_t size, CacheList *list);
extern void evict(CacheList *list);
extern CachedItem *find(char *url, CacheList *list);
extern void move_to_front(char *url, CacheList *list);
//extern void print_urls(CacheList *list);
extern void cache_destruct(CacheList *list);*/

int readcnt;
sem_t m, w, r;

void cache_init(CacheList *list) {
    //list = (CacheList*) Calloc(1, sizeof(CacheList));
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
    
    Sem_init(&m, 0, 1);
    Sem_init(&w, 0, 1);
    Sem_init(&r, 0, 1);
    
}
void cache_destruct(CacheList *list) {
    P(&w);
    CachedItem *node = list->head;
    while (node != NULL) {
        Free(node->url);
        Free(node->item);
        Free(node);
        node = node->next;
    }
    V(&w);
}

CachedItem *find(char *url, CacheList *list) {
    P(&r);
    CachedItem *node = list->head;
    while (node != NULL) {
        if (strcmp(node->url, url) == 0) {
            return node;
        }
        node = node->next;
    }
    V(&r);
    return NULL;
}

void evict(CacheList *list) { //Least Recently Used Policy
    P(&w);
    while (list->size > MAX_CACHE_SIZE) {
        CachedItem* node = list->tail;
        list->size -= node->size;
        list->tail = node->prev;
        list->tail->next = NULL;
        
        Free(node->url);
        Free(node->item);
        Free(node);
        
    }
    V(&w);
    
}

/*stores value in cache*/
void cache_url(char *url, void *item, size_t size, CacheList *list) {
    P(&w);
    list->size += size;
    if (list->size > MAX_CACHE_SIZE) {
        evict(list);
    }
    CachedItem *node = (struct CachedItem *) Malloc(sizeof(struct CachedItem));
    strcpy(node->url, url);
    node->item = item;
    node->size = size;
    node->prev = node->next = NULL;
    if (list->head) { //put it in the front of the list, LRU policy
        node->next = list->head;
        list->head->prev = node;
        list->head = node;
    }
    else { //if the list is empty
        list->tail = node;
        list->head = node;
    }
    
    V(&w);
}

/*CachedItem *find(char *url, CacheList *list) {
    CachedItem *node = list->head;
    while (node != NULL) {
        if (strcmp(node->url, url) == 0) {
            return node;
        }
        node = node->next;
    }
    return NULL;
}*/

void move_to_front(char *url, CacheList *list) {
    P(&r);
    CachedItem *node = find(url, list);
    if (!node) { //item is null
        return; //already in front
    }
    else if (!node->prev) { //item is head
        return; //already in front
    }
    else if (node->next) { //item has next
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }
    else { //item is tail
        node->prev->next = NULL;
        list->tail = node->prev;
    }
    node->prev = NULL;
    node->next = list->head;
    list->head = node; //move node to the front of the list
    if (node->next) {
        node->next->prev = node;
    }
    else { //single item
        list->tail = node;
    }
    V(&r);
}



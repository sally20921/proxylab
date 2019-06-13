#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
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

extern void cache_init(CacheList *list);
extern void cache_url(char *url, void *item, size_t size, CacheList *list);
extern void evict(CacheList *list);
extern CachedItem *find(char *url, CacheList *list);
extern void move_to_front(char *url, CacheList *list);
//extern void print_urls(CacheList *list);
extern void cache_destruct(CacheList *list);



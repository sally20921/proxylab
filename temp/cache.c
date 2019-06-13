#include "cache.h"

static size_t cache_size = 0;
static cache_obj* last = NULL;

// checks cache for an object. Ages all other objects as well
cache_obj* in_cache(char* name, int num_entries, cache_obj* cache){
    int i = 0;
    cache_obj* after;
    while(i < num_entries){
        if(!strcmp(name, cache->name)){
            // accessed most recentely, reset age
            cache->age = 0;
            after = cache->next;
            // age the rest
            while(after != NULL){
                after->age++;
                after = after->next;
            }
            return cache;
        }
        if(cache->next == NULL)
            break;

        cache->age++;
        cache = cache->next;
    }
    return NULL;
}

/* initializes cache with first object inserted
 * Allocates memory for cache, name, and buf
 */
cache_obj* cache_init(cache_obj* cache, char* name, char buf[]){
    size_t alloc_size = (strlen(name)+1) + (strlen(buf)+1) + (2*sizeof(int)) + sizeof(cache_obj*);
    cache = Malloc(sizeof(cache_obj));

    cache->name = Malloc(strlen(name)+1);
    strcpy(cache->name, name);

    cache->buf = Malloc(strlen(buf)+1);
    strcpy(cache->buf, buf);

    cache->obj_size = alloc_size;
    cache->age = 0;
    cache->next = NULL;
    last = cache;
    cache_size = alloc_size;
    return cache;
}

/* Appends entry to the end of the cache, updates the last pointer
 */
int set_next(cache_obj* entry){
    if(last->next == NULL){
        //return to be read
        last->next = entry;
        last = entry;
        return 0;
    }
    return 1;
}

/* adds object to end of cache list. Evicts if max cache size is reached
 * Allocates memory for Cache (if necessary), name, and buf
 * Frees name and buf before reallocating using free_spot()
 */
cache_obj* add_obj(cache_obj* cache, int num_entries, char* name, char buf[]){
    cache_obj* spot;
    int next_set;
    size_t alloc_size = (strlen(name)+1) + (strlen(buf)+1) + (2*sizeof(int)) + sizeof(cache_obj*);

    if(cache_size == MAX_CACHE_SIZE) {
        spot = cache_evict(cache, num_entries, alloc_size);
        free_spot(spot);
    } else {
        spot = Malloc(sizeof(cache_obj));
    }

    spot->name = Malloc(strlen(name)+1);

    spot->buf = Malloc(strlen(buf)+1);

    strcpy(spot->name, name);
    strcpy(spot->buf, buf);
    cache_size += alloc_size;
    spot->obj_size = alloc_size;
    spot->age = 0;
    spot->next = NULL;
    next_set = set_next(spot);

    if(next_set == 1) //set_next failed
        return NULL;
    return spot;
}

/* free the allocated strings in the cache entry and subtract the size from the cache
 * frees name and buf
 */
void free_spot(cache_obj* entry){
    cache_size -= entry->obj_size;
    Free(entry->name);
    Free(entry->buf);
}

/* destruct cache
 * frees cache
 */
void cache_deinit(cache_obj* cache, int num_entries){
    int i;
    cache_obj* next;
    for(i = 0; i < num_entries; i++){
        next = cache->next;
        free_spot(cache);
        Free(cache);
        if(next == NULL)
            break;
        else cache = next;
    }
    cache_size = 0;
}

// write to cache, will only add object since update is only to add entry
cache_obj* cache_write(char* name, char buf[], int num_entries, cache_obj* cache){
    // first cache entry
    if(cache != NULL){
        add_obj(cache, num_entries, name, buf);	
        return cache;
    }
    return cache_init(cache, name, buf);
}

// evict the eldest cache object that is greater than or equal to the new entry's size
cache_obj* cache_evict(cache_obj* cache, int num_entries, size_t alloc_size){
    int i = 0;
    cache_obj* eldest = cache;

    while(i < num_entries){
        if(eldest->age < cache->age && alloc_size <= cache->obj_size)
            eldest = cache;
        else if (cache->next == NULL)
            break;
        cache = cache->next;
        i++;
    }
    return eldest;
}                                                                                                                                                              

/*
 * Name:Yuchen Tian,Andrew ID: yuchent
 * Name:Guanyu Wang,Andrew ID: guanyuw
 */

#include "csapp.h"
#include "cache.h"


void init_cache_list(cache_list* cl)
{
        cl -> whole_cache_size = 0;

        cl -> head = build_cache("0",NULL,0);
        cl -> tail = build_cache("0",NULL,0);
        //printf("head is %p\n",cl->head);
        //printf("head %d tail %d\n",cl->head->blocksize,cl->tail->blocksize);
        cl -> head -> next = cl->tail;
        cl -> tail -> prev = cl->head;
}

cache_block* build_cache(char *id, char* content, unsigned blocksize)
{
        //printf("build\n");
        cache_block *block;
        block = (cache_block*)Malloc(sizeof(cache_block));
        //printf("size %d",sizeof(cache_block));
        char* new_id = (char*)Malloc(sizeof(char)*MAXLINE);
        //printf("strlen %d %d\n ",strlen(new_id),*(new_id+MAXLINE));
        //printf("id strlen %d\n ",strlen(id));
        //*new_id = 0;
        //char new_id[MAXLINE];
        //memcpy(new_id,id,strlen(id)+1);
        //printf("id %s",id);

        //block -> id = id;
        if(id != NULL){
        strcpy(new_id,id);
        block -> id = new_id;}
        else block -> id = NULL;
        block -> content = content;
        block -> blocksize = blocksize;
        block -> prev = NULL;
        block -> next = NULL;
        //printf("end built\n");
        return block;
}

void insert_cache(cache_list* cl,cache_block* block)
{
        block -> prev = cl -> head;
        block -> next = cl -> head -> next;
        cl -> head -> next -> prev = block;
        cl -> head -> next = block;
        cl -> whole_cache_size += block -> blocksize;
}

void delete_cache(cache_list* cl)
{
        if (cl->tail->prev == cl->head)
        {
                return;
        }

        cache_block* tp;
        tp = cl -> tail -> prev;
        cl -> tail -> prev = tp -> prev;
        cl -> tail -> prev -> next = cl -> tail;
        cl -> whole_cache_size -= tp -> blocksize;
        //if(to_delete)
        Free(tp);

}

void evict_cache(cache_list* cl, unsigned blocksize)
{
        while(cl->whole_cache_size + blocksize > MAX_CACHE_SIZE)
        {
                //printf("evict!\n");
                delete_cache(cl);
        }
}

cache_block* find_cache(cache_list* cl,char* id)
{
        //printf("")
        //printf("req id %s\n",id);
        print_list(cl);
        if(cl->head->next == cl -> tail) return NULL;
        cache_block* block;
        block = cl -> head -> next;
        for(;block!=NULL;block = block->next)
        {
                //printf("block id %s,req id %s\n",block->id,id);
                if(!strcmp(block->id,id))
                {

                    block->prev->next = block -> next;
                    block->next->prev = block ->prev;
                    insert_cache(cl,block);
                    return block;
                }


        }
        return NULL;
}


void print_list(cache_list* cl)
{
        cache_block* block;
        block = cl -> head -> next;
        for(;block!=NULL;block = block->next)
        {
                //printf("block id %s\n",block->id);

        }
}

/* $begin sbuf.h */
#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"

struct sbuf_t{
    int *buf;
    int n;
    int front;
    int rear;
    sem_t mutex;
    sem_t slots;
    sem_t items;
};
typedef struct sbuf_t sbuf_t;

/* Buffer array */
/* Maximum number of slots */
/* buf[(front+1)%n] is first item */
/* buf[rear%n] is last item */
/* Protects accesses to buf */
/* Counts available slots */
/* Counts available items */

void sbuf_init(sbuf_t* sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

#endif /* __SBUF_H__ */
/* $end sbuf.h */


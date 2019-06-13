#include <stdio.h>
//#include <stdlib.h>
//#include <assert.h>
//#include <string.h>

#include "csapp.h"
#include "cache.h"

/* Recommended max cache and object sizes */
//#define MAX_CACHE_SIZE 1049000
//#define MAX_OBJECT_SIZE 102400

/*#define DEFAULT_HTTP_PORT 80
#define CACHE_ENABLED 1*/

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
//static const char *host_hdr = "Host: %s\r\n";
//static const char *request_hdr = "GET %s HTTP/1.0\r\n";

//static const char *default_port = "80";
//static const char *version = "HTTP/1.0";

typedef struct {
    char *url;
    //char *request;
    //char *method;
    char *host;
    char *port;
    char *path;
    //char *version;
    char *host_hdr;
} Request;

static struct CacheList *cache = (CacheList *) Calloc(1, sizeof(CacheList));;
static sem_t mutex;

/*Helper function*/
void *handle_client(void *vargp);
void initialize_struct(Request *req);
void parse_request(char request[MAXLINE], Request *req);
//void parse_absolute(Request *req);
//void parse_relative(Request *req);
//void parse_header(char header[MAXLINE], Request *req);
void assemble_request(Request *req, char *request);
int get_from_cache(Request *req, int clientfd);
void get_from_server(Request *req, char request[MAXLINE], int clientfd, rio_t rio_to_client);
//void close_wrapper(int fd);
//void print_full(char *string);
//void print_struct(Request *req);

void initialize_struct(Request *req) {
    //req = (Request *) malloc(sizeof(Request));
    strcpy(req->url, "");
    strcpy(req->host, "");
    strcpy(req->port, "80");
    strcpy(req->path, "/");
}

void assemble_request(Request *req, char *request) {
    strcpy(request, "GET ");
    strcat(request, req->path);
    strcat(request, " HTTP/1.0\r\n");
    strcat(request, req->host_hdr);
    strcat(request, "\r\n");
}

int main(int argc, char *argv[])
{
    int listenfd, connfd;
    //int port;
    socklen_t clientlen;
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc !=2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }

    Signal(SIGPIPE, SIG_IGN);

    cache_init(cache);

    //port = atoi(argv[1]);
   
    if ((listenfd = Open_listenfd(argv[1])) < 0 ) {
        //fprintf(stderr, "failed to listen to port %s \n", argv[1]);
        //exit(1);
        return 0;
    }

    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Malloc(sizeof(int));
        if((connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen))<0) {
            return 0;
        }

        /* run client connection on another thread */
        Pthread_create(&tid, NULL, handle_client, (void *)connfd);

    }
    printf("%s", user_agent_hdr);
    cache_destruct(cache);
    return 0;
}

void *handle_client(void *vargp) {
    Pthread_detach(Pthread_self());
    
    int connfd = *((int *)vargp);
    //Pthread_detach(Pthread_self());
    //sockaddr_in clientaddr = *((sockaddr_in *)vargp);
    
    Request *req = (Request *) Malloc(sizeof(Request));
    initialize_struct(req);
    
    rio_t rio_to_client;
    char request[MAXLINE];
    Rio_readinitb(&rio_to_client, connfd);
    Rio_readlineb(&rio_to_client, request, MAXLINE);
    parse_request(request, req);
    
    char new_request[MAXLINE];
    assemble_request(req, new_request);
    
    if (!get_from_cache(req, connfd)) {
        get_from_server(req, new_request, connfd, rio_to_client);
    }
    
   
    Close(connfd);
    return NULL;
}
void parse_request(char request[MAXLINE], Request *req) {
    char *url;
    char *host;
    char *port;
    char *path;
    //char *host_hdr;
    
    char temp[10];
    sscanf(request, "%s %s %s", temp, req->url, temp);
    strcpy(url,req->url);
    
    char *saved;
    strtok_r(url, "/", &saved);
    port = strtok_r(saved, "/", &path);
    host = strtok_r(port, ":", &port);
    
    strcat(req->host, host);
    strcat(req->path, path);
    strcat(req->host_hdr, "Host: ");
    strcat(req->host_hdr, host);
    strcat(req->host_hdr, "\r\n");
    strcpy(req->port, port);
    
}

void get_from_server(Request *req, char request[MAXLINE], int clientfd, rio_t rio_to_client) {
    
    int serverfd = Open_clientfd(req->host, req->port);
    rio_t rio_to_server;
    Rio_readinitb(&rio_to_server, serverfd);
    Rio_writen(serverfd, request, strlen(request));
    
    char *buf = Malloc(MAXLINE);
    int n = 0;
    int size = 0;
    char *temp = Calloc(1, MAX_CACHE_SIZE);
    char *p = temp;
    int can_cache = 1;
 	while ((n = Rio_readnb(&rio_to_server, buf, MAXLINE)) != 0)
  	{
    	Rio_writen(clientfd, buf, n);
        size += n;
        if (size <= MAX_CACHE_SIZE) {
            memcpy(p, buf, n);
            p += n;
        }
        else {
            can_cache = 0;
        }
	}
    if (can_cache) {
        cache_url(req->url, temp, size, cache);
    }
	
  	Close(serverfd);
  	Free(buf);

}

int get_from_cache(Request *req, int clientfd) {
    struct CachedItem *node = find(req->url, cache);
    if(node)
	{
        move_to_front(req->url, cache);
		rio_writen(clientfd, node->item, node->size);
		return 1;
	}
	return 0;
}



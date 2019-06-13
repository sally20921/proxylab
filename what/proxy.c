/*
 * Name:Yuchen Tian,Andrew ID: yuchent
 * Name:Guanyu Wang,Andrew ID: guanyuw
 */


#include <stdio.h>
#include <time.h>
#include "csapp.h"
#include "cache.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define DEBUGx
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif

/*************************
    Default Header Settings
***************************/

static const char *default_user_agent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *default_accept = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *default_accept_encoding = "Accept-Encoding: gzip, deflate\r\n";
static const char *default_connect = "Connection: close\r\n";
static const char *default_proxy = "Proxy-Connection: close\r\n";
static const char *default_version = "HTTP/1.0\r\n";
static sem_t mutex;
static cache_list *Cache_list;
pthread_rwlock_t rwlock;

/*Define request struct*/
typedef struct {
        char* host; /*string host*/
        int port; /*port number*/
        char* request; /*HTTP request body*/
        char id[MAXLINE];
}req_t;

/*Helper function*/
void doit(int fd);
void get_request(rio_t *rp, req_t *req);
void extract_header(char *raw, char *header, char *value);
void add_to_header(char *key, char *value, char *header);
void append_header(char *hdr_line,char *header);
int parse_uri(char *uri, char *host, int *port,char *short_uri);
int parse_reqline(char *header,char *reqline, char* host, int *port);
void set_header(char *key, char *value, char *header);
void extract_host_port(char *value,char* host,int* port);
void forward_to_server(int server_fd);
void *thread(void* vargp);
int iRio_writen(int fd, void *usrbuf, size_t n);

/*
 *  Copy from tiny web server, listen the port speicified
 *  and repsonse through doit.
 */
int main(int argc, char **argv)
{

        int listenfd, port, clientlen;
        int* connfdp;
        struct sockaddr_in clientaddr;
        pthread_t tid1;
        Sem_init(&mutex,0,1);
        Cache_list = (cache_list*)Malloc(sizeof(Cache_list));
        init_cache_list(Cache_list);
        pthread_rwlock_init(&rwlock, NULL);
        Signal(SIGPIPE, SIG_IGN);

        if (argc != 2) {
                fprintf(stderr, "usage: %s <port>\n", argv[0]);
                exit(1);
        }
        port = atoi(argv[1]);

        listenfd = Open_listenfd(port);
        while (1) {
                //printf("new connection!\n");
                clientlen = sizeof(clientaddr);
                connfdp = Malloc(sizeof(int));
                *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);
                //doit(connfd);
                //Close(connfd);
                //doit(*connfdp);
                Pthread_create(&tid1,NULL,thread,connfdp);
                //Pthread_create(&tid2,NULL,thread,NULL);
        }

        return 0;
}

/*
 * Get request from the client side, and modify the header due to
 * the requirement in proxylab handout.
 */

void get_request(rio_t *rp, req_t* req)
{
        char buf[MAXLINE]; /*line buffer*/
        char key[MAXLINE],value[MAXLINE];/*Give a header, such as "Host: www.baidu.com\r\n",
                                           key is the header's key, ie "Host"; value is the ac
                                           according value, ie "www.baidu.com"*/
        char raw[MAXLINE]; /*raw request*/
        int has_host = 0; /* check whether the raw header has Host header*/

        char request[MAXLINE]; /*request body*/
        req->request = request;

        //printf("request address %p\n",request);

        /*initialize all!Important!*/
        *request = 0;
        *raw = 0;
        *buf = 0;
        *key = 0;
        *value = 0;


        char host[MAXLINE];
        int port;

        /*set default host address value*/
        *host = 0;
        port = 80;

        //char method[MAXLINE], uri[MAXLINE], version[MAXLINE],short_uri[MAXLINE];

        Rio_readlineb(rp, buf, MAXLINE);
        strcpy(req->id,buf);

        strcat(raw,buf);

        /*Parse the first line of a request, ie "GET http://www.baidu.com/ HTTP/1.1\r\n"
         * and extract the host and port.
         */
        parse_reqline(request,buf,host,&port);

        /*Set the default headers*/
        append_header((char *)default_user_agent,request);
        append_header((char *)default_accept,request);
        append_header((char *)default_accept_encoding,request);
        append_header((char *)default_connect,request);
        append_header((char *)default_proxy,request);


        while (strcmp(buf, "\r\n")) {
                *key = '\0';
                *value = '\0';
                Rio_readlineb(rp, buf, MAXLINE);
                //dbg_printf("raw header line %slen%d\n",buf,strlen(buf));
                //printf("%s",buf);
                strcat(raw,buf);
                //printf("raw %s len %d\n",buf,strlen(buf));
                if (!strcmp(buf, "\r\n")) {
                        //printf("break!\n");
                        break;
                }

                /*extract headers' key and value from a header line*/
                extract_header(buf,key,value);
                if (*key != '\0' && *value!='\0') {

                        /*If the raw request has Host header itself, use it*/
                        if (!strcmp(key,"Host")) {
                                dbg_printf("contain host\n");

                                /*Extract host and port from the value of the Host header,
                                * which is different from extract host and port from request line;
                                */
                                extract_host_port(value,host,&port);
                                has_host = 1;
                        }

                        set_header(key,value,request);
                }
        }
        /*If request doesn't have a Host header, using the host and port from the first request line
          and put them together as a Host header.*/
        if (!has_host) {
                char host_hdr[MAXLINE];
                if (port != 80) {
                        sscanf(host_hdr,"Host: %s:%d\r\n",host,&port);
                } else {
                        sscanf(host_hdr,"Host: %s\r\n",host);
                }
                append_header(host_hdr,request);
        }

        req->host = host;
        req->port = port;
        append_header("\r\n",request);
        //printf("raw:\n%s",raw);
        //printf("modified:\n%s",request);
        return;
}

void doit(int fd)
{
        rio_t rio;
        rio_t s_rio;
        req_t req;
        int server_fd;
        //char request[MAXLINE];
        Rio_readinitb(&rio,fd);

        //printf("bbfore reqs!\n");
        print_list(Cache_list);

        get_request(&rio,&req);

        //printf("before request\n");
        print_list(Cache_list);

        //printf("req id is %s",req.id);

        /*readlock area*/
        pthread_rwlock_rdlock(&rwlock);
        //printf("try find\n");
        cache_block *block;
        block = find_cache(Cache_list,req.id);
        if(block!=NULL)
        {
            //print_list(Cache_list);
            Rio_writen(fd,block->content,block->blocksize);
            //printf("hit!\n");
            pthread_rwlock_unlock(&rwlock);
            return;
        }
        pthread_rwlock_unlock(&rwlock);

        /*readlock area*/


        /*Connect to the server*/
        server_fd = Open_clientfd(req.host,req.port);
        if (server_fd == -1) return;
        Rio_readinitb(&s_rio,server_fd);

        /*Write modified request to server*/
        int server_connect = 0;
        server_connect = iRio_writen(server_fd,req.request,strlen(req.request));
        //if(errno == EPIPE) printf("Trace host %s\n",req.host);

        if (server_connect < 0) {
                Close(server_fd);
                return;
        }

        /*Forward response from the server to the client through connfd*/
        ssize_t nread;
        char buf[MAX_OBJECT_SIZE];
        unsigned cache_size = 0;

        while ((nread = Rio_readnb(&s_rio,buf,MAX_OBJECT_SIZE))!=0) {
                iRio_writen(fd,buf,nread);
                cache_size += nread;

        }

        if((cache_size!=0) && (cache_size <= MAX_OBJECT_SIZE))
        {
            /*write lock*/

            pthread_rwlock_wrlock(&rwlock);
            //printf("try write\n");
            //printf("cacheable!");
            evict_cache(Cache_list,cache_size);
            cache_block *block;
            char* content;
            content = (char*)Malloc(cache_size*sizeof(char));
            memcpy(content,buf,cache_size);
            block = build_cache(req.id,content,cache_size);
            insert_cache(Cache_list,block);
            //printf("after cached!\n");
            //print_list(Cache_list);
            pthread_rwlock_unlock(&rwlock);
            /*Write lock*/
        }

        Close(server_fd);

        //printf("in req:\n%s%s %d\n",req.request,req.host,req.port);
}


/*Add new header line to request*/
void append_header(char *hdr_line,char *header)
{
        strcat(header,hdr_line);
}


/*Extract key and value from a raw header line*/
void extract_header(char *raw, char *header, char *value)
{
        char *ptr;
        char t_value[MAXLINE];

        /*Header is splitted using a character ":"*/
        ptr = strstr(raw,":");
        if (ptr != NULL) {
                *ptr = 0; /*break the raw line into to strings*/
                strcpy(header,raw);
                strcpy(t_value,ptr+2);/*Ignore the blank behind ":"*/
                ptr = strstr(t_value,"\r");
                *ptr = 0; /*Eliminate "\r\n"*/
                strcpy(value,t_value);
        } else return;
}


/*Put key and value into a header line*/
void add_to_header(char *key, char *value, char *header)
{
        char hdr_line[MAXLINE];
        sprintf(hdr_line,"%s: %s\r\n",key,value);
        dbg_printf("formed header:%slen:%d\n",hdr_line,strlen(hdr_line));
        append_header(hdr_line,header);

}

/*Parse the uri field from the first request line.
 * Note: If the uri begins with "http://" the extract host and port from it
 * and cut the uri off the host domain, living only the shorter uri.
 * For example if the raw uri is http://www.baidu.com:8080/index.html/
 * the short_uri will be /index.html/ only
 */
int parse_uri(char *uri, char *host, int *port, char *short_uri)
{
        *host = 0;
        *port = 80;
        char *http_ptr,*slash_ptr;
        char host_t[MAXLINE],port_str[MAXLINE];
        http_ptr = strstr(uri,"http://");
        if (http_ptr == NULL) {
                strcpy(short_uri,uri);
                return 0;
        } else {
                /*Ignore http://*/
                http_ptr += 7;

                /*Find the first '/' to extract host*/
                slash_ptr = strchr(http_ptr,'/');
                *slash_ptr = 0;

                /*extract host to host_t*/
                strcpy(host_t,http_ptr);

                /*restore the short_uri*/
                *slash_ptr = '/';
                strcpy(short_uri,slash_ptr);


                /*If contains a port number, extract it*/
                char *ptr;
                ptr = strstr(host_t,":");
                if (ptr != NULL) {
                        *ptr = 0;
                        strcpy(port_str,ptr+1);
                        *port = atoi(port_str);
                }

                strcpy(host,host_t);
                return 1;

        }
}


/*Parse the first line of the request*/
int parse_reqline(char *header,char *reqline, char* host, int *port)
{
        char method[MAXLINE], uri[MAXLINE], version[MAXLINE],short_uri[MAXLINE];
        sscanf(reqline, "%s %s %s", method, uri, version);

        int found_host;
        found_host = parse_uri(uri,host,port,short_uri);
        char new_req[MAXLINE];

        /*using cutted short_uri and default version*/
        sprintf(new_req, "%s %s %s",method, short_uri,default_version);
        append_header(new_req,header);
        return found_host;

}

/*
* Add header to the request, but ignore the default headers
*/
void set_header(char *key, char *value, char *header)
{
        if (!strcmp(key,"User-Agent")|| !strcmp(key,"Accept")|| !strcmp(key,"Accept-Encoding")
                        || !strcmp(key,"Connection") || !strcmp(key, "Proxy-Connection")) {
                dbg_printf("Ignore header %s\n",key);
        } else {
                add_to_header(key,value,header);
        }


}


/*Extract host and port from the value of Host header*/
void extract_host_port(char *value,char* host,int* port)
{
        char t_value[MAXLINE];
        strcpy(t_value,value);
        char *ptr;

        *port = 80;

        ptr = strstr(t_value,":");
        if (ptr!=NULL) {
                *ptr = 0;
                strcpy(host,t_value);
                *port = atoi(ptr+1);
        }
}

void *thread(void* vargp)
{
        int connfd = *((int *)vargp);
        Pthread_detach(Pthread_self());
        Free(vargp);
        doit(connfd);
        Close(connfd);
        return NULL;
}

int iRio_writen(int fd, void *usrbuf, size_t n)
{
        if (rio_writen(fd, usrbuf, n) != n) {
                if (errno == EPIPE || errno == ECONNRESET) {
                        printf("Server closed %s\n",strerror(errno));
                        return -1;
                } else unix_error("Rio_writen error");
        }

        return 0;
}

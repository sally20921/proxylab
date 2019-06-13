#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "csapp.h"

#include "cache.h"
/* Recommended max cache and object sizes */
//#define MAX_CACHE_SIZE 1049000
//#define MAX_OBJECT_SIZE 102400

//MAXLINE defined in csapp.h to be 8192
#define METHOD_LEN 25
#define VERSION_LEN 15
#define PORT_LEN 25
//#define DEFAULT_PORT 80

/* You won't lose style points for including this long line in your code */
//important request headers are Host, User-Agent, Connection, and Proxy-Connection headers
static char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; " \
"Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static char *connection_hdr = "Connection: close\r\n";
static char *proxy_connection_hdr = "Proxy-Connection: close\r\n";

//Forward all requests as HTTP/1.0 even if the original request was HTTP/1.1
static char *server_version = "HTTP/1.0";

//HTTP request port is an optional field in the URL of HTTP request. If not included, us default port 80.
static char *default_port = "80";

//if browser sends any additional request headers, proxy forwards them unchanged
//static char *type_header = "Content-type";
//static char *type_text = "text";




struct Request
{
    char request[MAXLINE];
    char method[METHOD_LEN]; //GET
    char host_addr[MAXLINE]; //www.cmu.edu
    char port[PORT_LEN]; //default port: 80
    char path[MAXLINE]; // /hub/indexl.html
    char version[VERSION_LEN]; //HTTP/1.1 => HTTP/1.0
};


int parse_request(rio_t *rio,char *request, struct Request *req);
int from_client_to_server(rio_t *rio, struct Request *req, int serverfd, int clientfd);
int from_server_to_client(int serverfd, int clientfd, char *response);
void *handle_client(void *vargp);



int main(int argc, char **argv)
{
    Signal(SIGPIPE, SIG_IGN);
    //p.964, ignore SIGPIPE signals and deal gracefully with write operations that return EPIPE errors.
    
    if (argc != 2)
    {
        //fprintf(stderr, "usage: %s <port> \n", argv[0]);
        exit(1);
        //not able to use error handling functions such as unix_error, posix_error, dns_error, gai_error, app_error
    }
    
    cache_init();
    
    {
        int listenfd = Open_listenfd(argv[1]);
        
        struct sockaddr_storage clientaddr;
        
        while (1)
        {
            socklen_t clientlen = sizeof(clientaddr);
            
            int clientfd =
            Accept(listenfd, (SA *)&clientaddr, &clientlen);
            
            //Dealing with multiple concurrent requests
            pthread_t tid;
            Pthread_create(&tid, NULL,
                           handle_client, (void *)(long)clientfd);
        }
    }
}


void *handle_client(void *vargp)
{
    Pthread_detach(Pthread_self());
    
    int clientfd = (int) (long) vargp;
    rio_t rio_to_client;
    Rio_readinitb(&rio_to_client, clientfd);
    
    struct Request req;
    memset((char *)&req, 0,sizeof (struct Request));
    
    
    char request[MAXLINE];
    parse_request(&rio_to_client, request, &req);
    //parse request from the client
    
    
    
    char cached_obj[MAX_OBJECT_SIZE];
    //read cache and copy it into cached_obj
    //if n is 0 then it is not in the cache
    ssize_t n = read_cache(req.request, cached_obj);
    if (n) {
        
        Rio_writen(clientfd, cached_obj, n);
        return NULL;
        
    }
    
    //if it is not in the cache, then contact server
    int serverfd =Open_clientfd(req.host_addr, req.port);
    /**from client to server **/
    from_client_to_server(&rio_to_client,
                          &req, serverfd, clientfd);
    
    static char response[MAX_OBJECT_SIZE];
    /**from server to client**/
    c
    
    //then cache it in cache list
    if (response_size < MAX_OBJECT_SIZE) {
        //if the size of the buffer ever exceeds the maximum object size, the buffer can be discarded
        //write to cache
        write_cache(req.request, response, response_size);
    }
    
    
    Close(serverfd);
    Close(clientfd);
    
    return NULL;
}


int parse_request(rio_t *rio,
                  char *request, struct Request *req)
{
    //GET http://csapp.cs.cmu.edu:80/index.html HTTP/1.1
    
    static char uri[MAXLINE]; //http://csapp.cs.cmu.edu:80/index.html
    static char version[VERSION_LEN]; //HTTP/1.1
    static char method[METHOD_LEN]; //GET
    
    Rio_readlineb(rio, request, MAXLINE);
    
    sscanf(request, "%s %s %s", method, uri, version);
    
    //uri : http://csapp.cs.cmu.edu:80/index.html
    //hostname: csapp.cs.cmu.edu
    //hostname_port: 80
    //path: /index.html
    
    strncpy(req->request, request, strlen(request));
    strncpy(req->method, method,strlen(method));
    strncpy(req->version, version, strlen(version));
    
    char hostname_port[MAXLINE];//this is temp char for hostname and port
    char path[MAXLINE];
    char port[PORT_LEN]; //hostname_port is later divided into port and hostname
    char hostname[MAXLINE];
    
    
    char *temp = uri;
    //strstr(char *object_string, char *find_string)
    //returns pointer to the start of find_string, else NULL
    temp = strstr(temp, "://");
    temp = temp+strlen("://");
    path[0] = '/';//path always starts from '/' including home
    //"%[^/]" indicates the string can contain any charater other than '/'
    sscanf(temp, "%[^/]%s",
           hostname_port, path);
    //"%[^:]" indicates the string can contain any charater other than ':'
    sscanf(hostname_port, "%[^:]:%s",
           hostname, port);
    
    strncpy(req->host_addr, hostname, strlen(hostname));
    strncpy(req->path, path, strlen(path));
    
    if (strlen(port) == 0) //if it does not contain port then use default port
    {
        strncpy(req->port, default_port, strlen(default_port));
    }
    else
    {
        strncpy(req->port, port, strlen(port));
    }
    
    return 0;
}


int from_client_to_server(rio_t *rio,
                          struct Request *req, int serverfd, int clientfd)
{
    static char requestheader_pool[MAXLINE];
    static char *requestheaders[MAXLINE];
    
    int m = 0;
    
    //this proxy only implements GET method
    if (!strncasecmp(req->method, "GET", strlen("GET"))) {
        
        int n;
        char *current_ptr = requestheader_pool;
        
        while (1)
        {
            //reading from the client server and saving it into
            //requestheader_pool
            n = rio_readlineb(rio, current_ptr, MAXLINE);
            
            requestheaders[m++] = current_ptr;
            current_ptr[n] = '\0';
            
            char *header_str = current_ptr;
            
            current_ptr += n + 1; //because it read n+1 from requestheader_pool
            
            //check if it has reached the end of the line
            if (!strncmp(header_str, "\r\n", strlen("\r\n"))) {
                break;
            }
        }
        
        static char req_final[MAXLINE];
        
        n = snprintf(req_final, MAXLINE, "%s %s %s\r\n",
                     req->method,
                     *req->path ? req->path : "/",
                     server_version);
        
        //write the final request to server
        Rio_writen(serverfd, req_final, n);
        
        static char host_hdr[MAXLINE];
        snprintf(host_hdr, MAXLINE,
                 "Host: %s\r\n",
                 req->host_addr);
        
        //send the rest
        int has_agent_hdr = 0; //User-Agent:
        int has_conn_hdr = 0; //Connection:
        int has_host_hdr = 0; //Host:
        
        
        for(int i = 0; i < m; ++i)
        {
            char *header_str = requestheaders[i];
            
            if (!memcmp(header_str,
                        "User-Agent:",
                        strlen("User-Agent:"))) {
                has_agent_hdr = 1;
                Rio_writen(serverfd,user_agent_hdr,strlen(user_agent_hdr));
                
            }
            
            else if (!memcmp(header_str,
                             "Connection:",
                             strlen("Connection:"))) {
                has_conn_hdr = 1;
                Rio_writen(serverfd,connection_hdr,strlen(connection_hdr));
            }
            
            else if (!memcmp(header_str,
                             "Proxy-Connection:",
                             strlen("Proxy-Connection:"))) {
                has_conn_hdr = 1;
                Rio_writen(serverfd,proxy_connection_hdr, strlen(proxy_connection_hdr));
                
            }
            
            else if (!memcmp(header_str,
                             "Host:",
                             strlen("Host:"))) {
                
                has_host_hdr = 1;
                Rio_writen(serverfd,host_hdr,strlen(host_hdr));
            }
            
            else { //other header, just send it to the server
                Rio_writen(serverfd,header_str,strlen(header_str));
            }
        }
        
        if (!has_agent_hdr) {
            
            Rio_writen(serverfd, user_agent_hdr,strlen(user_agent_hdr));
        }
        
        if (!has_conn_hdr){
            
            Rio_writen(serverfd,connection_hdr,strlen(connection_hdr));
            
        }
        
        if (!has_host_hdr) {
            
            Rio_writen(serverfd,host_hdr,strlen(host_hdr));
        }
        
        return 1;
    }
    
    return 0;
}


int from_server_to_client(int serverfd, int clientfd, char *response)
{
    
    char buf[MAXLINE]={};
    response[0] = '\0';
    int response_size = 0;
    
    ssize_t n;
    while((n=rio_readn(serverfd, buf, MAXLINE)) != 0)
    {
        //this is the process of setting cache buf
        //save it in response until it exceeds max object size
        buf[n] = '\0';
        if (response_size < MAX_OBJECT_SIZE)
        {
            response_size += n;
            if (response_size < MAX_OBJECT_SIZE) {
                strcat(response, buf);
            }
            else {
                response_size = MAX_OBJECT_SIZE;
            }
        }
        
        Rio_writen(clientfd, buf, n);
        
    }
    
    return response_size;
    
    
}


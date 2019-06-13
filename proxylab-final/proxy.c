/**Proxy that handles HTTP GET requests.**/

//We cannot use Rio_ write wrapper functions because they implement unix_error
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
//This is different from the listening port 

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
    //default behavior for SIGPIPE is to terminate the process
    //p.964, ignore SIGPIPE signals and deal gracefully with write operations that return EPIPE errors.
    
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port> \n", argv[0]);
	//stderr: output monitor
        exit(1);
	//not able to use error handling functions such as unix_error, posix_error, dns_error, gai_error, app_error
    }
    
    cache_init();
    
    {	//Open_listenfd(int port): open and return a listening socket on port
        int listenfd = Open_listenfd(argv[1]);
	//listen for incoming connections on a port whose number will be 		//specified on the command line 
	//./proxy 15213
        
        struct sockaddr_storage clientaddr;
        
        while (1)
        {
            socklen_t clientlen = sizeof(clientaddr);
            
	    //connection established
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
    //thread should run in detached mode to avoid memory leaks
    Pthread_detach(Pthread_self());
    
    int clientfd = (int) (long) vargp;
    //read the entirety of the request from the client 
    rio_t rio_to_client;
    Rio_readinitb(&rio_to_client, clientfd);
    
    struct Request req;
    memset((char *)&req, 0,sizeof (struct Request));
    
   //parse request from the client
    char request[MAXLINE];
    if (parse_request(&rio_to_client, request, &req) <0 ){
    //the client has not sent a valid request
	Close(clientfd);
	return NULL;
    }
    
    
    char cached_obj[MAX_OBJECT_SIZE];
    //read cache and copy it into cached_obj
    //if n is 0 then it is not in the cache
    ssize_t n = read_cache(req.request, cached_obj);
    if (n) {

    	if (rio_writen(clientfd, cached_obj, n) < 0) {
		fprintf(stderr, "%s\n", "writing cache object to clientfd error");
	}
	Close(clientfd);
	return NULL;
	
    }
    
    //if it is not in the cache, establish connection to server
    //open_clientfd(char *hostname, int port): open connection to server at 
    //<hostname, port> and return socket descriptor ready for reading/writing
    int serverfd =Open_clientfd(req.host_addr, req.port);
    //request the object the client specified
    /**from client to server**/
    from_client_to_server(&rio_to_client,
                          &req, serverfd, clientfd);
    
    //read the server's response and forward it to the client 
    //allocate response(buffer) for each active connection and accumulate data 
    //as it is received from the server 
    static char response[MAX_OBJECT_SIZE];
    /**from server to client**/
    int response_size = from_server_to_client(serverfd, clientfd, response);
    
    if (response_size < 0) { //error handling
	return NULL;
    }
    //then cache it in cache list
    //when your proxy receives a web oject from a server, it should cache it as 
    //it transmits the object to client 
    if (response_size < MAX_OBJECT_SIZE) {
	//if the size of the buffer ever exceeds the maximum object size, the buffer can be discarded
	//write to cache 
    	write_cache(req.request, response, response_size);
    }
    
    
    Close(serverfd);
    Close(clientfd);
    
    return NULL;
}


//determine whethre the client has sent a valid HTTP request
int parse_request(rio_t *rio,
                  char *request, struct Request *req)
{
//GET http://csapp.cs.cmu.edu:80/index.html HTTP/1.1

    static char uri[MAXLINE]; //http://csapp.cs.cmu.edu:80/index.html
    static char version[VERSION_LEN]; //HTTP/1.1
    static char method[METHOD_LEN]; //GET
    
    if (Rio_readlineb(rio, request, MAXLINE) <0 ){
	fprintf(stderr, "%s\n", "parse request readline error");
    }
    
    if (sscanf(request, "%s %s %s", method, uri, version) !=3) {
	return -1;
    }

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
    
   //this proxy only implements GET method
    if (!strncasecmp(req->method, "GET", strlen("GET"))) {
	
	static char buf[MAXLINE];
    	static char *requestheaders[MAXLINE];
	//think of requestheaders char pointer array to contain pointers to 
	//header lines which is buf_ptr
	//pointer array of buf_ptrs

        int n;
	int m = 0;
        char *buf_ptr = buf; //start at the star t of buf char array
        
        while (1)
        {
	    //reading one line from the client server and save it in buf_ptr
            n = rio_readlineb(rio, buf_ptr, MAXLINE);
            
	    //this is where it gets complicated
	    //think of what requestheaders mean, array of buf_ptrs  
            requestheaders[m++] = buf_ptr;
            buf_ptr[n] = '\0'; //there should always be '\0' to indicate 
				   //the end of char array 
            
            char *proper_buf_str = buf_ptr; //buf_ptr with '\0' added
            
            buf_ptr += n + 1; //because it read n+1 from temp
            
	    //check if it has reached the end of the line
            if (!strncmp(proper_buf_str, "\r\n", strlen("\r\n"))) {
                break;
            }
        }
        
        static char formatted_request[MAXLINE];
        
        n = snprintf(formatted_request, MAXLINE, "%s %s %s\r\n",
                     req->method,
                     *req->path ? req->path : "/",
                     server_version); 
	//modern web browsers will generate HTTP/1.1 requests, but proxy 
	//handles them and forward them as HTTP/1.0 requests 
        
	//write the final request to server
        if (rio_writen(serverfd, formatted_request, n) <0 ){
		fprintf(stderr, "%s\n", "from client to server write final request to server error");
		return -1;
	}
	
	/**send other headers after the request **/
	int has_host_hdr = 0; //Host:
	static char host_hdr[MAXLINE];        
        snprintf(host_hdr, MAXLINE,
                 "Host: %s\r\n",
                 req->host_addr);
	
      
	//I can choose whether to always send the user-agent header 
        int has_agent_hdr = 0; //User-Agent:

	//always send connection, and proxy-connection header 
        int has_conn_hdr = 0; //Connection: 
	int has_proxy_conn_hdr = 0;
 
       
        for(int i = 0; i < m; ++i) // this for traverses requestheaders array
	//and traverse through all the buf_pointers 
        {
            char *header_line = requestheaders[i];
            
            if (!strncmp(header_line,
                        "User-Agent:",
                        strlen("User-Agent:"))) {
		
                if(rio_writen(serverfd,user_agent_hdr,strlen(user_agent_hdr))<0)
		{
			return -1;
		}
			
		has_agent_hdr = 1;
                
            }
            
            else if (!strncmp(header_line,
                        "Connection:",
                        strlen("Connection:"))) {
		
                if(rio_writen(serverfd,connection_hdr,strlen(connection_hdr))<0)
		{
			return -1;
		}
		has_conn_hdr = 1;
            }
            
            else if (!strncmp(header_line,
                        "Proxy-Connection:",
                        strlen("Proxy-Connection:"))) {
		 has_conn_hdr = 1;
                if (rio_writen(serverfd,proxy_connection_hdr, strlen(proxy_connection_hdr))<0){
			return -1;
		}
		 has_proxy_conn_hdr = 1;

            }
            
            else if (!strncmp(header_line,
                        "Host:",
                        strlen("Host:"))) {
               
                
                if(rio_writen(serverfd,host_hdr,strlen(host_hdr))<0){
			return -1;
		}
		has_host_hdr = 1;
            }
            
            else { 
	     //if a browser sends any additional request headers as part of an 
	     //HTTP request, your proxy should forward them unchanged
             if(rio_writen(serverfd,header_line,strlen(header_line))<0){
		return -1;
	     }
            }
        }

        
        if (!has_agent_hdr) {//choose to always send agent header
           
            if(rio_writen(serverfd, user_agent_hdr,strlen(user_agent_hdr))<0){
		return -1;
	    }
        }
        
        if (!has_conn_hdr){ //always send connection header 
            
            if(rio_writen(serverfd,connection_hdr,strlen(connection_hdr))<0){
		return -1;
	    }
           
        }
	if (!has_proxy_conn_hdr){ //always send connection header 
            
            if(rio_writen(serverfd,proxy_connection_hdr,strlen(connection_hdr))<0){
		return -1;
	    }
           
        }
        
        if (!has_host_hdr) { //always send host header
            
            if (rio_writen(serverfd,host_hdr,strlen(host_hdr))<0){
		return -1;
	    }
        }
        
        return 1;
    }
    
   return 0;
}


int from_server_to_client(int serverfd, int clientfd, char *response)
{
    //if the entirety of the web server's response is read before the maximum 
    //object size is exceeded, then the object can be cached 
    char buf[MAXLINE]="\0";
    //initalize buf char array to null character
    response[0] = '\0'; //starts with null character
    int response_size = 0;

    ssize_t n;
    while((n=rio_readn(serverfd, buf, MAXLINE)) != 0)
    {
	//this is the process of setting cache buf
	//save it in response until it exceeds max object size
        buf[n] = '\0';
	//to use string.h functions
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
        
	if (rio_writen(clientfd, buf, n)<0){
	   return -1;
	}
        
    }
    
    return response_size;
    
   
}






#include <stdio.h>
#include "cache.h"
#include "csapp.h"


#define CONCURRENT

#ifndef DEBUG
#define debug_printf(...) \
  {}
#else
#define debug_printf(...) printf(__VA_ARGS__)
#endif

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* ---------global var ----------*/
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";
static const char *accept_hdr =
    "Accept: "
    "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/"
    "apng,*/*;q=0.8,application/signed-exchange;v=b3\r\n";
static const char *accept_encode_hdr = "Accept-Encoding: gzip, deflate\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_connection_hdr = "Proxy-Connection: close\r\n";
static const char *bad_request_response =
    "HTTP/1.0 400 Bad Request\r\n\r\n<html><body>Bad "
    "Request</body></html>\r\n\r\n";
static char *dns_error_response =
    "HTTP/1.0 500 Proxy Error\r\n\r\n<html><body>DNS "
    "Error</body></html>\r\n\r\n";
static char *sock_error_response =
    "HTTP/1.0 500 Proxy Error\r\n\r\n<html><body>Socket "
    "Error</body></html>\r\n\r\n";

typedef struct {
  int port;
  char host[MAXLINE];
  char content[MAXLINE];
} HttpRequest;

/* -----------declare func------------- */
void sigpipe_handler(int sig);
void proxy(int connfd);
void *proxy_thread(void *vargp);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void read_requesthdrs(rio_t *rp);
int parse_uri(const char *uri, int *port, char *hostname, char *pathname);
int parse_http_request(rio_t *rio, HttpRequest *request);
int parse_http_host(const char *host_header, char *hostname, int *port);
void forward_http_request(int clientfd, HttpRequest *request);

/* -------------routine------------*/
int main(int argc, char *argv[]) {
  int listenfd, connfd;
  socklen_t clientlen;
  char hostname[MAXLINE], port[MAXLINE];
  struct sockaddr_in clientaddr;
  pthread_t tid;
  int *connfdp;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <port>\n", argv[0]);
    exit(1);
  }

  cache_init();

  /* set SIGPIPE handler func, no exiting */
  signal(SIGPIPE, sigpipe_handler);

  /* client --------> proxy(listenfd, connfd) */

  /* create listen_fd */
  listenfd = open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    /* accept */
    connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd < 0) continue;
    /* get client host addr .etc info */
    getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE,
                0);
#ifndef CONCURRENT
    printf("Accepted new connection from (%s, %s)\n", hostname, port);
    proxy(connfd);
    Close(connfd);
#else
    debug_printf("New Thread\n");
    printf("Accepted new connection from (%s, %s)\n", hostname, port);
    connfdp = malloc(sizeof(int));
    *connfdp = connfd;
    if (0 != pthread_create(&tid, NULL, proxy_thread, connfdp)) {
      debug_printf("create new thread [%d] failed", tid);
      free(connfdp);
    }

#endif
  }
  cache_destroy();
  return 0;
}

void sigpipe_handler(int sig) {
  printf("SIGPIPE handled!\n");
  return;
}

void *proxy_thread(void *vargp) {
  int connfd = *((int *)(vargp));
  /* detach the thread. when the thread terminates,
   * its resource will be recycled.
   */
  pthread_detach(pthread_self());
  free(vargp);

  proxy(connfd);
  close(connfd);
  return NULL;
}

void proxy(int connfd) {
  rio_t fromcli_rio, toserv_rio;
  HttpRequest request;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char host[MAXLINE], path[MAXLINE];
  int port, toserverfd, irespond;
  char toserverport[MAXLINE];
  char toserverreq[MAXLINE];

  /* client ---(request)---->  (connfd)proxy */
  /*       <----(response)---   (connfd) */
  /* Read http request line and headers */
  rio_readinitb(&fromcli_rio, connfd);
  if (parse_http_request(&fromcli_rio, &request) == -1) {
    rio_writen(connfd, bad_request_response, strlen(bad_request_response));
    return;
  }

  debug_printf("Host: %s, Port: %d\n", request.host, request.port);

  /* proxy ----(request)----> server */
  /* forward the request to the server */
  /* proxy <---(response)---- server */
  /* forward the response */
  forward_http_request(connfd, &request);
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg) {
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body,
          "%s<body bgcolor="
          "ffffff"
          ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  rio_writen(fd, buf, strlen(buf));
  rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];
  rio_readlineb(rp, buf, MAXLINE);
  while (strcasecmp(buf, "\r\n")) {
    rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

int parse_uri(const char *uri, int *port, char *hostname, char *pathname) {
  /* uri : http://www.google.com:80/index.html or /index.html */
  /* Remove protocal part */
  const char *url = strncmp(uri, "http://", 7) == 0 ? uri + 7 : uri;
  /* is there port ":" ? */
  if (strchr(url, ':')) {
    /* regexr [^:] find not include ':' part */
    sscanf(url, "%[^:]:%i%s", hostname, port, pathname);
  } else {
    *port = 80;
    sscanf(url, "%[^/]%s", hostname, pathname);
  }

  /* if Path is NULL */
  if (pathname == NULL) strcpy(pathname, "/");

  return 0;
}

/*
 * client---(request)--->proxy
 * parse "request (GET / HTTP/1.1 ...)" into specific "request".
 */
int parse_http_request(rio_t *rio, HttpRequest *request) {
  char line[MAXLINE], version[64], method[64], uri[MAXLINE], path[MAXLINE];
  size_t rc;
  rc = 0;
  request->port = 80;
  rio_readlineb(rio, line, MAXLINE);
  /* request header : GET http://www.google.com:80/index.html or /index.html
   * HTTP/1.1 */
  sscanf(line, "%s %s %s", method, uri, version);
  if (strcasecmp(method, "GET") != 0) {
    printf("Error: %s is not supported!\n", method);
    return -1;
  }
  debug_printf("Request from client: >---------%s", line);
  /* parse uri : get host and port */
  parse_uri(uri, &(request->port), (char *)&(request->host), path);

  /* HTTP/1.1 -> HTTP/1.0 */
  sprintf(request->content, "GET %s HTTP/1.0\r\n", path);

  /* read following headers line by line*/
  while ((rc = rio_readlineb(rio, line, MAXLINE)) > 0) {
    /* http request header the last line is \r\n */
    if (strcmp(line, "\r\n") == 0) {
      strcat(request->content, line);
      break;
    } else if (strstr(line, "Host:")) {
      strcat(request->content, line);
      // Host: 192.168.1.1:8000
      parse_http_host(line, (char *)&(request->host), &(request->port));
    } else if (strstr(line, "User-Agent:")) {
      strcat(request->content, user_agent_hdr);
    } else if (strstr(line, "Accept:")) {
      strcat(request->content, accept_hdr);
    } else if (strstr(line, "Accept-Encoding:")) {
      strcat(request->content, accept_encode_hdr);
    } else if (strstr(line, "Connection:")) {
      strcat(request->content, connection_hdr);
    } else if (strstr(line, "Porxy-Connection:")) {
      strcat(request->content, proxy_connection_hdr);
    } else {
      /* others */
      strcat(request->content, line);
    }
  }

  if (rc < 0) {
    printf("Error when reading request!\n");
    return -1;
  }
  return 0;
}

/* Host: 192.168.1.1:8000\r\n  get host and port */
int parse_http_host(const char *host_header, char *hostname, int *port) {
  char port_str[8];
  char *host_begin = (char *)(host_header + 6);  // skip Host:
  char *host_end = strstr(host_header, "\r\n");
  char *port_begin = strchr(host_begin, ':');
  size_t port_len;
  if (port_begin == NULL)
    strncpy(hostname, host_begin, (size_t)(host_end - host_begin));
  else {
    port_len = (size_t)(host_end - port_begin) - 1;
    strncpy(hostname, host_begin, (size_t)(port_begin - host_begin));
    strncpy(port_str, port_begin + 1, port_len);
    port_str[port_len] = '\0';
    *port = atoi(port_str);
  }
  return 0;
}

/*
 * client <------(response)------ [connfd] proxy [serverfd]
 * ----(request)---->server
 */
void forward_http_request(int connfd, HttpRequest *request) {
  int serverfd, object_size, n;
  char buf[MAXLINE], response_from_server[MAX_OBJECT_SIZE], port_str[8];
  rio_t toserver_rio;
  debug_printf("Request to server: \n---------\n%s", request->content);

  /*
   * If the cache has the request's response,
   * just write it back to connfd.
   * If not , to proxy ---> server, server--->proxy,proxy--->client.
   * And put the new request-response into the cache.
   */

  if (cache_get(request->content, response_from_server)) {
    debug_printf("Hit response in the cache!\n");
    rio_writen(connfd, response_from_server, strlen(response_from_server));
    return;
  }

  sprintf(port_str, "%d", request->port);
  serverfd = open_clientfd(request->host, port_str);
  if (serverfd == -1) {
    /* create socket error */
    /* response the info to client */
    rio_writen(connfd, sock_error_response, strlen(sock_error_response));
    return;
  } else if (serverfd == -2) {
    /* DNS error */
    rio_writen(connfd, dns_error_response, strlen(dns_error_response));
    return;
  }

  /* proxy[serverfd] -----(request(from client)) ----> server */
  rio_readinitb(&toserver_rio, serverfd);
  rio_writen(serverfd, request->content, strlen(request->content));

  object_size = 0;
  while ((n = rio_readlineb(&toserver_rio, buf, MAXLINE)) > 0) {
    object_size += n;
    if (object_size <= MAX_OBJECT_SIZE) strcat(response_from_server, buf);
    rio_writen(connfd, buf, n);
  }

  /* proxy[serverfd] <--------response------- server */
  debug_printf("Response from server :\n-----------\n%s", response_from_server);

  /* Put the new request-respond into the cache */
  if (object_size <= MAX_OBJECT_SIZE)
    cache_place(request->content, response_from_server);
  close(serverfd);
}

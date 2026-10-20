#define _GNU_SOURCE
#include "engine.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 3000
#define BUF 8192
static void send_text(int c, const char *status, const char *type, const char *body) { dprintf(c,"HTTP/1.1 %s\r\nContent-Type: %s\r\nCache-Control: no-cache\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n%s",status,type,strlen(body),body); }
static double query_num(const char *q, const char *key, double fallback) { char pat[64]; snprintf(pat,sizeof pat,"%s=",key); char *p=strstr(q,pat); return p ? atof(p+strlen(pat)) : fallback; }
static void handle(int c) {
    char req[BUF]={0}; ssize_t n=read(c,req,sizeof(req)-1); if(n<=0) return;
    char method[8]={0}, path[2048]={0}; sscanf(req,"%7s %2047s",method,path);
    if (strncmp(path,"/api/advice",11)==0) {
        const char *q=strchr(path,'?'); q=q?q+1:""; FinanceInput in={query_num(q,"income",120000),query_num(q,"expenses",55000),query_num(q,"emi",20000),query_num(q,"emergency",240000),query_num(q,"age",29), (int)query_num(q,"risk",2), (int)query_num(q,"goal",0)}; Advice a; char json[2048]; calculate_advice(&in,&a); advice_json(&in,&a,json,sizeof json); send_text(c,"200 OK","application/json",json); return;
    }
    if (strcmp(path,"/manus-routes.json")==0) { send_text(c,"200 OK","application/json","{\"routes\":[{\"path\":\"/\",\"title\":\"FinAdvisor C\"}]}"); return; }
    char rel[2100]; const char *clean = strcmp(path,"/")==0 ? "/index.html" : path; if(strstr(clean,"..")) { send_text(c,"400 Bad Request","text/plain","Bad path"); return; }
    snprintf(rel,sizeof rel,"public%s",clean); FILE *f=fopen(rel,"rb"); if(!f){send_text(c,"404 Not Found","text/plain","Not found");return;} fseek(f,0,SEEK_END); long size=ftell(f); rewind(f); char *body=malloc(size+1); if(!body){fclose(f);send_text(c,"500 Internal Server Error","text/plain","Out of memory");return;} size_t read_bytes=fread(body,1,size,f); body[read_bytes]=0; fclose(f); const char *type=strstr(rel,".css")?"text/css":(strstr(rel,".js")?"application/javascript":"text/html; charset=utf-8"); send_text(c,"200 OK",type,body); free(body);
}
int main(void) { signal(SIGPIPE,SIG_IGN); int s=socket(AF_INET,SOCK_STREAM,0); int one=1; setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&one,sizeof one); struct sockaddr_in a={.sin_family=AF_INET,.sin_port=htons(PORT),.sin_addr.s_addr=INADDR_ANY}; if(bind(s,(struct sockaddr*)&a,sizeof a)<0||listen(s,32)<0){perror("bind/listen");return 1;} printf("FinAdvisor C server listening on http://0.0.0.0:%d\n",PORT); fflush(stdout); while(1){int c=accept(s,0,0); if(c>=0){handle(c);close(c);}} }

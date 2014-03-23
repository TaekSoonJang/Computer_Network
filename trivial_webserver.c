
/*
  "Just" trival TCP Web Server
  Developed Based on Dr. JK.Kim's 'very trivial web server' code
  Written By TaekSoon Jang (NHN NEXT, 131062)
*/

#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define WEB_SERVER_ROOT   "webapps"
#define READ_BUF_SIZE   1024

void urlTransform(char *inputUrl, char *finalUrl);

int main(int argc, char**argv)
{
  int i;
  int reqUrlLen;
  int listenfd, connfd, n;
  int requestFd, readFileSize;
  struct sockaddr_in servaddr,cliaddr;
  socklen_t clilen;
  char mesg[4096];
  char workingDir[1024];
  char targetDir[1024];
  char reqUrl[256];
  char outputHeader[] = "HTTP/1.0 200 OK\r\nDate: Wed, 12 Mar 2014 00:14:10 GMT\r\n\r\n";
  char fileBuf[1024];
  char outputBody[4096];

  

  listenfd=socket(AF_INET,SOCK_STREAM,0);

  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  servaddr.sin_port=htons(32000);
  bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr));

  listen(listenfd,1024);

  for(i = 0; i < 100;i++) {
    memset(reqUrl, 0, sizeof(reqUrl));
    memset(targetDir, 0, sizeof(targetDir));
    memset(workingDir, 0, sizeof(workingDir));

    if (getcwd(workingDir, sizeof(workingDir)) != 0)
    {
      printf("Current Working Directory : %s\n", workingDir);
    }

    clilen=sizeof(cliaddr);
    connfd = accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
    printf("connected (%d)\n", connfd);

    if (connfd > 0) {
      n = recv(connfd,mesg,4096,0);
      mesg[n] = 0;

      /*
        Homework part
        (Suppose there are only GET request from a client)
      */

      /* Parse request url from request */
      reqUrlLen = 0;
      while (strncmp(mesg + reqUrlLen + 4, " ", 1))
      {
        printf("wow\n");
        reqUrlLen++;
      }

      strncpy(reqUrl, mesg + 4, reqUrlLen);
      reqUrl[reqUrlLen] = 0;

      /* Transform request url to absolute path to my web server resource directory */
      urlTransform(reqUrl, targetDir);
      strcpy(workingDir + strlen(workingDir), "/");
      strcpy(workingDir + strlen(workingDir), targetDir);

      printf("Target dir : %s\n", workingDir);

      /* Read target file and write to response body part */
      if (0 < (requestFd = open(workingDir, O_RDONLY)))
      {
        while (0 < (readFileSize = read(requestFd, fileBuf, READ_BUF_SIZE - 1)))
        {
          printf("read\n");
          fileBuf[readFileSize] = 0;
          strncpy(outputBody, fileBuf, READ_BUF_SIZE);
        }

        close(requestFd);
      }
      else
      {
        strcpy(outputBody, "<html><h1>No Such File.</h1></html>\n");
      }

      outputBody[strlen(outputBody)] = 0;
      strcpy(outputHeader, outputBody);

      printf("=====\n%s=====\n", mesg);
      send(connfd,outputHeader,strlen(outputHeader),0);
      close(connfd);
    }
  }
}

void urlTransform(char *inputUrl, char *finalUrl)
{ 
  const char *prefix = WEB_SERVER_ROOT;
  const char *defaultHtml = "index.html\0";

  strcpy(finalUrl, prefix);
  strcpy(finalUrl + strlen(finalUrl), inputUrl);

  /* Case : inputUrl is default, '/' */
  if (!strcmp(inputUrl, "/"))
  {
    strcpy(finalUrl + strlen(finalUrl), defaultHtml);
  }
  /* Case : inputUrl has prefix, but no html file */
  else if (strncmp(finalUrl + strlen(finalUrl) - 4, "html", 4))
  {
    strcpy(finalUrl + strlen(finalUrl), "/");
    strcpy(finalUrl + strlen(finalUrl), defaultHtml);
  }
}
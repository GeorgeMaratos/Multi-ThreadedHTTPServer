#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <pthread.h>
#include <dirent.h>

#define DEBUG 0
#define BUFFERSIZE 50
#define FILENAMESIZE 20
#define BACKLOG 10

char *badR =  "<html><head><title>Homework 1 test pages</title></head>\
                        <body>\
                        ERROR 404.\
                        </body></html>\n";
char *dirHead = "<title>Directory listing for /</title>\
		<body>\
		<h2>Directory listing for /</h2>\
		<hr>\
		<ul>";

char *dirTail = "</ul>\
		<hr>\
		</body>\
		</html>";
char *refHead = "<li><a href=\"";

char *folder;


void
sendBadReq(int fd) {
  if (DEBUG) fprintf(stdout,"Sending Bad Request Socket <%d>\n",fd);
  if (DEBUG) fprintf(stdout,"Sending Request\n");
  if (send(fd,badR,strlen(badR),0) < 1)
    if (DEBUG) fprintf(stdout,"Cannot send request\n"); 
  if (DEBUG) fprintf(stdout,"Finished Request\n");
}

char*
formatLookup(char* fileName) {
  //variables
  int size1,size2,size3,i;
  char *newString;
  //ops
  if (DEBUG) fprintf(stdout,"Filename in formatLookup <%s>\n",fileName);
  size1 = strlen(folder);
  size2 = strlen(fileName);
  size3 = size1 + size2 + 2;
  newString = malloc(size3);
  sprintf(newString,"%s/%s",folder,fileName);
  //return
  if (DEBUG) fprintf(stdout,"format <%s>\n",newString);
  return newString;
}
int
folderCheck(char *fileName)
{
  //varibles
  struct stat buf;
  //ops
  if (DEBUG) fprintf(stdout,"folderCheck: %s\n",fileName);
  stat(fileName,&buf);
  if (S_ISDIR(buf.st_mode) == 1) {
    if (DEBUG) fprintf(stdout,"IS A DIRECTORY\n");
    return 1;
  }
  else return 0;
}
void
printDirContents(int fd, char *dirPath)
{
  //variables
  DIR *d;
  struct dirent *df;
  char *file,*frmpath,*sendFile,*fileChecker, *fHTML;
  //ops
  sendFile = malloc(900);
  fileChecker = malloc(20);
  fHTML = malloc(20);
  frmpath = formatLookup(dirPath);
  if (DEBUG) fprintf(stdout,"Searching DIR: %s\n",frmpath);
  d = opendir(frmpath);
  df = readdir(d);
  sprintf(sendFile,"%s",dirHead);
  while (df != NULL) {
    file = df->d_name;
    sprintf(fileChecker,"%s/%s\n",frmpath,file);
    sprintf(fHTML,"%s/%s",dirPath,file);
    if (df->d_type == DT_DIR)
      sprintf(sendFile,"%s%s%s\">%s/</a>",sendFile,refHead,fHTML,file);
    else sprintf(sendFile,"%s%s%s\">%s</a>",sendFile,refHead,fHTML,file);
    df = readdir(d);
  }
  sprintf(sendFile,"%s%s",sendFile,dirTail);
  if (DEBUG) fprintf(stdout,"TOBESENT:\n %s",sendFile);
  send(fd,sendFile,strlen(sendFile),0);
}
void
sendObject(int fd,char* fileName)
{
  //variables
  FILE *f,*f2;
  int i,size;
  char c,*frmStr,*content,*sendFile,*dirFileName;
  struct stat buf;
  //ops
  i = 0;
  frmStr = formatLookup(fileName);
  if (DEBUG) fprintf(stdout,"Looking in %s\n",frmStr); 
  f = fopen(frmStr,"r");
  stat(frmStr,&buf); 
  content = malloc((int)buf.st_size);
  dirFileName = malloc(20);
  if (f == NULL) sendBadReq(fd);
  else {
    if (folderCheck(frmStr) == 0) {
      for(i=0;i<(int)buf.st_size;i++) {
        fscanf(f,"%c",&c);
        content[i] = c;
      }
      if (send(fd,content,(int)buf.st_size,0) < 1)
        fprintf(stderr,"Send Failed\n");
    }
    else {
      sprintf(dirFileName,"%s/index.html",fileName);
      if (DEBUG) fprintf(stdout,"User Requested Dir sending <%s>\n",dirFileName);
      f2 = fopen(dirFileName,"r");
      if (f2 == NULL)
	printDirContents(fd,fileName);
      else sendObject(fd,dirFileName);
    }
  }
  return;
}
void*
handleClient(void *fd)
{
  char *buf,*objectName;
  //ops
  if (DEBUG) fprintf(stdout,"Handling Client\n");
  buf = malloc(sizeof(char) * BUFFERSIZE);
  objectName = malloc(sizeof(char) * FILENAMESIZE);
  recv((int)fd,buf,BUFFERSIZE,0);
  sscanf(buf,"GET /%s/",objectName);
  if (DEBUG) fprintf(stdout,"Echo:\n%s",buf);
  if (DEBUG) fprintf(stdout,"File Requested: %s\n",objectName);
  if (strlen(objectName) == 1) {
    fprintf(stdout,"MainPage Requested\n");
    sendObject((int)fd,"index.html");
  }
  else sendObject((int)fd,objectName);
  //return
  return NULL;
}

int 
acceptConn(int fd)
{
  //variables
  int newfd;
  socklen_t sin_size;
  struct sockaddr_storage their_addr;
  pthread_t thredId;
  char s[INET6_ADDRSTRLEN];
  //ops
  sin_size = sizeof(their_addr);
  if (DEBUG) fprintf(stdout,"File Descriptor <%d>\n",fd);
  fprintf(stdout,"Server awaiting connection...\n");
  while (1) {
    newfd = accept(fd,(struct sockaddr *)&their_addr,&sin_size);
    if (DEBUG) fprintf(stdout,"Sending File Descriptor to thread <%d>\n",newfd);
    if (DEBUG) fprintf(stdout,"Connection Accepted\n");
    pthread_create(&thredId,NULL,&handleClient,(void*)newfd);
  }
}
void
setHints(struct addrinfo *h) 
{
  //ops
  if (DEBUG) fprintf(stdout,"Setting Hints\n");
  memset(h, 0, sizeof(struct addrinfo));
  h->ai_family = AF_UNSPEC;
  h->ai_socktype = SOCK_STREAM;
  h->ai_flags = AI_PASSIVE;
}
int
formatServer(char *port) 
{
  //variables
  int sockfd,rv;
  struct addrinfo hints, *servinfo, *p;
  struct sigaction sa;
  int optVal;
  //ops
  if(DEBUG) fprintf(stdout,"Formating Server\n");
  optVal = 1;
  setHints(&hints);
  if ((rv = getaddrinfo(NULL,port,&hints,&servinfo)) != 0) {
    fprintf(stderr,"getaddrinfo: %s\n",gai_strerror(rv));
    return -1;
  }
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family,p->ai_socktype,
             p->ai_protocol)) < 0) {
      fprintf(stderr,"socket\n");
      continue;
    }
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optVal,
            sizeof(int)) < 0) {
      fprintf(stderr,"setsockopt\n");
      exit(1);
    }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
      close(sockfd);
      fprintf(stderr,"bind\n");
      continue;
    }
    break;
  }
  if (p == NULL) {
    fprintf(stderr,"server: failed to bind\n");
    return -2;
  }
  freeaddrinfo(servinfo);
  if (listen(sockfd,BACKLOG) < 0) {
    fprintf(stderr,"server: listen failed\n");
    return -3;
  }
  //return
  acceptConn(sockfd);
}

int
main(int argc, char **argv) 
{
  //variables
  //ops
  if(argc != 3) {
    fprintf(stderr,"Not enough arguments...usage\
			  ./hw2 <Port_Number> <Root_Folder>\n");
    return -1;
  }
  folder = argv[2];
  if (DEBUG) fprintf(stdout,"Arguments <%s> <%s>\n",argv[1],argv[2]);
  if (formatServer(argv[1]) < 0) return -1;
  //return
  return 1;
}

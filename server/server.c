#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>     // for dirname()/basename()
#include <time.h> 

#define MAX   256
#define BLK  1024

int server_sock, client_sock;      // file descriptors for sockets
char *serverIP = "127.0.0.1";      // hardcoded server IP address
int serverPORT = 1234;             // hardcoded server port number
char command[16], pathname[64];
//                0         1     2     3     4      5     6    7     
char *cmd[] = {"mkdir", "rmdir","ls", "cd", "pwd","rm", "get","put", 0};
struct sockaddr_in saddr, caddr;   // socket addr structs

int init()
{
    printf("1. create a socket\n");
    server_sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (server_sock < 0) { 
        printf("socket creation failed\n"); 
        exit(0); 
    }
    
    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr)); 
    saddr.sin_family = AF_INET; 
    saddr.sin_addr.s_addr = inet_addr(serverIP);
    saddr.sin_port = htons(serverPORT);
    
    printf("3. bind socket to server\n");
    if ((bind(server_sock, (struct sockaddr *)&saddr, sizeof(saddr))) != 0) { 
        printf("socket bind failed\n"); 
        exit(0); 
    }
    printf("4. server listen with queue size = 5\n");
    if ((listen(server_sock, 5)) != 0) { 
        printf("Listen failed\n"); 
        exit(0); 
    }
    printf("5. server at IP=%s port=%d\n", serverIP, serverPORT);
}
  
int findCmd(char *command) //finding the cmd for main menu
{
   int i = 0;
   while(cmd[i]){
     if (strcmp(command, cmd[i])==0)
         return i;
     i++;
   }
   return -1;
}

int main() 
{
    int n, length;
    char line[MAX];
    int  index;
    
    init();  

    while(1){
       printf("server: try to accept a new connection\n");
       length = sizeof(caddr);
       client_sock = accept(server_sock, (struct sockaddr *)&caddr, &length);
       if (client_sock < 0){
          printf("server: accept error\n");
          exit(1);
       }
 
       printf("server: accepted a client connection from\n");
       printf("-----------------------------------------------\n");
       printf("    IP=%s  port=%d\n", "127.0.0.1", ntohs(caddr.sin_port));
       printf("-----------------------------------------------\n");

       // Processing loop
       while(1){
         printf("server ready for next request ....\n");
         n = read(client_sock, line, MAX);
         if (n==0){
           printf("server: client died, server loops\n");
           close(client_sock);
           break;
         }
         line[n]=0;
         printf("server: read  n=%d bytes; line=[%s]\n", n, line);
         sscanf(line, "%s %s", command, pathname);
      printf("command=%s pathname=%s\n", command, pathname);
      if (command[0]==0) 
         continue;
      index = findCmd(command);   
     
      if (line[0]==0)                  // exit if NULL line
         exit(0);
        int r;
        char buf[MAX];
        char *currentPath;
       switch (index){
        case 0: r = mkdir(pathname, 0755); break;
        case 1: r = rmdir(pathname); break;
        case 2:  break;
        case 3: r = chdir(pathname);          break;
        case 4:currentPath = getcwd(buf, MAX);
            printf("Current Path: %s\n", currentPath);
        		break;
        case 5: r =  unlink(pathname);    break;
        case 6:  break; //get
        case 7: break; //put
      }
         
         strcat(line, " ECHO");
         // send the echo line to client 
         n = write(client_sock, line, MAX);

         printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
       }
    }
}



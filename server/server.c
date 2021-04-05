#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h> // for dirname()/basename()
#include <sys/stat.h>
#include <time.h>

#include "shared/info.h"
#include "server.h"

#define MAX 256
#define BLK 1024

int server_sock, client_sock; // file descriptors for sockets
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

//                        0         1     2     3     4      5     6    7
char *local_cmd[] = {"mkdir", "rmdir", "ls", "cd", "pwd", "rm", "get", "put", "quit", 0};
struct sockaddr_in saddr, caddr; // socket addr structs
int ls_file(char *fname)
{
  char lsLine[MAX];
  int n;
        struct stat fstat, *sp;
        int r, i;
        char ftime[64];
        sp = &fstat;
        if ((r = lstat(fname, &fstat)) < 0) {
            sprintf(lsLine, "can’t stat %s\n", fname);
            n = write(client_sock, lsLine, MAX);
                exit(1);
                // 256 8 System Calls for File Operations
        }
        if ((sp->st_mode & 0xF000) == 0x8000){ // if (S_ISREG())
            sprintf(lsLine, "%c", '-');
          n = write(client_sock, lsLine, MAX);       
        }
        if ((sp->st_mode & 0xF000) == 0x4000){ // if (S_ISDIR())
            sprintf(lsLine,"%c", 'd');
            n = write(client_sock, lsLine, MAX);      
        }       
        if ((sp->st_mode & 0xF000) == 0xA000){ // if (S_ISLNK())
              sprintf(lsLine,"%c", 'l');
              n = write(client_sock, lsLine, MAX);
        }
        for (i = 8; i >= 0; i--) {
                if (sp->st_mode & (1 << i)){ // print r|w|x
                      sprintf(lsLine,"%c", t1[i]);
                      n = write(client_sock, lsLine, MAX);
                }
                else{
                        sprintf(lsLine,"%c", t2[i]);
                        n = write(client_sock, lsLine, MAX);
                }
        }
        sprintf(lsLine,"%4d ", sp->st_nlink);
        n = write(client_sock, lsLine, MAX);
        sprintf(lsLine,"%4d ", sp->st_gid);
        n = write(client_sock, lsLine, MAX);
        sprintf(lsLine,"%4d ", sp->st_uid);
        n = write(client_sock, lsLine, MAX);
        sprintf(lsLine,"%8ld ", sp->st_size);
        n = write(client_sock, lsLine, MAX);
        strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
        ftime[strlen(ftime) - 1] = 0; // kill \n at end
        sprintf(lsLine,"%s ", ftime);
        n = write(client_sock, lsLine, MAX);
        // print name
        sprintf(lsLine,"%s", basename(fname)); // print file basename
        n = write(client_sock, lsLine, MAX);
        // print -> linkname if symbolic file
        if ((sp->st_mode & 0xF000) == 0xA000) {
                // use readlink() to read linkname

                // printf(" -> %s", linkname); // print linked name
        }
        sprintf(lsLine,"\n");
        n = write(client_sock, lsLine, MAX);
        return 0;
}

int ls_dir(char *dname)
{
        // use opendir(), readdir(); then call ls_file(name)
        DIR *dir = opendir(dname);
        struct dirent *dp = NULL;
        dp = readdir(dir);
        do {
                ls_file(dp->d_name);
                dp = readdir(dir);
        } while(dp != NULL);
        return 0;
}

int ls(char *pathname)
{
        char lsLine[MAX];
        struct stat mystat, *sp = &mystat;
        char *filename, path[1024], cwd[256];
        filename = "./"; // default to CWD
        if (strcmp(pathname, "") != 0)
                filename = pathname;// if specified a filename
        if (lstat(filename, sp) < 0) {
                sprintf(lsLine,"no such file %s\n", filename);
                write(client_sock, lsLine, MAX);
                exit(1);
        }

        strcpy(path, filename);
        if (path[0] != '/') { // filename is relative : get CWD path
                getcwd(cwd, 256);
                strcpy(path, cwd);
                strcat(path, "/");
                strcat(path, filename);
        }
        if (S_ISDIR(sp->st_mode))
                ls_dir(path);
        else
                ls_file(path);
        return 0;
}

void init()
{
        char buf[MAX];
        getcwd(buf, MAX);
        if(chroot(buf) == -1) {
                printf("Error chroot: %d %s", errno, strerror(errno));
        }


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
                printf("Error: %d %s\n", errno, strerror(errno));
                exit(0);
        }
        printf("4. server listen with queue size = 5\n");
        if ((listen(server_sock, 5)) != 0) {
                printf("Listen failed\n");
                exit(0);
        }
        printf("5. server at IP=%s port=%d\n", serverIP, serverPORT);
}

int findCmd(char *command) // finding the cmd for main menu
{
        int i = 0;
        while (local_cmd[i]) {
                if (strcmp(command, local_cmd[i]) == 0)
                        return i;
                i++;
        }
        return -1;
}

int main()
{
        int n;
        unsigned int length;
       char line[MAX];

        int index;

        char command[16], pathname[64];

        init();

        while (1) {
                printf("server: try to accept a new connection\n");
                length = sizeof(caddr);
                client_sock = accept(server_sock, (struct sockaddr *)&caddr, &length);
                if (client_sock < 0) {
                        printf("server: accept error\n");
                        printf("Error: %d %s\n", errno, strerror(errno));
                        exit(1);
                }

                printf("server: accepted a client connection from\n");
                printf("-----------------------------------------------\n");
                printf("    IP=%s  port=%d\n", "127.0.0.1", ntohs(caddr.sin_port));
                printf("-----------------------------------------------\n");

                // Processing loop
                while (1) {
                        printf("server ready for next request ....\n");
                        n = read(client_sock, line, MAX);
                        if (n == 0) {
                                printf("server: client died, server loops\n");
                                close(client_sock);
                                break;
                        }
                        line[n] = 0;
                        printf("server: read  n=%d bytes; line=[%s]\n", n, line);
                        sscanf(line, "%s %s", command, pathname);
                        printf("command=%s pathname=%s\n", command, pathname);
                        if (command[0] == 0)
                                continue;
                        index = findCmd(command);

                        if (line[0] == 0) // exit if NULL line
                                exit(0);
                        int r = 0;
                        char buf[MAX];
                        char *currentPath;
                        switch (index) {
                        case 0:
                                r = mkdir(pathname, 0755);
                                strcat(line, " ECHO");
                                //send the echo line to client
                                 n = write(client_sock, line, MAX);
                                break;
                        case 1:
                                r = rmdir(pathname);
                                strcat(line, " ECHO");
                                //send the echo line to client
                                 n = write(client_sock, line, MAX);
                                break;
                        case 2:
                                ls(pathname);
                               //strcat(line, " ECHO");
                                //send the echo line to client
                                n = write(client_sock, "DONE", MAX);
                                break;
                        case 3:
                                r = chdir(pathname);
                                strcat(line, " ECHO");
                                //send the echo line to client
                                 n = write(client_sock, line, MAX);
                                break;
                        case 4:
                                currentPath = getcwd(buf, MAX);
                                sprintf(line, "Current Path: %s\n", currentPath);
                                strcat(line, " ECHO");
                                //send the echo line to client
                                 n = write(client_sock, line, MAX);                                
                                break;
                        case 5:
                                r = unlink(pathname);
                                strcat(line, " ECHO");
                                //send the echo line to client
                                 n = write(client_sock, line, MAX);
                                break;
                        case 6:
                                get(pathname);
                                break; // get
                        case 7:
                                put(pathname);
                                break; // put

                        case 8:
                                exit(0);
                        }
                        if (r != 0) {
                                sprintf(line, "Error: %d %s", errno, strerror(errno));
                        }

//                        strcat(line, " ECHO");
//                        // send the echo line to client
//                        n = write(client_sock, line, MAX);

                        //printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
                }
        }
}


void put(const char *pathname)
{
        char buffer[MAX];
        int file_size = 0;
        int total_read = 0;
        int bytes_read = 0;

        int fd = open(pathname, O_CREAT | O_TRUNC | O_RDWR);
        if (fd != -1) {
                // Get the file size.
                bytes_read = read(client_sock, buffer, MAX);
                file_size = atoi(buffer);

                while (bytes_read != 0 && total_read != file_size) {
                        // Get size of transfer.
                        bytes_read = read(client_sock, buffer, MAX);
                        total_read += atoi(buffer);

                        if (bytes_read == 0)
                                break;

                        bytes_read = read(client_sock, buffer, MAX);

                        if(bytes_read == 0)
                                break;
                        write(fd, buffer, bytes_read);
                }
        }
        close(fd);
}

void get(const char *pathname)
{
        struct stat st;
        char buffer[MAX];
        char size_buffer[MAX];
        int bytes_read = 0;
        int bytes_sent = 0;

        int fd = open(pathname, O_RDWR);
        if (fd != -1) {
                stat(pathname, &st);
                sprintf(buffer, "%ld", st.st_size);
                write(client_sock, buffer, MAX); // Send the size of the file.
                printf("Sent: %ld as file size.", st.st_size);
                bytes_read = read(fd, buffer, MAX);
                while(bytes_read != 0) {
                        sprintf(size_buffer, "%d", bytes_read);
                        bytes_sent = write(client_sock, size_buffer, MAX); // Write size of line sent.
                        if (bytes_sent == -1) {
                                printf("Error sending file. %s", strerror(errno));
                        }
                        bytes_sent = write(client_sock, buffer, bytes_read);
                        if (bytes_sent == -1) {
                                printf("Error sending file. %s", strerror(errno));
                        }
                        bytes_read = read(fd, buffer, MAX);
                }
        }
        close(fd);
}

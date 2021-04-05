#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h> // for dirname()/basename()
#include <sys/stat.h>
#include <time.h>

#include "client.h"
#include "shared/info.h"

#define MAX 256
#define BLK 1024


//                            0           1      2     3     4       5       6        7       8
const char *local_cmd[] = {"lmkdir", "lrmdir", "lls", "lcd", "lpwd", "lrm", "lcat", "ls","put", 0};
char *t1 = "xwrxwrxwr-------";
char *t2 = "----------------";

int ls_file(char *fname)
{
        struct stat fstat, *sp;
        int r, i;
        char ftime[64];
        sp = &fstat;
        if ((r = lstat(fname, &fstat)) < 0) {
                printf("can’t stat %s\n", fname);
                exit(1);
                // 256 8 System Calls for File Operations
        }
        if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
                printf("%c", '-');
        if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
                printf("%c", 'd');
        if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
                printf("%c", 'l');
        for (i = 8; i >= 0; i--) {
                if (sp->st_mode & (1 << i)) // print r|w|x
                        printf("%c", t1[i]);
                else
                        printf("%c", t2[i]); // or print -
        }
        printf("%4d ", sp->st_nlink); // link count
        printf("%4d ", sp->st_gid); // gid
        printf("%4d ", sp->st_uid); // uid
        printf("%8d ", sp->st_size); // file size
        // print time
        strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
        ftime[strlen(ftime) - 1] = 0; // kill \n at end
        printf("%s ", ftime);
        // print name
        printf("%s", basename(fname)); // print file basename
        // print -> linkname if symbolic file
        if ((sp->st_mode & 0xF000) == 0xA000) {
                // use readlink() to read linkname

                // printf(" -> %s", linkname); // print linked name
        }
        printf("\n");
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
        struct stat mystat, *sp = &mystat;
        int r;
        char *filename, path[1024], cwd[256];
        filename = "./"; // default to CWD
        if (strcmp(pathname, "") != 0)
                filename = pathname;// if specified a filename
        if ((r = lstat(filename, sp) < 0)) {
                printf("no such file %s\n", filename);
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

int init()
{
        struct sockaddr_in saddr;
        int sock = 0;

        printf("1. create a socket\n");
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
                printf("socket creation failed\n");
                exit(0);
        }

        printf("2. fill in server IP=%s, port number=%d\n", serverIP, serverPORT);
        memset(&saddr, 0, sizeof(struct sockaddr_in));
        saddr.sin_family = AF_INET;
        saddr.sin_addr.s_addr = inet_addr(serverIP);
        saddr.sin_port = htons(serverPORT);

        printf("3. connect to server\n");
        if (connect(sock, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
                printf("connection with the server failed...\n");
                exit(0);
        }
        printf("4. connected to server OK\n");

        return sock;
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

int run_client(int argc, char *argv[])
{
        int n;
        char command[16], pathname[64];
        char line[MAX], ans[MAX];
        int index;

        int sock = init();

        while (1) {
                printf("****************************MENU*************************\n");
                printf("get put ls cd pwd mkdir rmdir rm quit\n");
                printf("lcat lls lcd lpwd lmkdir lrmdir lrm\n");
                printf("*****************************************************\n");
                printf("enter command Line : ");
                fgets(line, MAX, stdin);
                line[strlen(line) - 1] = 0;
                printf("line=%s\n", line);
                memset(pathname, 0, 64);
                sscanf(line, "%s %s", command, pathname);
                printf("command=%s pathname=%s\n", command, pathname);
                if (command[0] == 0)
                        continue;
                index = findCmd(command);

                if (strcmp(line, "") == 0) // exit if NULL line
                        exit(0);
                switch (index) {
                case 0:
                        lmkdir(pathname);
                        break;
                case 1:
                        lrmdir(pathname);
                        break;
                case 2:
                        ls(pathname);
                        break; // ls
                case 3:
                        lcd(pathname);
                        break;
                case 4:
                        lpwd();
                        break;
                case 5:
                        lrm(pathname);
                        break;
                case 6:
                        lcat(line);
                        break; // cat
                case 7:
                    send_to_server(line, sock);
                    bzero(ans, MAX);
                    while(strcmp(ans, "DONE") != 0){
                    n = read(sock, ans, MAX);
                    if(strcmp(ans, "DONE") != 0){
                     printf("%s",ans);
                    }
                    }
                        break;
                case 8:
                        send_to_server(line, sock);

                        put(pathname, sock);
                        // Read a line from sock and show it
                        bzero(ans, MAX);
                        n = read(sock, ans, MAX);
                        printf("client: read  n=%d bytes; echo=(%s)\n", n, ans);
                        break;
                case 9:
                        send_to_server(line, sock);
                        get(pathname, sock);
                        break;
                case 10:
                        exit(0);
                case -1:
                        send_to_server(line, sock);
                        // Read a line from sock and show it
                        bzero(ans, MAX);
                        n = read(sock, ans, MAX);
                        printf("client: read  n=%d bytes; echo=(%s)\n", n, ans);
                        break;
                }
                // line[strlen(line)-1] = 0;        // kill \n at end
        }
}

void lmkdir(const char *pathname)
{
        int status = mkdir(pathname, 0755);
        if (status != 0) {
                printf("Error: mkdir %d %s", errno, strerror(errno));
        }
}

void lrmdir(const char *pathname)
{
        int status = rmdir(pathname);
        if (status != 0) {
                printf("Error: rmdir %d %s", errno, strerror(errno));
        }
}

void lcd(const char *pathname)
{
        int status = chdir(pathname);
        if (status != 0) {
                printf("Error: chdir %d %s", errno, strerror(errno));
        }
}

void lpwd()
{
        char buf[MAX];
        const char *currentPath = getcwd(buf, MAX);
        printf("Current Path: %s\n", currentPath);
}

void lrm(const char *pathname)
{
        int status = unlink(pathname);
        if (status != 0) {
                printf("Error: rm %d %s", errno, strerror(errno));
        }
}

void lcat(char *line)
{
        int i = 0, j = 0;
        for (i = 0; line[i] != '\0'; i++) {
                if (line[i] != 'l') {
                        line[j++] = line[i];
                }
        }
        line[j] = '\0';
        printf("LINE:%s\n", line);
        system(line);
}

void send_to_server(const char *line, int sock)
{
        int n = write(sock, line, MAX);
        printf("client: wrote n=%d bytes; line=(%s)\n", n, line);
}

void put(const char *pathname, int sock)
{
        struct stat st;
        char buffer[MAX];
        char size_buffer[MAX];
        int bytes_read = 0;
        int bytes_sent = 0;

        int fd = open(pathname, O_RDONLY);
        if (fd != -1) {
                stat(pathname, &st);
                sprintf(buffer, "%ld", st.st_size);
                write(sock, buffer, MAX); // Send the size of the file.
                printf("Sent: %ld as file size.", st.st_size);
                bytes_read = read(fd, buffer, MAX);
                while (bytes_read != 0) {
                        sprintf(size_buffer, "%d", bytes_read);
                        bytes_sent = write(sock, size_buffer, MAX); // Write size of line sent.
                        if (bytes_sent == -1) {
                                printf("Error sending file. %s", strerror(errno));
                        }
                        bytes_sent = write(sock, buffer, bytes_read);
                        if (bytes_sent == -1) {
                                printf("Error sending file. %s", strerror(errno));
                        }
                        bytes_read = read(fd, buffer, MAX);
                }
        }
        close(fd);
}


void get(const char *pathname, int sock)
{
        char buffer[MAX];
        int file_size = 0;
        int total_read = 0;
        int bytes_read = 0;

        int fd = open(pathname, O_CREAT | O_TRUNC | O_RDWR);
        fchmod(fd, 0755);
        if (fd != -1) {
                // Get the file size.
                bytes_read = read(sock, buffer, MAX);
                file_size = atoi(buffer);

                while (bytes_read != 0 && total_read != file_size) {
                        memset(buffer, 0, MAX);
                        // Get size of transfer.
                        bytes_read = read(sock, buffer, MAX);
                        total_read += atoi(buffer);

                        if (bytes_read == 0)
                                break;

                        bytes_read = read(sock, buffer, MAX);

                        if(bytes_read == 0)
                                break;
                        write(fd, buffer, bytes_read);
                }
        }
        close(fd);
}
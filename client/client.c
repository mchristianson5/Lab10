#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h> // for dirname()/basename()
#include <sys/stat.h>
#include <time.h>

#include "client.h"
#include "shared/info.h"

#define MAX 256
#define BLK 1024

//                            0           1      2     3     4       5       6        7
const char *local_cmd[] = {"lmkdir", "lrmdir", "lls", "lcd", "lpwd", "lrm", "lcat", "put", 0};

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

int run_client()
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
                        break;
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
                        put(pathname, sock);
                        break;
                case -1:
                        send_to_server(line, sock);
                        break;
                }
                // line[strlen(line)-1] = 0;        // kill \n at end

                // Read a line from sock and show it
                bzero(ans, MAX);
                n = read(sock, ans, MAX);
                printf("client: read  n=%d bytes; echo=(%s)\n", n, ans);
        }
}

void lmkdir(const char *pathname)
{
        int status = mkdir(pathname, 0755);
        if (status != 0)
        {
                printf("Error: mkdir %d %s", errno, strerror(errno));
        }
}

void lrmdir(const char *pathname)
{
        int status = rmdir(pathname);
        if (status != 0)
        {
                printf("Error: rmdir %d %s", errno, strerror(errno));
        }
}

void lcd(const char *pathname)
{
        int status = chdir(pathname);
        if (status != 0)
        {
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
        if (status != 0)
        {
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
        int bytes_read = 0;
        int bytes_sent = 0;

        int fd = open(pathname, O_RDONLY);
        if (fd != -1) {
                stat(pathname, &st);
                sprintf(buffer, "%ld", st.st_size);
                write(sock, buffer, MAX); // Send the size of the file.
                printf("Sent: %ld as file size.", st.st_size);
                bytes_read = read(fd, buffer, MAX);
                while(bytes_read != 0) {
                        sprintf(buffer, "%d", bytes_read);
                        bytes_sent = write(sock, buffer, MAX); // Write size of line sent.
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

#ifndef CLIENT_H
#define CLIENT_H

int init();
int findCmd(char *command);
int run_client();
void send_to_server(const char *line, int sock);

// Commands
void lmkdir(const char *pathname);
void lrmdir(const char *pathname);
void lcd(const char *pathname);
void lpwd();
void lrm(const char *pathname);
void lcat(char *line);
void put(const char *pathname, int sock);
void get(const char *pathname, int sock);

#endif // CLIENT_H

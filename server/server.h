#ifndef SERVER_H
#define SERVER_H

void init();
void put(const char *pathname);
void get(const char *pathname);
int ls_file(char *fname);
int ls_dir(char *dname);
int ls(char *pathname);

#endif // SERVER_H

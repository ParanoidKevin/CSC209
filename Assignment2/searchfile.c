#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

extern void findb(DIR *dirm, char *pathn);

int main(int argc, char **argv)
{
	if (argc <= 1) {
		fprintf(stderr, "usage: findbin file...\n");
		exit(1);
	}int i;
	DIR *mdir;
	for (i = 1; i < argc; i++) {
		if ((mdir = opendir(argv[i]))) {
			findb(mdir, argv[i]);
		}else {
			fprintf(stderr, "%s: invaild path\n", argv[i]);
			exit(1);
		}
	}return(0);
}


void findb(DIR *mdir, char *pathn) {
	//printf("%s\n", pathn);
	//DIR *mdir = d;
	struct dirent *dir;
	struct stat st;
	while ((dir = readdir(mdir)) != NULL) {
		printf("777\n");
		printf("%s\n", dir->d_name);
		if (lstat(dir->d_name, &st) == 0) {
			printf("%s\n", dir->d_name);
		}else {
			perror("lstat");
        	exit(EXIT_FAILURE);
		}
		//struct stat st;
		if (S_ISDIR(st.st_mode) == 0 && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
			//printf("%s\n", dir->d_name);
			//printf("999\n");
			DIR *new;
			char newpath[2000];
			strcpy(newpath, pathn);
			strcat(newpath, "/");
			strcat(newpath, dir->d_name);
			//printf("%s\n", newpath);
			if ((new = opendir(dir->d_name))) {
				//printf("%s\n", dir->d_name);
				//printf("%s\n", newpath);
				findb(opendir(newpath), newpath);
			}
		}else if (S_ISREG(st.st_mode) == 0) {
			//printf("%s\n", dir->d_name);
			FILE *file;
			unsigned char k[4];
			unsigned int dig[1];
			file = fopen(dir->d_name, "r");
			if (fread(k, 4, 1, file) && fread(dig, 1, 1, file)) {
				//printf("%c\n", dig[0]);
				//printf("%c\n", k[0]);
				if (k[1] == 69 && k[2] == 76 && k[3] == 70 && dig[0] == 127){
					printf("%s", pathn);
					printf("/%s\n", dir->d_name);
				}
			}else {
				fprintf(stderr, "%s: can not open\n", dir->d_name);
				exit(1);
			}
		}
	}
}



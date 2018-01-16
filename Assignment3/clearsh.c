#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

int main(int argc, char **argv)
{
	extern char **environ;
	char **result = NULL;
	char *m_str = (char *)malloc(1000 * sizeof(char));
	char **m_ary = (char **)malloc(10 * sizeof(m_str));
	char *pre = (char *)malloc(1000 * sizeof(char));
	int ary_index = 0;
	int ary_size = 10;
	int x;
	char *sp;
	if (argc == 1 || (argc == 2 && (strcmp(argv[1], "-e")) == 0)) {
		while (fgets(m_str, 1000, stdin)) {
			if ((strcmp(pre, "\n")) != 0 && (strcmp(m_str, "\n")) == 0) {
				/* user wants to execute the command */
				result = (char **)malloc((ary_index + 1) * sizeof(m_str));
				x = fork();
				if (x == -1) {
					perror("fork");
					return(1);
				}else if(x == 0) {
					/* child */
					/* check if the command contain '/' and make a new array */
					int i;
					for (i = 0; i < ary_index; i++) {
						result[i] = m_ary[i];
					}result[ary_index] = NULL;
					if ((sp = strchr(result[0], '/')) == NULL) {
						/* '/' found do the PATH search */
						char *temp = NULL;
						DIR *mdir;
						if ((mdir = opendir("/bin")) == NULL) {
							perror("opendir");
							return(1);
						}struct dirent *dir;
						while ((dir = readdir(mdir)) != NULL) {
							if ((strcmp(dir->d_name, result[0])) == 0) {
								/* command found in "/bin" */
								temp = (char *)malloc(1000 * sizeof(char));
								strcat(temp, "/bin/");
								strcat(temp, result[0]);
								//m_ary[0] = temp;
								execve(temp, result, environ);
								perror(temp);
								return(1);
							}
						}if ((mdir = opendir("/usr/bin")) == NULL) {
							perror("opendir");
							return(1);
						}while ((dir = readdir(mdir)) != NULL) {
							if ((strcmp(dir->d_name, result[0])) == 0) {
								/* command found in "/usr/bin" */
								temp = (char *)malloc(1000 * sizeof(char));
								strcat(temp, "/usr/bin/");
								strcat(temp, result[0]);
								execve(temp, result, environ);
								perror(temp);
								return(1);
							}
						}if ((mdir = opendir(".")) == NULL) {
							perror("opendir");
							return(1);
						}while ((dir = readdir(mdir)) != NULL) {
							if ((strcmp(dir->d_name, result[0])) == 0) {
								/* command found in "." */
								temp = (char *)malloc(1000 * sizeof(char));
								strcat(temp, "/./");
								strcat(temp, result[0]);
								execve(temp, result, environ);
								perror(temp);
								return(1);
							}
						}fprintf(stderr, "%s: No such file or directory\n", m_ary[0]);
						exit(1);
					}else {
						/* '/' not found just run execve */
						execve(m_ary[0], m_ary, NULL);
						perror(m_ary[0]);
						return(1);
					}
				}else {
					/* parent */
					/* wait for the child process ends and reset everything */
					pid_t pid;
					int status;
					if ((pid = wait(&status)) == -1) {
						perror("wait");
						return(1);
					}if (argc == 2 && (strcmp(argv[1], "-e")) == 0) {
						printf("exit status %d\n", WEXITSTATUS(status));
					}
					/* child process ends */
					ary_index = 0;
					ary_size = 10;
					m_str = (char *)malloc(1000 * sizeof(char));
					m_ary = (char **)malloc(10 * sizeof(m_str));
					pre = (char *)malloc(1000 * sizeof(char));
				}
			}else {
				/* user not finish entering data */
				pre = m_str;
				if ((sp = strchr(m_str, '\n')) != NULL) {
					/* delete the newline at the end of m_str */
					*sp = '\0';
				}
				if (ary_index > ary_size) {
					/* increase the array size if user enters more commands */
					ary_size = ary_size + 10;
					m_ary = (char **)realloc(m_ary, (ary_size * sizeof(m_str)));
				}
				m_ary[ary_index] = m_str;
				m_str = (char *)malloc(1000 * sizeof(char));
				ary_index++;
			}
		}
	return(0);
	}else {
		fprintf(stderr, "usage: clearsh [-e]\n");
		return(1);
	}
}
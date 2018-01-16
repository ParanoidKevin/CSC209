#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main (int argc, char **argv)
{
	char* wall = "*";
	int width = 70;
	int c;
	int cross = 1;
	
	while ((c = getopt (argc, argv, "ew:c:*")) != -1) {
		switch (c) {
			case 'w':
				width = atoi(optarg);
				break;
			case 'c':
				wall = optarg;
				break;
			case 'e':
				cross = 0;
				break;
			case '*':
				//printf("%s\n", optarg);
				break;
			default:
				abort ();
		}printf("%s\n", argv[optind]);
		if (argv[optind][0] != '-' || (strlen(argv[optind]) >1 && argv[optind][0] == '-' && argv[optind][1] != 'e' && argv[optind][1] != 'w' && argv[optind][1] != 'c')) {
			//printf("123\n");
			for (c = 0; c < width - 1; c++) {
				printf("%s", wall);
			}printf("%s\n", wall);
			int i;
			char line[300];
			FILE *file;
			char *sp;
			for (i = optind; i < argc; i++) {
				if((file = fopen(argv[i], "r")) != NULL) {
					while (fgets(line, sizeof(line), file)) {
						printf("%s ", wall);
						if ((sp = strchr(line, '\n')) != NULL) {
							*sp = '\0';
						}if (strlen(line) <= width -4) {
							int tmp;
							printf("%s", line);
							for (tmp = strlen(line);tmp < width - 4 ; tmp++) {
								printf(" ");
							}
						}else if (cross == 1) {
							printf("%.*s", width - 4, line);
						}else if (cross == 0) {
							printf("%s", line);
						}printf(" %s\n", wall);
					}
				}else {
					fprintf(stderr, "%s: No such file or directory\n", argv[i]);
				}fclose(file);
			}for (c = 0; c < width - 1; c++) {
	                printf("%s", wall);
	        }printf("%s\n", wall);
		}
	}printf("%s\n", argv[optind]);
	if ((optind + 1 == argc && strcmp(argv[optind], "-") == 0) || argc == optind) {
		for (c = 0; c < width - 1; c++) {
                        printf("%s", wall);
                }printf("%s\n", wall);
		int count = 0;
		char result[256];
		char get;
		while ((get = getchar()) != EOF) {
			if (get == '\n') {
				count = 0;
				if (strlen(result) != 0) {
					printf("%s %s", wall, result);
					int tmp1;
					for (tmp1 = strlen(result); tmp1 < width -4; tmp1++) {
						printf(" ");
					}printf(" %s\n", wall);
				}else {
					printf("%s %s", wall, result);
					int tmp2;
					for (tmp2 = 0; tmp2 < width - 4; tmp2++) {
						printf(" ");
					}printf(" %s\n", wall);
				}memset(result,0,sizeof(result));
			}else if (count < width - 4 || cross == 0) {
				result[count] = get;
				count++;
			}
		}for (c = 0; c < width - 1; c++) {
                        printf("%s", wall);
                }printf("%s\n", wall);
	}
}
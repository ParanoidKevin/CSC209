#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *mystrstr(char *s, char *t) {
    char *full = s;
    char *comp = t;
    while (*full != '\0' && *comp != '\0') {
        if (*full == *comp) {
            char *new = full;
            while (*new != '\0' && *comp != '\0' && *new == *comp) {
                new++;
                comp++;
            }if (*comp == '\0') {
                return full;
            }else {
                comp = t;
            }
        }full++;
    }return NULL;
}

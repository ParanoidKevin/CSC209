#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int from, to, del, c;

    if (argc != 3) {
        fprintf(stderr, "usage: vstr {fromchar tochar | -d char}\n");
        return(1);
    }

    if (strcmp(argv[1], "-d") == 0) {

        del = argv[2][0];

        while ((c = getchar()) != EOF) {
            if (c != del){
                putchar(c);
		}

    if (strcmp(argv[1], "-d") == 0) {

        del = argv[2][0];

        while ((c = getchar()) != EOF) {
            if (c != del){
                putchar(c);
            }
        }

    } else {

        from = argv[1][0];
        to = argv[2][0];

        while ((c = getchar()) != EOF) {
            if (c == from){

}

    } else {

        from = argv[1][0];
        to = argv[2][0];

        while ((c = getchar()) != EOF) {
            if (c == from){
                putchar(to);
            }else{
                putchar(c);
            }
        }

    }

    return(0);
}

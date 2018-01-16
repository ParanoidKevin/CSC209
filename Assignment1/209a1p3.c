/*
 * squ -- "squeeze" adjacent blank lines.  That is, the output has no more
 *        than one blank line in a row, but is otherwise a copy of the input.
 */

#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc == 2){
	    int c;
	    enum { START, SAW_NL, SAW_TWO_NL } state = START;
	    while ((c = getchar()) != EOF) {
	    	if (c == 10){
	    		if (state == START){
	    			putchar(c);
	    			state = SAW_NL;
	    		}else if (state == SAW_NL){
	    			putchar(c);
	    			state = SAW_TWO_NL;
	    		}
	    	}else{
	    		state = START;
	    		putchar(c);
	    	}
	    }return (0);
	}else{
		fprintf(stderr, "usage: squ format error\n");
		return (1);
	}
}
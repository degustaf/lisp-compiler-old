#include <stdio.h>
#include <unistd.h>

#include "Repl.h"
#include "RunTime.h"
#include "Symbol.h"
#include "Var.h"

void cmd_line_err(void) {
    printf("Usage: \n");
}

int main(int argc, char **argv) {
    int c;
    __attribute__((unused)) char *out_file = "a.out";

    while((c = getopt(argc, argv, ":o:")) != -1) {
        switch(c) {
        case 'o':
            out_file = optarg;
            break;
        case ':':
            fprintf(stderr, "Option -%c requires an operand\n", optopt);
            cmd_line_err();
            break;
        case '?':
            fprintf(stderr, "Unrecognized option: '-%c'\n", optopt);
            cmd_line_err();
            break;
        default:
            fprintf(stderr, "Unexpected result from getopt:  '%c'\n", c);
        }
    }

	initRT();
	printf("Finished initRT()\n");
	fflush(stdout);
	// const Symbol *lisp_main = internSymbol1("lisp.main");
	__attribute__((unused)) const Var *require = RTVar("lisp.core", "require");
	__attribute__((unused)) const Var *MAIN = RTVar("lisp.main", "main");

	// ((const IFn*)require)->obj.fns->IFnFns->invoke1((const IFn*)require, (const lisp_object*)MAIN);
	repl(stdin);

	return 0;
}

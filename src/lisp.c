#include <stdio.h>
#include <unistd.h>

#include "llvm-c/Types.h"
#include "llvm-c/Core.h"

#include "Repl.h"

void cmd_line_err(void) {
    printf("Usage: \n");
}

int main(int argc, char **argv) {
    int c;
    char *out_file = "a.out";
    LLVMContextRef context = LLVMContextCreate();

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

    repl(stdin);

    return 0;
}


#include "Repl.h"

#include "LispObject.h"
#include "Reader.h"

static void HandleTopLevelExpression(lisp_object *const current);

void repl(FILE *input) {
    for(;;) {
        printf("Lisp>");
        lisp_object *current = read(input, false, '\0');
        switch(current->type) {
            case EOF_type:
                printf("\n");
                return;
            default:
                HandleTopLevelExpression(current);
                break;
        }
    }
}

static void HandleTopLevelExpression(lisp_object *const current) {
    if(current) {
        printf("Parsed a top level expression.\n");
    }
}

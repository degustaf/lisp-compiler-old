
#include "Repl.h"

#include "Lexer.h"
#include "Parser.h"

static void HandleTopLevelExpression(FILE *input, token *const current);

void repl(FILE *input) {
    for(;;) {
        printf("Lisp>");
        token current = gettok(input);
        switch(current.type) {
            case TOKEN_EOF:
                return;
            default:
                HandleTopLevelExpression(input, &current);
                break;
        }
    }
}

static void HandleTopLevelExpression(FILE *input, token *const current) {
    lisp_object *obj = Parser(input, current);
    if(obj) {
        printf("Parsed a top level expression.\n");
    }
}

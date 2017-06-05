#include "Repl.h"

#include <assert.h>

#include "LispObject.h"
#include "Compiler.h"
#include "Reader.h"

static void HandleTopLevelExpression(lisp_object *const current);

void repl(FILE *input) {
    init_macros();
    for(;;) {
        printf("Lisp>");
        lisp_object *current = read(input, false, '\0');
		assert(current);
		switch(current->type) {
            case EOF_type:
			case ERROR_type:
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
		const lisp_object *result = Eval(current);
		printf("%s\n", result->toString(result));
    }
}

#include "ARef.h"

#include "Error.h"	// TODO convert Errors to use setjmp/longjmp.

void Validate(bool (*validator)(const lisp_object*), const lisp_object *val) {
	// TRY
	if(validator && !validator(val))
		NULL;	// Throw excexption
	// Catch
}

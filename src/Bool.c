#include "Bool.h"

#include <assert.h>
#include <stdbool.h>

#include "Util.h"

struct Bool_struct {
	lisp_object obj;
	bool val;
};


static const char* toStringTrue(const lisp_object *obj);
static const char* toStringFalse(const lisp_object *obj);

const Bool _True =  {{BOOL_type, toStringTrue, NULL, EqualBase, NULL, &NullInterface}, true};
const Bool *const True = &_True;
const Bool _False =  {{BOOL_type, toStringFalse, NULL, EqualBase, NULL, &NullInterface}, false};
const Bool *const False = &_False;

static const char* toStringTrue(const lisp_object *obj) {
	assert((void*)obj == (void*)&_True);
	return "true";
}

static const char* toStringFalse(const lisp_object *obj) {
	assert((void*)obj == (void*)&_False);
	return "false";
}

#include "ARef.h"

#include "Error.h"
#include "StringWriter.h"

void Validate(bool (*validator)(const lisp_object*), const lisp_object *val) {
	TRY
		if(validator && !validator(val)) {
			exception e = {IllegalStateException, "Invalid reference state"};
			Raise(e);
		}
	EXCEPT(RuntimeExcp)
		ReRaise;
	EXCEPT(ANY)
		StringWriter *sw = NewStringWriter();
		AddString(sw, "Invalid reference state\n");
		AddString(sw, _ctx.id->msg);
		exception e = {IllegalStateException, WriteString(sw)};
		Raise(e);
	ENDTRY
}

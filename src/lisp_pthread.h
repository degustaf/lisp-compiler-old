#ifndef LISP_PTHREAD_H
#define LISP_PTHREAD_H

#ifdef MULTITHREAD
#include <pthread.h>
#else /* MULTITHREAD */

typedef void* pthread_t;

static inline pthread_t pthread_self(void) {
	return (pthread_t) 1;
}

#endif /* MULTITHREAD */

#endif /* LISP_PTHREAD_H */

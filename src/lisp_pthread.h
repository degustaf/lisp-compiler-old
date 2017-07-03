#ifndef LISP_PTHREAD_H
#define LISP_PTHREAD_H

#ifdef MULTITHREAD
#include <pthread.h>
#else /* MULTITHREAD */

typedef void* pthread_t;
typedef int pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER ((pthread_mutex_t)1)

static inline pthread_t pthread_self(void) {
	return (pthread_t) 1;
}

static inline int pthread_mutex_lock(__attribute__((unused)) pthread_mutex_t *x) {
	return 0;
}

static inline int pthread_mutex_unlock(__attribute__((unused)) pthread_mutex_t *x) {
	return 0;
}

#endif /* MULTITHREAD */

#endif /* LISP_PTHREAD_H */

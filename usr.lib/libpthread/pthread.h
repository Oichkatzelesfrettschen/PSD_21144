/*	$NetBSD: pthread.h,v 1.16 2003/12/07 20:29:07 christos Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LIB_PTHREAD_H
#define _LIB_PTHREAD_H

#include <sys/cdefs.h>

#include <time.h>	/* For timespec */
#include <sched.h>

#include <pthread_types.h>

__BEGIN_DECLS
int	pthread_atfork(void (*)(void), void (*)(void), void (*)(void));
int	pthread_create(pthread_t * __restrict, const pthread_attr_t * __restrict, void *(*)(void *), void * __restrict);
void 	pthread_exit(void *) __attribute__((__noreturn__));
int	pthread_join(pthread_t, void **);
int	pthread_equal(pthread_t, pthread_t);
pthread_t pthread_self(void);
int	pthread_detach(pthread_t);

int	pthread_attr_init(pthread_attr_t *);
int	pthread_attr_destroy(pthread_attr_t *);
int	pthread_attr_get_np(pthread_t, pthread_attr_t *);
int	pthread_attr_getguardsize(const pthread_attr_t *, size_t *);
int	pthread_attr_setguardsize(pthread_attr_t *, size_t);
int	pthread_attr_getinheritsched(const pthread_attr_t *, int *);
int	pthread_attr_setinheritsched(pthread_attr_t *, int);
int	pthread_attr_getschedparam(const pthread_attr_t *, struct sched_param *);
int	pthread_attr_setschedparam(pthread_attr_t *, const struct sched_param *);
int	pthread_attr_getscope(const pthread_attr_t *, int *);
int	pthread_attr_setscope(pthread_attr_t *, int);
int	pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
int	pthread_attr_setstack(pthread_attr_t *, void *, size_t);
int	pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
int	pthread_attr_setstacksize(pthread_attr_t *, size_t);
int	pthread_attr_getstackaddr(const pthread_attr_t *, void **);
int	pthread_attr_setstackaddr(pthread_attr_t *, void *);
int	pthread_attr_getdetachstate(const pthread_attr_t *, int *);
int	pthread_attr_setdetachstate(pthread_attr_t *, int);
int	pthread_attr_getname_np(const pthread_attr_t *, char *, size_t, void **);
int	pthread_attr_setname_np(pthread_attr_t *, const char *, void *);

int	pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
int	pthread_mutex_destroy(pthread_mutex_t *);
int	pthread_mutex_lock(pthread_mutex_t *);
int	pthread_mutex_trylock(pthread_mutex_t *);
int	pthread_mutex_unlock(pthread_mutex_t *);
int	pthread_mutexattr_init(pthread_mutexattr_t *);
int	pthread_mutexattr_destroy(pthread_mutexattr_t *);
int	pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
int	pthread_mutexattr_settype(pthread_mutexattr_t *attr, int);

int	pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
int	pthread_cond_destroy(pthread_cond_t *);
int	pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int	pthread_cond_timedwait(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);
int	pthread_cond_signal(pthread_cond_t *);
int	pthread_cond_broadcast(pthread_cond_t *);
int	pthread_condattr_init(pthread_condattr_t *);
int pthread_condattr_setclock(pthread_condattr_t *, clockid_t);
int	pthread_condattr_getclock(const pthread_condattr_t * __restrict, clockid_t * __restrict);
int	pthread_condattr_destroy(pthread_condattr_t *);

int	pthread_once(pthread_once_t *, void (*)(void));

int	pthread_key_create(pthread_key_t *, void (*)(void *));
int	pthread_key_delete(pthread_key_t);
int	pthread_setspecific(pthread_key_t, const void *);
void 	*pthread_getspecific(pthread_key_t);

int	pthread_cancel(pthread_t);
int	pthread_setcancelstate(int, int *);
int	pthread_setcanceltype(int, int *);
void 	pthread_testcancel(void);

int	pthread_getname_np(pthread_t, char *, size_t);
int	pthread_setname_np(pthread_t, const char *, void *);

int 	pthread_attr_setcreatesuspend_np(pthread_attr_t *);
int	pthread_suspend_np(pthread_t);
int	pthread_resume_np(pthread_t);

struct pthread_cleanup_store {
	void	*pad[4];
};

#define pthread_cleanup_push(routine, arg)			\
        {							\
		struct pthread_cleanup_store __store;		\
		pthread__cleanup_push((routine),(arg), &__store);

#define pthread_cleanup_pop(execute)				\
		pthread__cleanup_pop((execute), &__store);	\
	}

void	pthread__cleanup_push(void (*)(void *), void *, void *);
void	pthread__cleanup_pop(int, void *);

int	pthread_spin_init(pthread_spinlock_t *, int);
int	pthread_spin_destroy(pthread_spinlock_t *);
int	pthread_spin_lock(pthread_spinlock_t *);
int	pthread_spin_trylock(pthread_spinlock_t *);
int	pthread_spin_unlock(pthread_spinlock_t *);

int	pthread_rwlock_init(pthread_rwlock_t *, const pthread_rwlockattr_t *);
int	pthread_rwlock_destroy(pthread_rwlock_t *);
int	pthread_rwlock_rdlock(pthread_rwlock_t *);
int	pthread_rwlock_tryrdlock(pthread_rwlock_t *);
int	pthread_rwlock_wrlock(pthread_rwlock_t *);
int	pthread_rwlock_trywrlock(pthread_rwlock_t *);
int	pthread_rwlock_timedrdlock(pthread_rwlock_t *, const struct timespec *);
int	pthread_rwlock_timedwrlock(pthread_rwlock_t *, const struct timespec *);
int	pthread_rwlock_unlock(pthread_rwlock_t *);
int	pthread_rwlockattr_init(pthread_rwlockattr_t *);
int	pthread_rwlockattr_destroy(pthread_rwlockattr_t *);

int	pthread_barrier_init(pthread_barrier_t *, const pthread_barrierattr_t *, unsigned int);
int	pthread_barrier_wait(pthread_barrier_t *);
int	pthread_barrier_destroy(pthread_barrier_t *);
int	pthread_barrierattr_init(pthread_barrierattr_t *);
int	pthread_barrierattr_destroy(pthread_barrierattr_t *);

int	pthread_getschedparam(pthread_t, int *, struct sched_param *);
int	pthread_setschedparam(pthread_t, int, const struct sched_param *);

int 	*pthread__errno(void);
__END_DECLS

#define	PTHREAD_CREATE_JOINABLE	0
#define	PTHREAD_CREATE_DETACHED	1

#define PTHREAD_INHERIT_SCHED	0
#define PTHREAD_EXPLICIT_SCHED	1

#define PTHREAD_SCOPE_PROCESS	0
#define PTHREAD_SCOPE_SYSTEM	1

#define PTHREAD_PROCESS_PRIVATE	0
#define PTHREAD_PROCESS_SHARED	1

#define PTHREAD_CANCEL_DEFERRED		0
#define PTHREAD_CANCEL_ASYNCHRONOUS	1

#define PTHREAD_CANCEL_ENABLE		0
#define PTHREAD_CANCEL_DISABLE		1

#define PTHREAD_BARRIER_SERIAL_THREAD	1234567

/*
 * POSIX 1003.1-2001, section 2.5.9.3: "The symbolic constant
 * PTHREAD_CANCELED expands to a constant expression of type (void *)
 * whose value matches no pointer to an object in memory nor the value
 * NULL."
 */
#define PTHREAD_CANCELED	((void *) 1)

/*
 * Maximum length of a thread's name, including the terminating NUL.
 */
#define	PTHREAD_MAX_NAMELEN_NP			32

/*
 * Mutex attributes.
 */
#define	PTHREAD_MUTEX_NORMAL			0
#define	PTHREAD_MUTEX_ERRORCHECK		1
#define	PTHREAD_MUTEX_RECURSIVE			2
#define	PTHREAD_MUTEX_DEFAULT			PTHREAD_MUTEX_NORMAL

#define PTHREAD_COND_INITIALIZER		_PTHREAD_COND_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER		_PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_ONCE_INIT				_PTHREAD_ONCE_INIT
#define PTHREAD_RWLOCK_INITIALIZER		_PTHREAD_RWLOCK_INITIALIZER
#define PTHREAD_SPINLOCK_INITIALIZER	_PTHREAD_SPINLOCK_INITIALIZER

/*
 * Use macros to rename many pthread functions to the corresponding
 * libc symbols which are either trivial/no-op stubs or the real
 * thing, depending on whether libpthread is linked in to the
 * program. This permits code, particularly libraries that do not
 * directly use threads but want to be thread-safe in the presence of
 * threaded callers, to use pthread mutexes and the like without
 * unnecessairly including libpthread in their linkage.
 *
 * Left out of this list are functions that can't sensibly be trivial
 * or no-op stubs in a single-threaded process (pthread_create,
 * pthread_kill, pthread_detach), functions that normally block and
 * wait for another thread to do something (pthread_join), and
 * functions that don't make sense without the previous functions
 * (pthread_attr_*). The pthread_cond_wait and pthread_cond_timedwait
 * functions are useful in implementing certain protection mechanisms,
 * though a non-buggy app shouldn't end up calling them in
 * single-threaded mode.
 *
 * The rename is done as:
 * #define pthread_foo	__libc_foo
 * instead of
 * #define pthread_foo(x) __libc_foo((x))
 * in order that taking the address of the function ("func =
 * &pthread_foo;") continue to work.
 *
 * POSIX/SUSv3 requires that its functions exist as functions (even if
 * macro versions exist) and specifically that "#undef pthread_foo" is
 * legal and should not break anything. Code that does such will not
 * successfully get the stub behavior implemented here and will
 * require libpthread to be linked in.
 */

#ifndef __LIBPTHREAD_SOURCE__
__BEGIN_DECLS
int	__libc_mutex_init(pthread_mutex_t * __restrict, const pthread_mutexattr_t * __restrict);
int	__libc_mutex_lock(pthread_mutex_t *);
int	__libc_mutex_trylock(pthread_mutex_t *);
int	__libc_mutex_unlock(pthread_mutex_t *);
int	__libc_mutex_destroy(pthread_mutex_t *);

int	__libc_mutexattr_init(pthread_mutexattr_t *);
int	__libc_mutexattr_settype(pthread_mutexattr_t *, int);
int	__libc_mutexattr_destroy(pthread_mutexattr_t *);
__END_DECLS

#define	pthread_mutex_init		    __libc_mutex_init
#define	pthread_mutex_lock		    __libc_mutex_lock
#define	pthread_mutex_trylock		__libc_mutex_trylock
#define	pthread_mutex_unlock		__libc_mutex_unlock
#define	pthread_mutex_destroy		__libc_mutex_destroy

#define	pthread_mutexattr_init		__libc_mutexattr_init
#define	pthread_mutexattr_settype	__libc_mutexattr_settype
#define	pthread_mutexattr_destroy	__libc_mutexattr_destroy

__BEGIN_DECLS
int	__libc_cond_init(pthread_cond_t * __restrict,
	    const pthread_condattr_t * __restrict);
int	__libc_cond_signal(pthread_cond_t *);
int	__libc_cond_broadcast(pthread_cond_t *);
int	__libc_cond_wait(pthread_cond_t * __restrict,
	    pthread_mutex_t * __restrict);
int	__libc_cond_timedwait(pthread_cond_t * __restrict,
	    pthread_mutex_t * __restrict, const struct timespec * __restrict);
int	__libc_cond_destroy(pthread_cond_t *);
__END_DECLS

#define	pthread_cond_init	     	__libc_cond_init
#define	pthread_cond_signal		    __libc_cond_signal
#define	pthread_cond_broadcast		__libc_cond_broadcast
#define	pthread_cond_wait		    __libc_cond_wait
#define	pthread_cond_timedwait		__libc_cond_timedwait
#define	pthread_cond_destroy		__libc_cond_destroy

__BEGIN_DECLS
int	__libc_rwlock_init(pthread_rwlock_t * __restrict,
	    const pthread_rwlockattr_t * __restrict);
int	__libc_rwlock_rdlock(pthread_rwlock_t *);
int	__libc_rwlock_wrlock(pthread_rwlock_t *);
int	__libc_rwlock_tryrdlock(pthread_rwlock_t *);
int	__libc_rwlock_trywrlock(pthread_rwlock_t *);
int	__libc_rwlock_unlock(pthread_rwlock_t *);
int	__libc_rwlock_destroy(pthread_rwlock_t *);
__END_DECLS

#define	pthread_rwlock_init		    __libc_rwlock_init
#define	pthread_rwlock_rdlock		__libc_rwlock_rdlock
#define	pthread_rwlock_wrlock		__libc_rwlock_wrlock
#define	pthread_rwlock_tryrdlock	__libc_rwlock_tryrdlock
#define	pthread_rwlock_trywrlock	__libc_rwlock_trywrlock
#define	pthread_rwlock_unlock		__libc_rwlock_unlock
#define	pthread_rwlock_destroy		__libc_rwlock_destroy

__BEGIN_DECLS
int	__libc_thr_keycreate(pthread_key_t *, void (*)(void *));
int	__libc_thr_setspecific(pthread_key_t, const void *);
void	*__libc_thr_getspecific(pthread_key_t);
int	__libc_thr_keydelete(pthread_key_t);
__END_DECLS

#define	pthread_key_create		__libc_thr_keycreate
#define	pthread_setspecific		__libc_thr_setspecific
#define	pthread_getspecific		__libc_thr_getspecific
#define	pthread_key_delete		__libc_thr_keydelete

__BEGIN_DECLS
int	__libc_thr_once(pthread_once_t *, void (*)(void));
pthread_t	__libc_thr_self(void);
void	__libc_thr_exit(void *) __attribute__((__noreturn__));
int	__libc_thr_setcancelstate(int, int *);
int	__libc_thr_equal(pthread_t, pthread_t);
unsigned int	__libc_thr_curcpu(void);
__END_DECLS

#define	pthread_once			__libc_thr_once
#define	pthread_self			__libc_thr_self
#define	pthread_exit			__libc_thr_exit
#define	pthread_setcancelstate	__libc_thr_setcancelstate
#define pthread_equal			__libc_thr_equal
#define pthread_curcpu		    __libc_thr_curcpu

#endif /* __LIBPTHREAD_SOURCE__ */

#include <signal.h>

__BEGIN_DECLS
int	pthread_execve(const char *, char *const [], char *const []);
int pthread_kill(pthread_t, int);
int pthread_nanosleep(const struct timespec *, struct timespec *);
int	pthread_sigaction(int, const struct sigaction *, struct sigaction *);
int	pthread_sigmask(int, const sigset_t *, sigset_t *);
int	pthread_sigsuspend(const sigset_t *);
int	pthread_timedwait(const sigset_t * __restrict, siginfo_t * __restrict, const struct timespec * __restrict);
__END_DECLS

#endif /* _LIB_PTHREAD_H */

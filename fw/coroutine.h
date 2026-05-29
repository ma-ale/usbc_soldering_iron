/*
 * Copyright (c) 2026, Alessandro Mauri <alemauri001@gmail.com>.
 * All rights reserved.
 *
 * This file is a heavily modified, single-header derivative of the
 * protothreads library.
 *
 * =============================================================================
 * Original Protothreads License:
 * =============================================================================
 * Copyright (c) 2004-2005, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Adam Dunkels <adam@sics.se>
 */


#ifndef _COROUTINE_H
#define _COROUTINE_H

/* This is an implementation of stackless coroutines that uses the labels as
 * values [1] feature of GNU C, similar to protothreads [2] with the lc-addrlabels.h
 * include, the only difference being better ergonomics and minimal type checking.
 *
 * Since coroutines implemented this way need to return to wait for a condition
 * stack-local variables are NOT preserved between wait()s, as such these functions
 * have to be considered stackless. Any state that has to be preserved between
 * waits have to be declared static or have to be preserved outside the routine.
 *
 * This library's intended use is in microcontrollers and memory-constrained
 * systems where having a full rtos and/or having stackful coroutines is either
 * too heavy or overkill. Stackless coroutines implemented this way are an
 * alternative way of expressing state machines. Since coroutines have to be
 * repeatedly called to be "scheduled", the code maintains a main loop, but state
 * changes can be expressed with waits on conditions, timers, semaphores and other
 * more expressive constructs.
 *
 * A simple example program is:
 *
 * #include "coroutine.h"
 *
 * // Updates the timer when called, returns true when it expires
 * static int timer_expired(struct timer *t);
 * // Sets a timer in milliseconds
 * static void timer_set(struct timer *t, int interval);
 *
 * // First coroutine, waits for 1s and prints something
 * coroutine coro_1(coro_state_t *s)
 * {
 *   static struct timer t;
 *   coro_begin(s);
 *   for (;;) {
 *     timer_set(&t, 1000);
 *     coro_wait_until(s, timer_expired(&t));
 *     printf("coroutine 1\n");
 *   }
 *   coro_end();
 * }
 *
 * // Second coroutine, waits for 500ms and prints something
 * coroutine coro_2(coro_state_t *s)
 * {
 *   static struct timer t;
 *   coro_begin(s);
 *   for (;;) {
 *     timer_set(&t, 500);
 *     coro_wait_until(s, timer_expired(&t));
 *     printf("coroutine 2\n");
 *   }
 *   coro_end();
 * }
 *
 * int main(void)
 * {
 *   coro_state_t s1, s2;
 *   coro_init(&s1);
 *   coro_init(&s2);
 *
 *   // Main loop, calls each coroutine periodically
 *   for (;;) {
 *     coro_1(&s1);
 *     coro_2(&s2);
 *     usleep(100);
 *   }
 * }
 *
 * [1]: https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Labels-as-Values.html
 * [2]: https://dunkels.com/adam/pt/index.html
 */


#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
	// GCC: the extension is always present unless strict ANSI mode is forced.
	// (Even with -pedantic it still works, just warns.)
	#define HAVE_LABELS_AS_VALUES 1
#elif __has_extension(c_labels_as_values)
	#define HAVE_LABELS_AS_VALUES 1
#else
	#define HAVE_LABELS_AS_VALUES 0
#endif

_Static_assert(HAVE_LABELS_AS_VALUES, "This library requires the GNU C 'labels as values' extension");

// Useful macros
#define CORO_CONCAT2(s1, s2) s1##s2
#define CORO_CONCAT(s1, s2) CORO_CONCAT2(s1, s2)
#define CORO_CHECK_TYPE(var,type) { typedef void (*type_t)(type); type_t tmp = (type_t)0; if(0) tmp(var); }


// Return value of a coroutine, a coroutine should be scheduled (called) until
// it's return value becomes DONE
typedef enum {
	CORO_INIT = 0,
	CORO_CONT = CORO_INIT,
	CORO_DONE = 1,
	CORO_YIELDED = 2,
	CORO_WAITING = 3,
} coroutine;

// Local continuation of the coroutine, where it left off
typedef void* coro_state_t;


/* -------------------------- COROUTINE MANAGEMENT -------------------------- */

// Initialize the coroutine state
#define coro_init(s) CORO_CHECK_TYPE(s, coro_state_t*)                         \
	do { *s = NULL; } while (0)

// Jump to the previous state where the coroutine left off, __yield flag indicates
// wether the coroutine should yield this cycle
#define coro_begin(s) CORO_CHECK_TYPE(s, coro_state_t*)                        \
	unsigned char __yield = 0;                                                 \
	do { if(*s != NULL) goto *(*s); } while (0)

#define coro_end()

// Set a jump with a gcc label as value feature, this is a GNU C extension
#define CORO_SET_JUMP(s) CORO_CHECK_TYPE(s, coro_state_t*)                     \
	do {                                                                       \
		CORO_CONCAT(LC_LABEL, __LINE__):                                       \
		*s = &&CORO_CONCAT(LC_LABEL, __LINE__);                                \
	} while (0)

// Yield from a coroutine
#define coro_yield(s) CORO_CHECK_TYPE(s, coro_state_t*)                        \
	do {                                                                       \
		__yield = 1;                                                           \
		CORO_SET_JUMP(s);                                                      \
		if (__yield == 1) return CORO_YIELDED;                                 \
	} while (0)

// Wait until a condition is met (becomes true)
#define coro_wait_until(s, cond) CORO_CHECK_TYPE(s, coro_state_t*)             \
	do {                                                                       \
		CORO_SET_JUMP(s);                                                      \
		if (!(cond)) return CORO_WAITING;                                      \
	} while (0)

// Shorthand for wait_until
#define coro_wait(s, cond) coro_wait_until(s, cond)

// Wait while a condition remains true
#define coro_wait_while(s, cond) coro_wait_until(s, !(cond))

// Exit from the coroutine, resets the state
#define coro_exit(s) CORO_CHECK_TYPE(s, coro_state_t*)                         \
	do {                                                                       \
		coro_init(s);                                                          \
		return CORO_DONE;                                                      \
	} while (0)

// Restart a coroutine, when called it will not resume from where it left off,
// instead it will start from the beginning, the only difference with exit is
// the return value. To determine wether a coroutine has exited or restarted
// is to check it's return value
#define coro_restart(s) CORO_CHECK_TYPE(s, coro_state_t*)                      \
	do {                                                                       \
		coro_init(s);                                                          \
		return CORO_INIT;                                                      \
	} while(0)


/* ------------------------- SEMAPHORE IMPLEMENTATION ----------------------- */
// Implements a basic semaphore type with the classic wait and signal operations
// semaphores are not bound checked

typedef int coro_semaphore_t;

// Initializes a semaphore with a starting value of "count"
#define coro_sem_init(sem, count) CORO_CHECK_TYPE(sem, coro_semaphore_t*)      \
	do {                                                                       \
		*sem = (int)count;                                                     \
	} while (0)

// Wait until a semaphore has a value other than zero
#define coro_sem_wait(state, sem)                                              \
	CORO_CHECK_TYPE(state, coro_state_t*)                                      \
	CORO_CHECK_TYPE(sem, coro_semaphore_t*)                                    \
	do {                                                                       \
		coro_wait_until(state, *(sem) > 0);                                    \
		--(*sem);                                                              \
	} while (0)

// Signal a semaphore
#define coro_sem_signal(state, sem)                                            \
	CORO_CHECK_TYPE(state, coro_state_t*)                                      \
	CORO_CHECK_TYPE(sem, coro_semaphore_t*)                                    \
	++(*sem)


#endif

/* General queue data structure for GDB, the GNU debugger.

   Copyright (C) 2012 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* These macros implement functions and structs for a general queue.  Macro
   'DEFINE_QUEUE_P(TYPE)' is to define the new queue type for 'TYPE', and
   macro 'DECLARE_QUEUE_P' is to declare these external APIs.  */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

#include "libiberty.h" /* xmalloc */
#include "gdb_assert.h"

/* Define a new queue implementation.  */

#define QUEUE(TYPE) struct queue_ ## TYPE
#define QUEUE_ELEM(TYPE) struct queue_elem_ ## TYPE
#define QUEUE_ITER(TYPE) struct queue_iter_ ## TYPE
#define QUEUE_ITER_FUNC(TYPE) queue_ ## TYPE ## _operate_func

#define DEFINE_QUEUE_P(TYPE)			\
QUEUE_ELEM (TYPE)				\
{						\
  QUEUE_ELEM (TYPE) *next;			\
						\
  TYPE data;					\
};						\
						\
/* Queue iterator.  */				\
QUEUE_ITER (TYPE)				\
{						\
  /* The current element during traverse.  */	\
  QUEUE_ELEM (TYPE) *p;			\
  /* The previous element of P.  */		\
  QUEUE_ELEM (TYPE) *prev;			\
};						\
						\
QUEUE(TYPE)					\
{						\
  QUEUE_ELEM (TYPE) *head;			\
  QUEUE_ELEM (TYPE) *tail;			\
  void (*free_func) (TYPE);			\
};						\
						\
/* Typical enqueue operation.  Put V into queue Q.  */			\
									\
void									\
queue_ ## TYPE ## _enque (QUEUE (TYPE) *q, TYPE v)			\
{									\
  QUEUE_ELEM (TYPE) *p							\
    = xmalloc (sizeof (QUEUE_ELEM (TYPE)));				\
									\
  gdb_assert (q != NULL);						\
  p->data = v;								\
  p->next = NULL;							\
  if (q->tail == NULL)							\
    {									\
      q->tail = p;							\
      q->head = p;							\
    }									\
  else									\
    {									\
      q->tail->next = p;						\
      q->tail = p;							\
    }									\
}									\
									\
/* Typical dequeue operation.  Return one element queue Q.  Assert	\
   fail if Q is NULL or Q is empty.  */				\
									\
TYPE									\
queue_ ## TYPE ## _deque (QUEUE (TYPE) *q)				\
{									\
  QUEUE_ELEM (TYPE) *p;						\
  TYPE v;								\
									\
  gdb_assert (q != NULL);						\
  p = q->head;								\
  gdb_assert (p != NULL);						\
									\
  if (q->head == q->tail)						\
    {									\
      q->head = NULL;							\
      q->tail = NULL;							\
    }									\
  else									\
    q->head = q->head->next;						\
									\
  v = p->data;								\
									\
  xfree (p);								\
  return v;								\
}									\
									\
/* Return the element on head, but don't remove it from queue.  Assert	\
   fail is Q is NULL or Q is empty.  */				\
									\
TYPE									\
queue_ ## TYPE ## _peek (QUEUE (TYPE) *q)				\
{									\
  gdb_assert (q != NULL);						\
  gdb_assert (q->head != NULL);					\
  return q->head->data;						\
}									\
									\
/* Return true if queue Q is empty.  */				\
									\
int									\
queue_ ## TYPE ## _is_empty (QUEUE (TYPE) *q)				\
{									\
  gdb_assert (q != NULL);						\
  return q->head == NULL;						\
}									\
									\
/* Remove the element per the state of iterator ITER from queue Q.  */	\
									\
void									\
queue_ ## TYPE ## _remove_elem (QUEUE (TYPE) *q,			\
				QUEUE_ITER (TYPE) *iter)		\
{									\
 gdb_assert (q != NULL);						\
 gdb_assert (iter != NULL && iter->p != NULL);				\
									\
  if (iter->p == q->head || iter->p == q->tail)			\
    {									\
      if (iter->p == q->head)						\
	q->head = iter->p->next;					\
      if (iter->p == q->tail)						\
	q->tail = iter->prev;						\
    }									\
  else									\
    iter->prev->next = iter->p->next;					\
									\
  xfree (iter->p);							\
  /* Indicate that ITER->p has been deleted from QUEUE q.  */		\
  iter->p = NULL;							\
}									\
									\
/* Iterate over elements in the queue Q and call function OPERATE on	\
   each element.  OPERATE would return false to terminate the		\
   iteration and true to continue the iteration.  Return false if	\
   iteration is terminated by function OPERATE, otherwise return	\
   true.  */								\
									\
int									\
queue_ ## TYPE ## _iterate (QUEUE (TYPE) *q,				\
			    QUEUE_ITER_FUNC (TYPE) operate,		\
			    void *data)				\
{									\
  QUEUE_ELEM (TYPE) *next = NULL;					\
  QUEUE_ITER (TYPE) iter = { NULL, NULL };				\
									\
  gdb_assert (q != NULL);						\
									\
  for (iter.p = q->head; iter.p != NULL; iter.p = next)		\
    {									\
      next = iter.p->next;						\
      if (!operate (q, &iter, iter.p->data, data))			\
	return 0;							\
      /* ITER.P was not deleted by function OPERATE.  */		\
      if (iter.p != NULL)						\
	iter.prev = iter.p;						\
    }									\
  return 1;								\
}									\
									\
/* Allocate memory for queue.  */					\
									\
QUEUE (TYPE) *								\
queue_ ## TYPE ## _alloc (void (*free_func) (TYPE))			\
{									\
  QUEUE (TYPE) *q;							\
									\
  q = (QUEUE (TYPE) *) xmalloc (sizeof (QUEUE (TYPE)));		\
  q->head = NULL;							\
  q->tail = NULL;							\
  q->free_func = free_func;						\
  return q;								\
}									\
									\
/* Length of queue Q.  */						\
									\
int									\
queue_ ## TYPE ## _length (QUEUE (TYPE) *q)				\
{									\
  QUEUE_ELEM (TYPE) *p;						\
  int len = 0;								\
									\
  gdb_assert (q != NULL);						\
									\
  for (p = q->head; p != NULL; p = p->next)				\
    len++;								\
									\
  return len;								\
}									\
									\
/* Free memory for queue Q.  */					\
									\
void									\
queue_ ## TYPE ## _free (QUEUE (TYPE) *q)				\
{									\
  QUEUE_ELEM (TYPE) *p, *next;						\
									\
  gdb_assert (q != NULL);						\
									\
  for (p = q->head; p != NULL; p = next)				\
    {									\
      next = p->next;							\
      if (q->free_func)						\
	q->free_func (p->data);					\
      xfree (p);							\
    }									\
  xfree (q);								\
}									\

/* External declarations for these functions.  */
#define DECLARE_QUEUE_P(TYPE)					\
QUEUE (TYPE);							\
QUEUE_ELEM (TYPE);						\
QUEUE_ITER (TYPE);						\
extern void							\
  queue_ ## TYPE ## _enque (QUEUE (TYPE) *q, TYPE v);		\
extern TYPE							\
  queue_ ## TYPE ## _deque (QUEUE (TYPE) *q);			\
extern int queue_ ## TYPE ## _is_empty (QUEUE (TYPE) *q);	\
extern QUEUE (TYPE) *						\
  queue_ ## TYPE ## _alloc (void (*free_func) (TYPE));		\
extern int queue_ ## TYPE ## _length (QUEUE (TYPE) *q);	\
extern TYPE							\
  queue_ ## TYPE ## _peek (QUEUE (TYPE) *q);			\
extern void queue_ ## TYPE ## _free (QUEUE (TYPE) *q);		\
typedef int QUEUE_ITER_FUNC(TYPE) (QUEUE (TYPE) *,		\
				   QUEUE_ITER (TYPE) *,	\
				   TYPE,			\
				   void *);			\
extern int							\
  queue_ ## TYPE ## _iterate (QUEUE (TYPE) *q,			\
			      QUEUE_ITER_FUNC (TYPE) operate,	\
			      void *);				\
extern void							\
  queue_ ## TYPE ## _remove_elem (QUEUE (TYPE) *q,		\
				  QUEUE_ITER (TYPE) *iter);	\

#define QUEUE_enque(TYPE, QUEUE, V) queue_ ## TYPE ## _enque ((QUEUE), (V))
#define QUEUE_deque(TYPE, QUEUE) queue_ ## TYPE ## _deque (QUEUE)
#define QUEUE_peek(TYPE, QUEUE) queue_ ## TYPE ## _peek (QUEUE)
#define QUEUE_is_empty(TYPE, QUEUE) queue_ ## TYPE ## _is_empty (QUEUE)
#define QUEUE_alloc(TYPE, FREE_FUNC) queue_ ## TYPE ## _alloc (FREE_FUNC)
#define QUEUE_length(TYPE, QUEUE) queue_ ## TYPE ## _length (QUEUE)
#define QUEUE_free(TYPE, QUEUE) queue_ ## TYPE ## _free (QUEUE)
#define QUEUE_iterate(TYPE, QUEUE, OPERATE, PARAM)	\
  queue_ ## TYPE ## _iterate ((QUEUE), (OPERATE), (PARAM))
#define QUEUE_remove_elem(TYPE, QUEUE, ITER) \
  queue_ ## TYPE ## _remove_elem ((QUEUE), (ITER))

#endif /* QUEUE_H */

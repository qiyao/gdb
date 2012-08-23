/* Notification to GDB.
   Copyright (C) 1989, 1993-1995, 1997-2000, 2002-2012 Free Software
   Foundation, Inc.

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

#include "notif.h"

static void
notif_reply_test (struct notif_reply *reply, char *own_buf)
{
  strcpy (own_buf, "CESHI");
}

struct notif notif_test =
{
  "vTested", "NTest", NOTIF_TEST, NULL, notif_reply_test
};

static struct notif *notif_packets [] =
{
  &notif_stop,
  &notif_test,
  NULL,
};

DEFINE_QUEUE_P(notif_p);

QUEUE(notif_p) *notif_queue;

DEFINE_QUEUE_P(notif_reply_p);

/* Push another reply, or if there are no more left, an OK.  */

void
notif_send_reply (struct notif *notif, char *own_buf)
{
  if (!QUEUE_is_empty (notif_reply_p, notif->queue))
    {
      struct notif_reply *reply = QUEUE_peek (notif_reply_p, notif->queue);

      notif->reply (reply, own_buf);
    }
  else
    write_ok (own_buf);
}

/* Handle the ack in buffer OWN_BUF.  Return 1 if the ack is handled, and
   return 0 if the contents in OWN_BUF is not a ack.  */

int
handle_notif_ack (char *own_buf)
{
  int i = 0;
  struct notif *np = NULL;

  for (i = 0; notif_packets[i] != NULL; i++)
    {
      np = notif_packets[i];
      if (strncmp (own_buf, np->name, strlen (np->name)) == 0)
	break;
    }

  if (np == NULL)
    return 0;

  /* If we're waiting for GDB to acknowledge a pending reply,
     consider that done.  */
  if (!QUEUE_is_empty (notif_reply_p, np->queue))
    {
      struct notif_reply *head = QUEUE_deque (notif_reply_p, np->queue);

      if (remote_debug)
	fprintf (stderr, "%s: acking %d\n", np->name,
		 QUEUE_length (notif_reply_p, np->queue));

      xfree (head);
    }

  notif_send_reply (np, own_buf);

  return 1;
}

void
notif_reply_enque (struct notif *notif, struct notif_reply *reply)
{
  QUEUE_enque (notif_reply_p, notif->queue, reply);

  if (remote_debug)
    fprintf (stderr, "pending replies: %s %d\n", notif->notif_name,
	     QUEUE_length (notif_reply_p, notif->queue));

}

/* Process one notification.  */

void
notif_process (struct notif *np, char *buf,
	       struct target_waitstatus status, ptid_t ptid)
{
  struct notif_reply *new_notif = NULL;
  int is_first_event = 0;

  switch (np->type)
    {
    case NOTIF_STOP:
      {
	struct vstop_notif *vstop_notif
	  = xmalloc (sizeof (struct vstop_notif));

	vstop_notif->status = status;
	new_notif = (struct notif_reply *) vstop_notif;
      }
      break;
    case NOTIF_TEST:
      new_notif = xmalloc (sizeof (struct notif_reply));
      break;
    default:
      error ("Unknown notification type");
    }

  new_notif->ptid = ptid;
  is_first_event = QUEUE_is_empty (notif_reply_p, np->queue);

  /* Something interesting.  Tell GDB about it.  */
  notif_reply_enque (np, new_notif);

  /* If this is the first stop reply in the queue, then inform GDB
     about it, by sending a corresponding notification.  */
  if (is_first_event)
    {
      char *p = buf;

      xsnprintf (p, PBUFSIZ, "%s:", np->notif_name);
      p += strlen (p);

      np->reply (new_notif, p);
      putpkt_notif (buf);
    }
}

/* Return true if the queues of all notifications are empty.  If the queue of
   one of notification is not empty, return false.  */

int
notif_queued_replies_empty_p (void)
{
  int i;

  for (i = 0; notif_packets[i] != NULL; i++)
    if (!QUEUE_is_empty (notif_reply_p, notif_packets[i]->queue))
      return 0;

  return 1;
}

static int
always_remove_on_match_pid (QUEUE (notif_reply_p) *q,
			    QUEUE_ITER (notif_reply_p) *iter,
			    struct notif_reply *reply,
			    void *data)
{
  int *pid = data;

  if (*pid == -1 || ptid_get_pid (reply->ptid) == *pid)
    {
      if (q->free_func != NULL)
	q->free_func (reply);

      QUEUE_remove_elem (notif_reply_p, q, iter);
    }

  return 1;
}

/* Get rid of the currently pending replies of NOTIF for PID.  If PID is
   -1, then apply to all processes.  */

void
notif_queued_replies_discard (int pid, struct notif *notif)
{
  QUEUE_iterate (notif_reply_p, notif->queue,
		 always_remove_on_match_pid, &pid);
}

/* Get rid of pending replies of all notifications.  */

void
notif_queued_replies_discard_all (int pid)
{
  int i;

  for (i = 0; notif_packets[i] != NULL; i++)
    notif_queued_replies_discard (pid, notif_packets[i]);
}

static void
notif_xfree (struct notif *notif)
{
  if (notif->queue != NULL)
    QUEUE_free (notif_reply_p, notif->queue);

  xfree (notif);
}

void
initialize_notif (void)
{
  int i = 0;

  for (i = 0; notif_packets[i] != NULL; i++)
    notif_packets[i]->queue
      = QUEUE_alloc (notif_reply_p,
		     (void (*)(struct notif_reply *)) xfree);

  notif_queue = QUEUE_alloc (notif_p, notif_xfree);
}

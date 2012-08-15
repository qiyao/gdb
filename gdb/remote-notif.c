/* Remote notification in GDB protocol

   Copyright (C) 1988-2012 Free Software Foundation, Inc.

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

#include "defs.h"
#include "remote.h"
#include "remote-notif.h"
#include "event-loop.h"
#include "target.h"
#include "inferior.h"

#include <string.h>

int notif_debug = 0;

/* Supported notifications.  */

static struct notif *notifs[] =
{
  (struct notif *) &notif_packet_stop,
};

static void do_notif_reply_xfree (void *arg);

/* Parse the BUF for the expected notification NP, and send packet to
   acknowledge.  */

void
remote_notif_ack (struct notif *np, char *buf)
{
  struct notif_reply *reply = np->alloc_reply ();
  struct cleanup *old_chain = make_cleanup (do_notif_reply_xfree, reply);

  DEBUG_NOTIF ("ack '%s'", np->ack_command);

  np->parse (np, buf, reply);
  np->ack (np, buf, reply);

  discard_cleanups (old_chain);
}

/* Parse the BUF for the expected notification NP.  */

struct notif_reply *
remote_notif_parse (struct notif *np, char *buf)
{
  struct notif_reply *reply = np->alloc_reply ();
  struct cleanup *old_chain = make_cleanup (do_notif_reply_xfree, reply);

  DEBUG_NOTIF ("parse '%s'", np->name);

  np->parse (np, buf, reply);

  discard_cleanups (old_chain);
  return reply;
}

/* Destructor of 'notif_queued' SELF.  */

void
remote_notif_queued_dtr (struct notif *self)
{
  struct notif_queued *notif = (struct notif_queued *) self;

  if (notif->ack_queue != NULL)
    QUEUE_free (notif_reply_p, notif->ack_queue);
}

DEFINE_QUEUE_P(notif_p);

QUEUE(notif_p) *notif_queue;

/* Process notifications one by one until a Stop notification is found.
   EXCEPT is not expected in the queue.  */

void
remote_notif_process (struct notif *except)
{
  while (!QUEUE_is_empty (notif_p, notif_queue))
    {
      struct notif *np = QUEUE_deque (notif_p, notif_queue);

      gdb_assert (np != except);

      if (np == (struct notif *) &notif_packet_stop)
	{
	  /* If we get a Stop notification, handle it in
	     remote_wait_ns.  */
	  mark_async_event_handler (remote_async_inferior_event_token);
	  break;
	}

      remote_notif_pending_replies (np);
    }
}

static void
remote_async_get_pending_events_handler (gdb_client_data data)
{
  remote_notif_process (NULL);
}

/* Asynchronous signal handle registered as event loop source for when
   the remote sent us a notification.  The registered callback
   will do a ACK sequence to pull the rest of the events out of
   the remote side into our event queue.  */

static struct async_event_handler *remote_async_get_pending_events_token;

/* Register async_event_handler for notification.  */

void
remote_notif_register_async_event_handler (void)
{
  remote_async_get_pending_events_token
    = create_async_event_handler (remote_async_get_pending_events_handler,
				  NULL);
}

/* Unregister async_event_handler for notification.  */

void
remote_notif_unregister_async_event_handler (void)
{
  if (remote_async_get_pending_events_token)
    delete_async_event_handler (&remote_async_get_pending_events_token);
}

/* Remote notification handler.  */

void
handle_notification (char *buf)
{
  struct notif *np = NULL;
  int i;

  for (i = 0; i < sizeof (notifs) / sizeof (notifs[0]); i++)
    {
      np = notifs[i];
      if (strncmp (buf, np->name, strlen (np->name)) == 0
	  && buf[strlen (np->name)] == ':')
	break;
    }

  /* We ignore notifications we don't recognize, for compatibility
     with newer stubs.  */
  if (np == NULL)
    return;

  if (np->pending_reply)
    {
      /* We've already parsed the in-flight reply, but the stub for some
	 reason thought we didn't, possibly due to timeout on its side.
	 Just ignore it.  */
      DEBUG_NOTIF ("ignoring resent notification");
    }
  else
    {
      struct notif_reply *reply
	= remote_notif_parse (np, buf+ strlen (np->name) + 1);

      /* Be careful to only set it after parsing, since an error
	 may be thrown then.  */
      np->pending_reply = reply;

      /* Notify the event loop there's a stop reply to acknowledge
	 and that there may be more events to fetch.  */
      QUEUE_enque (notif_p, notif_queue, np);
      mark_async_event_handler (remote_async_get_pending_events_token);

      DEBUG_NOTIF ("Notification '%s' captured", np->name);
    }
}

/* Free REPLY.  */

void
notif_reply_xfree (struct notif_reply *reply)
{
  if (reply && reply->dtr)
    reply->dtr (reply);

  xfree (reply);
}

/* Cleanup wrapper.  */

static void
do_notif_reply_xfree (void *arg)
{
  struct notif_reply *reply = arg;

  notif_reply_xfree (reply);
}

DEFINE_QUEUE_P(notif_reply_p);

/* A parameter to pass data in and out.  */

struct queue_iter_param
{
  void *input;
  struct notif_reply *output;
};

static int
remote_notif_remove_once_on_match (QUEUE (notif_reply_p) *q,
				   QUEUE_ITER (notif_reply_p) *iter,
				   notif_reply_p reply,
				   void *data)
{
  struct queue_iter_param *param = data;
  ptid_t *ptid = param->input;

  if (ptid_match (reply->ptid, *ptid))
    {
      param->output = reply;
      QUEUE_remove_elem (notif_reply_p, q, iter);
      return 0;
    }

  return 1;
}

/* Remove the first reply in NP->ack_queue if function MATCH returns true.  */

struct notif_reply *
remote_notif_discard_queued_reply (struct notif_queued *np, ptid_t ptid)
{
  struct queue_iter_param param;

  param.input = &ptid;
  param.output = NULL;

  QUEUE_iterate (notif_reply_p, np->ack_queue,
		 remote_notif_remove_once_on_match, &param);

  DEBUG_NOTIF ("discard queued reply: '%s' in %s", np->base.name,
	       target_pid_to_str (ptid));

  return param.output;
}

static int
remote_notif_remove_always (QUEUE (notif_reply_p) *q,
			    QUEUE_ITER (notif_reply_p) *iter,
			    notif_reply_p reply,
			    void *data)
{
  struct queue_iter_param *param = data;
  int *pid = (int *) param->input;

  if (*pid == -1 || ptid_get_pid (reply->ptid) == *pid)
    {
      notif_reply_xfree (reply);
      QUEUE_remove_elem (notif_reply_p, q, iter);
    }

  return 1;
}

/* Discard all pending replies of inferior PID.  If PID is -1,
   discard everything.  */

void
remote_notif_discard_replies (int pid)
{
  int i;
  struct queue_iter_param param;

  for (i = 0; i < sizeof (notifs) / sizeof (notifs[0]); i++)
    {
      struct notif *np = notifs[i];

      /* Discard the in-flight notification.  */
      if (np->pending_reply != NULL
	  && (pid == -1 || ptid_get_pid (np->pending_reply->ptid) == pid))
	{
	  notif_reply_xfree (np->pending_reply);
	  np->pending_reply = NULL;
	}

      DEBUG_NOTIF ("discard all replies: '%s' in pid %d", np->name, pid);
    }

  param.input = &pid;
  param.output = NULL;
  /* Discard the replies we have already pulled with ack.  */
  QUEUE_iterate (notif_reply_p, notif_packet_stop.ack_queue,
		 remote_notif_remove_always, &param);
}

static void
notif_xfree (struct notif *notif)
{
  if (notif->pending_reply != NULL)
    notif_reply_xfree (notif->pending_reply);

  if (notif->dtr != NULL)
    notif->dtr (notif);

  xfree (notif);
}

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_notif;

void
_initialize_notif (void)
{
  notif_packet_stop.ack_queue = QUEUE_alloc (notif_reply_p, notif_reply_xfree);

  notif_queue = QUEUE_alloc (notif_p, notif_xfree);
}

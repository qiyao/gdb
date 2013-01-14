/* Async remote notification on trace.

   Copyright (C) 2013 Free Software Foundation, Inc.

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
#include <string.h>
#include "remote.h"
#include "tracepoint.h"
#include "remote-notif.h"
#include "observer.h"

static void
remote_notif_trace_status_parse (struct notif_client *self, char *buf,
				 struct notif_event *event)
{
  struct trace_status *ts = current_trace_status ();

  gdb_assert (buf[0] == 'T');
  parse_trace_status (buf + 1, ts);

  /* When the tracing is stopped, there is no changes anymore in
     the trace, so the remote stub can't send another notification.
     We don't have to worry about notifying 'trace_changed' observer
     with argument 1 twice.
     The remote stub can't request tracing start and the remote stub
     may send multiple trace notifications on various status changes,
     we don't notify 'trace_changed' observer with argument 0.  */
  if (!ts->running)
    observer_notify_trace_changed (0);
}

static void
remote_notif_trace_ack (struct notif_client *self, char *buf,
			struct notif_event *event)
{
  /* acknowledge */
  putpkt ((char *) self->base.ack_name);
}

static int
remote_notif_trace_can_get_pending_events (struct notif_client *self)
{
  return 1;
}

static struct notif_event *
remote_notif_trace_alloc_event (void)
{
  struct notif_event *event = xmalloc (sizeof (struct notif_event));

  event->dtr = NULL;

  return event;
}

static struct notif_annex notif_client_trace_annex[] =
{
  { "status", -1, remote_notif_trace_status_parse, },
  { NULL, -1, NULL, },
};

/* A client of notification 'Trace'.  */

struct notif_client notif_client_trace =
{
  { "Trace", "vTraced", notif_client_trace_annex, },
  remote_notif_trace_ack,
  remote_notif_trace_can_get_pending_events,
  remote_notif_trace_alloc_event,
  NULL,
};

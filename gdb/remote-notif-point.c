/* Async remote notification on the creation, deletion, and
   modification of various types of breakpoint.

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

#include "defs.h"
#include <string.h>
#include "remote.h"
#include "remote-notif.h"
#include "tracepoint.h"

enum NOTIF_POINT_TYPE { POINT_MODIFIED, POINT_CREATED, };

struct notif_point_event
{
  struct notif_event base;
  enum NOTIF_POINT_TYPE type;
  union
  {
    struct uploaded_tp *utp;
  } u;
};

static void
remote_notif_point_parse (struct notif_client *self, char *buf,
			  struct notif_event *event)
{
  struct notif_point_event *pevent
    = (struct notif_point_event *) event;

  if (strncmp (buf, "modified:", 9) == 0)
    {
      char *p = buf + 9;
      struct uploaded_tp *utp = NULL;

      parse_tracepoint_definition (p, &utp);

      gdb_assert (utp != NULL);
      gdb_assert (utp->next == NULL);

      pevent->type = POINT_MODIFIED;
      pevent->u.utp = utp;
    }
  else if (strncmp (buf, "created:", 8) == 0)
    {
      pevent->type = POINT_CREATED;
    }
  else
    error (_("Unknown trace notification."));
}

static void
remote_notif_point_ack (struct notif_client *self, char *buf,
			struct notif_event *event)
{
  struct notif_point_event *pevent
    = (struct notif_point_event *) event;

  /* It is safe to send packets to the remote stub to get its state
     here.  */
  switch (pevent->type)
    {
    case POINT_MODIFIED:
      {
	struct bp_location *loc
	  = find_matching_tracepoint_location (pevent->u.utp);

	target_get_tracepoint_status (NULL, pevent->u.utp);
	/* TODO: extract the changed data from PEVENT->u.utp to
	   LOC->owner.  */
	xfree (pevent->u.utp);
	/* The tracepoint (location) should exist in GDB side.  */
	gdb_assert (loc != NULL);
      }
      break;
    case POINT_CREATED:
      {
	/* Start the sequence to query about auto-load
	   breakpoints.  */
      }
      break;
    }

  /* acknowledge */
  putpkt ((char *) self->ack_command);
}

static int
remote_notif_point_can_get_pending_events (struct notif_client *self)
{
  return 1;
}

static struct notif_event *
remote_notif_point_alloc_event (void)
{
  struct notif_point_event *event
    = xmalloc (sizeof (struct notif_point_event));

  event->base.dtr = NULL;

  return (struct notif_event *) event;
}

/* A client of notification 'Point'.  */

struct notif_client notif_client_point =
{
  "Point", "vPointed",
  remote_notif_point_parse,
  remote_notif_point_ack,
  remote_notif_point_can_get_pending_events,
  remote_notif_point_alloc_event,
  NULL,
};

/* Notification to GDB.
   Copyright (C) 1989-2013 Free Software Foundation, Inc.

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

#include "ptid.h"
#include "server.h"
#include "target.h"
#include "queue.h"
#include "notif-base.h"

/* Structure holding information related to a single event.  We
   keep a queue of these to push to GDB.  It can be extended if
   the event of given notification contains more information.  */

typedef struct notif_event
{
  /* C requires that a struct or union has at least one member.  */
  char dummy;
} *notif_event_p;

DECLARE_QUEUE_P (notif_event_p);

/* An event of a notification which has an annex.  */

struct notif_annex_event
{
  struct notif_event base;
  /* The index of the annex in field 'annexes' in 'notif_server'.  */
  int annex_index;
};

/* A type notification to GDB.  An object of 'struct notif_server'
   represents a type of notification.  */

typedef struct notif_server
{
  struct notif_base base;

  /* A queue of events to GDB.  A new notif_event can be enque'ed
     into QUEUE at any appropriate time, and the notif_reply is
     deque'ed only when the ack from GDB arrives.  */
  QUEUE (notif_event_p) *queue;
} *notif_server_p;

extern struct notif_server notif_stop;

int handle_notif_ack (char *own_buf, int packet_len);
void notif_write_event (struct notif_server *notif, char *own_buf);
char* notif_qsupported_reply (void);
void notif_qsupported_record (char *gdb_notifications);

void notif_push (struct notif_server *np, struct notif_event *event);
void notif_event_enque (struct notif_server *notif,
			struct notif_event *event);

void initialize_notif (void);

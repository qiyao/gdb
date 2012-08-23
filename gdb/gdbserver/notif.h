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

#include "ptid.h"
#include "server.h"
#include "target.h"
#include "queue.h"

/* Structure holding information relative to a single reply.  We
   keep a queue of these (really a singly-linked list) to push to GDB
   in non-stop mode.  */

typedef struct notif_reply
{
  /* Thread or process that got the event.  */
  ptid_t ptid;
} *notif_reply_p;

/* A sub-class of 'struct ack_notif' for stop reply.  */

struct vstop_notif
{
  struct notif_reply base;

  /* Event info.  */
  struct target_waitstatus status;
};

enum notif_type { NOTIF_STOP, NOTIF_TEST };

/* A notification to GDB.  */

typedef struct notif
{
  /* The name of ack packet, for example, 'vStopped'.  */
  const char *name;

  /* The notification packet, for example, '%Stop'.  Note that '%' is not in
     'notif_name'.  */
  const char *notif_name;

  const enum notif_type type;

  /* A queue of reply to GDB.  */
  QUEUE (notif_reply_p) *queue;

  /* Reply to GDB.  */
  void (*reply) (struct notif_reply *reply, char *own_buf);
} *notif_p;

extern struct notif notif_stop;

int handle_notif_ack (char *own_buf);
void notif_send_reply (struct notif *notif, char *own_buf);

int notif_queued_replies_empty_p (void);
void notif_queued_replies_discard (int pid, struct notif *notif);
void notif_queued_replies_discard_all (int pid);

void notif_process (struct notif *np, char *buf,
		    struct target_waitstatus status, ptid_t ptid);
void notif_reply_enque (struct notif *notif, struct notif_reply *reply);

extern struct notif notif_test;
extern QUEUE (notif_p) *notif_queue;

DECLARE_QUEUE_P (notif_p);
DECLARE_QUEUE_P (notif_reply_p);

void initialize_notif (void);

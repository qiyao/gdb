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

#ifndef REMOTE_NOTIF_H
#define REMOTE_NOTIF_H
#include "queue.h"

/* A reply for a remote notification.  */

typedef struct notif_reply
{
  ptid_t ptid;
  /* Destructor.  Release everything from SELF, but not SELF itself.  */
  void (*dtr) (struct notif_reply *self);
} *notif_reply_p;

DECLARE_QUEUE_P (notif_reply_p);

/* Data and operations related to async notification.  */

typedef struct notif
{
  /* The name of notification packet.  */
  const char *name;

  /* The packet to acknowledge a previous reply.  */
  const char *ack_command;

  /* Parse BUF to get the expected reply.  This function may throw
     exception if contents in BUF is not the expected reply.  */
  void (*parse) (struct notif *self, char *buf, void *data);

  /* Send field <ack_command> to remote, and do some checking.  If
     something wrong, throw exception.  */
  void (*ack) (struct notif *self, char *buf, void *data);

  /* Allocate a reply.  */
  struct notif_reply *(*alloc_reply) (void);

  /* Release memory for sub-class.  */
  void (*dtr) (struct notif *self);

  /* One pending reply.  This is where we keep it until it is
     acknowledged.  When there is a notification packet, parse it,
     and create an object of 'struct notif_reply' to assign to
     it.  This field is unchanged until GDB starts to ack this
     notification (which is done by
     remote.c:remote_notif_pending_replies).  */
  struct notif_reply *pending_reply;
} *notif_p;

/* A stop notification.  */

struct notif_stop
{
  struct notif base;
  /* The list of already fetched and acknowledged events.  */
  QUEUE (notif_reply_p) *ack_queue;
};

DECLARE_QUEUE_P (notif_p);

void remote_notif_ack (struct notif *np, char *buf);
struct notif_reply *remote_notif_parse (struct notif *np, char *buf);

void handle_notification (char *buf);

void remote_notif_register_async_event_handler (void);
void remote_notif_unregister_async_event_handler (void);

void notif_reply_xfree (struct notif_reply *reply);

void remote_notif_discard_replies (int pid);
void remote_notif_process (struct notif *except);

extern struct notif_stop notif_packet_stop;

extern unsigned int notif_debug;

#define DEBUG_NOTIF(fmt, args...)	\
  if (notif_debug)			\
    fprintf_unfiltered (gdb_stdlog, "notif: " fmt "\n", ##args);

#endif /* REMOTE_NOTIF_H */

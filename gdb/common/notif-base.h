/* Shared structs of asynchronous remote notification.

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
#ifndef NOTIF_BASE_H
#define NOTIF_BASE_H 1

struct notif_event;
#ifndef GDBSERVER
struct notif_client;
#endif

/* An annex of a notification.  A notification may or may not have
   annexes.  */

struct notif_annex
{
  /* Name of this annex.  If it is NULL, the notification doesn't have
     annex, and the field PARSE is about the notification this annex
     belongs to.  For example, notification event "N1:event" doesn't
     have an annex in the packet, but the notification object still has
     one instance of 'notif_annex', and its field <name> is NULL.  */
  const char *name;

#ifdef GDBSERVER
  /* This annex is supported by GDB or not.  A notification may have
     multiple annexes and some of them are supported.  Annex is the
     smallest unit of support.  */
  int supported;

  /* Write event EVENT to OWN_BUF.  */
  void (*write) (struct notif_event *event, char *own_buf);
#else
  /* The id of the annex.  In GDB, we use this field as an index to
     access the state of this annex.  */
  int id;

  /* Parse BUF to get the expected event and update EVENT.  This
     function may throw exception if contents in BUF is not the
     expected event.  */
  void (*parse) (struct notif_client *self, char *buf,
		 struct notif_event *event);
#endif
};

/* Iterate over annexes in *NOTIF by increasing INDEX from zero.  */

#define NOTIF_ITER_ANNEX(NOTIF, INDEX, ANNEX)	\
  for (INDEX = 0; (ANNEX) = &(NOTIF)->annexes[INDEX], (ANNEX)->name != NULL; INDEX++)

/* Notification *NOTIF has annex or not.  */

#define NOTIF_HAS_ANNEX(NOTIF) ((NOTIF)->annexes[0].name != NULL)

/* Whether the annex of notification N is supported.  */

#ifdef GDBSERVER
#define NOTIF_ANNEX_SUPPORTED(STATE, N, INDEX) \
  ((N)->annexes[INDEX].supported)
#else
#define NOTIF_ANNEX_SUPPORTED(STATE, N, INDEX)	\
  ((STATE)->supported[(N)->annexes[INDEX].id])
#endif

/* "Base class" of a notification.  It can be extended in both GDB
   and GDBserver to represent a type of notification.  */

struct notif_base
{
  /* The notification packet, for example, '%Stop'.  Note that '%' is
     not in 'notif_name'.  */
  const char *notif_name;
  /* The name of ack packet, for example, 'vStopped'.  */
  const char *ack_name;

  /* Annexes the notification has.  The notification may or not have
     annexes.  Macro 'NOTIF_HAS_ANNEX' is to check notification has
     annexes, and macro 'NOTIF_ITER_ANNEX' is to iterate over annexes
     of the notification.  */
  struct notif_annex *annexes;
};

char *notif_supported (struct notif_base *notifs[], int num);

#ifndef GDBSERVER
struct remote_notif_state;
#endif

void notif_parse_supported (const char *reply,
			    struct notif_base *notifs[],
#ifdef GDBSERVER
			    int num);
#else
			    int num, struct remote_notif_state *state);
#endif

#endif /* NOTIF_BASE_H */

/* Copyright (C) 2013 Free Software Foundation, Inc.

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

#ifdef GDBSERVER
#include "server.h"
#else
#include "defs.h"
#include "remote-notif.h"
#endif
#include <string.h>
#include "notif-base.h"
#include "gdb_assert.h"

/* Return a string about the notifications in array NOTIFS.  NUM is
   the number of elements in array NOTIFS.  The caller is responsible
   to free the returned string.  Suppose array NOTIFS has
   notifications N1, N2, and N3.  Only N2 has annexes A1 and A2.  The
   returned string is "N1,N2.A2.A2,N3".  */

char *
notif_supported (struct notif_base *notifs[], int num)
{
  int i;
  char * p = NULL;

#define BUF_LEN 128

  for (i = 0; i < num; i++)
    {
      struct notif_base *nb = notifs[i];

      if (p == NULL)
	{
	  p = xmalloc (BUF_LEN);
	  strcpy (p, nb->notif_name);
	}
      else
	xsnprintf (p + strlen (p), BUF_LEN - strlen (p), ",%s",
		   nb->notif_name);

      if (NOTIF_HAS_ANNEX (nb))
	{
	  int j;
	  struct notif_annex *annex;

	  NOTIF_ITER_ANNEX (nb, j, annex)
	    {
	      xsnprintf (p + strlen (p), BUF_LEN - strlen (p),
			 ".%s", annex->name);
	    }
	}
    }

  return p;
}

/* Find annex in notification NB by name NAME and length LEN.
   If found, return its index in NB, otherwise return -1.  */

static int
notif_find_annex (struct notif_base *nb, const char *name, int len)
{
  if (NOTIF_HAS_ANNEX (nb))
    {
      int j;
      struct notif_annex *annex;

      NOTIF_ITER_ANNEX (nb, j, annex)
	if (strncmp (name, annex->name, len) == 0
	    && len == strlen (annex->name))
	  return j;
    }
  return -1;
}

/* Parse the REPLY, which is about supported annexes and
   notifications from the peer, and disable some annexes of
   notification in array NOTIFS if the peer doesn't support.  NUM is
   the number of elements in array NOTIFS.  */

void
notif_parse_supported (const char *reply, struct notif_base *notifs[],
#ifdef GDBSERVER
		       int num)
#else
		       int num, struct remote_notif_state *state)
#endif
{
  const char *p = reply;
  int notif_num = 1;
  char **notif_str;
  int i;

  /* Count how many notifications in REPLY.  */
  for (i = 0; reply[i] != '\0'; i++)
    if (reply[i] == ',')
      notif_num++;

  /* Copy contents of each notification in REPLY to each slot of
     NOTIF_STR.  */
  notif_str = xmalloc (notif_num * sizeof (char *));
  for (i = 0; i < notif_num; i++)
    {
      char *end = strchr (p, ',');

      if (end == NULL)
	notif_str[i] = xstrdup (p);
      else
	{
	  /* Can't use xstrndup in GDBserver.  */
	  notif_str[i] = strndup (p, end - p);
	  p = end + 1;
	}
    }

  /* Iterate each element in NOTIF_STR and parse annex in it.  */
  for (i = 0; i < notif_num; i++)
    {
      int j;
      struct notif_base *nb = NULL;

      p = notif_str[i];

      for (j = 0; j < num; j++)
	{
	  int name_len = strlen (notifs[j]->notif_name);

	  if (0 == strncmp (notifs[j]->notif_name, p, name_len)
	      && (p[name_len] == '.' || p[name_len] == 0))
	    {
	      nb = notifs[j];
	      p += name_len;
	      break;
	    }
	}

      if (nb != NULL)
	{
	  if (p[0] == 0)
	    {
	      /* No annex.  */
	      gdb_assert (!NOTIF_HAS_ANNEX (nb));
	      NOTIF_ANNEX_SUPPORTED (state, nb, 0) = 1;
	    }
	  else if (p[0] == '.')
	    {
	      gdb_assert (NOTIF_HAS_ANNEX (nb));

	      p++;

	      /* Parse the rest of P and look for annexes.  */
	      while (p != NULL)
		{
		  char *end = strchr (p, '.');
		  int index;

		  if (end != NULL)
		    {
		      index = notif_find_annex (nb, p, end - p);
		      p = end + 1;
		    }
		  else
		    {
		      index = notif_find_annex (nb, p, strlen (p));
		      p = end;
		    }

		  /* If annex is known, mark it supported, otherwise
		     skip it because the peer knows the annex but we
		     don't know.  */
		  if (index >= 0)
		    NOTIF_ANNEX_SUPPORTED (state, nb, index) = 1;
		}
	    }
	  else
	    warning (_("Unknown supported notification"));
	}
    }

  for (i = 0; i < notif_num; i++)
    xfree (notif_str[i]);

  xfree (notif_str);
}

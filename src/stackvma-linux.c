/* Determine the virtual memory area of a given address.  Linux version.
   Copyright (C) 2002, 2006, 2008, 2016-2017  Bruno Haible <bruno@clisp.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include "stackvma.h"
#include <stdio.h>

#include "stackvma-simple.c"
#include "stackvma-rofile.c"

#if HAVE_MINCORE
# define sigsegv_get_vma mincore_get_vma
# define STATIC static
# include "stackvma-mincore.c"
# undef sigsegv_get_vma
#endif

struct callback_locals
{
  uintptr_t address;
  struct vma_struct *vma;
#if STACK_DIRECTION < 0
  uintptr_t prev;
#else
  int stop_at_next_vma;
#endif
  int retval;
};

static int
callback (struct callback_locals *locals, uintptr_t start, uintptr_t end)
{
#if STACK_DIRECTION < 0
  if (locals->address >= start && locals->address <= end - 1)
    {
      locals->vma->start = start;
      locals->vma->end = end;
      locals->vma->prev_end = locals->prev;
      locals->retval = 0;
      return 1;
    }
  locals->prev = end;
#else
  if (locals->stop_at_next_vma)
    {
      locals->vma->next_start = start;
      locals->stop_at_next_vma = 0;
      return 1;
    }
  if (locals->address >= start && locals->address <= end - 1)
    {
      locals->vma->start = start;
      locals->vma->end = end;
      locals->retval = 0;
      locals->stop_at_next_vma = 1;
      return 0;
    }
#endif
  return 0;
}

#include "stackvma-vma-iter.c"

int
sigsegv_get_vma (uintptr_t address, struct vma_struct *vma)
{
  struct callback_locals locals;
  locals.address = address;
  locals.vma = vma;
#if STACK_DIRECTION < 0
  locals.prev = 0;
#else
  locals.stop_at_next_vma = 0;
#endif
  locals.retval = -1;

  vma_iterate (&locals);
  if (locals.retval == 0)
    {
#if !(STACK_DIRECTION < 0)
      if (locals.stop_at_next_vma)
        vma->next_start = 0;
#endif
      vma->is_near_this = simple_is_near_this;
      return 0;
    }

#if HAVE_MINCORE
  return mincore_get_vma (address, vma);
#else
  return -1;
#endif
}

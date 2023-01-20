/*
 * Copyright (c) 2023 One Identity LLC
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#include "fuzzing_malloc.h"

extern void
__libc_free(void *);

extern void *
__libc_malloc(size_t);

extern int
__sanitizer_install_malloc_and_free_hooks(
  void (*malloc_hook)(const volatile void *, size_t),
  void (*free_hook)(const volatile void *));

void *
_fuzzer_malloc(size_t size)
{
  return __libc_malloc(size);
}

void *
malloc(size_t size)
{
  return _fuzzer_malloc(size);
}

void
_fuzzer_free(void *ptr)
{
  if(ptr != NULL)
    {
      __libc_free(ptr);
      ptr = NULL;
    }
}

void
free(void *ptr)
{
  _fuzzer_free(ptr);
}

int
register_memory_function_hooks(__int8_t replace_memory_functions)
{
  if(replace_memory_functions != 0)
    {
      int hooks = __sanitizer_install_malloc_and_free_hooks((void (*)(volatile const void *, size_t)) _fuzzer_malloc,
                                                            (void (*)(volatile const void *)) _fuzzer_free);
      if(hooks == 0)
        return -1;
      else
        return 0;
    }
  return 1;
}

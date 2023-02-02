/*
 * Copyright (c) 2023 One Identity LLC.
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

#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include "compression.h"
#include "test_compression_helpers.h"

Compressor *compressor;
GString *input, *result;

static void
setup(void)
{
  cr_redirect_stdout();
  cr_redirect_stderr();
  input = g_string_new(test_message);
}

static void
teardown(void)
{
  g_string_free(input, 0);
}

TestSuite(compression, .init = setup, .fini = teardown);

Test(compression, compressor_gzip_compression)
{
  compressor = gzip_compressor_new();
  cr_assert_not_null(compressor);
  result = g_string_new("");
  compressor_compress(compressor, result, input);
  test_compression_results(result, test_message_gzipped_bytes, test_message_gzipped_length);
  compressor_free(compressor);
  g_string_free(result, 0);
}

Test(compression, compressor_deflate_compression)
{
  compressor = deflate_compressor_new();
  cr_assert_not_null(compressor);
  result = g_string_new("");
  compressor_compress(compressor, result, input);
  test_compression_results(result, test_message_deflated_bytes, test_message_deflated_length);
  compressor_free(compressor);
  g_string_free(result, 0);
}
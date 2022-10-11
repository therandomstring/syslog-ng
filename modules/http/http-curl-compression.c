/*
 * Copyright (c) 2022 One Identity LLC.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#define _DEFLATE_WBITS_DEFLATE MAX_WBITS
#define _DEFLATE_WBITS_GZIP MAX_WBITS + 16

#include "http-curl-compression.h"
#include "messages.h"
#include <zlib.h>

gint8 CURL_COMPRESSION_TYPES_LEN = 3;
gint8 CURL_COMPRESSION_DEFAULT = CURL_COMPRESSION_UNCOMPRESSED;
gchar *CURL_COMPRESSION_LITERAL_ALL = "all";
gchar *curl_compression_types[] = {"identity", "gzip", "deflate"};

enum _DeflateAlgorithmTypes
{
  DEFLATE_TYPE_DEFLATE,
  DEFLATE_TYPE_GZIP

};

typedef enum{
  _COMPRESSION_OK,
  _COMPRESSION_ERR_BUFFER,
  _COMPRESSION_ERR_DATA,
  _COMPRESSION_ERR_STREAM,
  _COMPRESSION_ERR_MEMORY,
  _COMPRESSION_ERR_UNSPECIFIED
} _CompressionUnifiedErrorCode;

gboolean http_dd_curl_compression_string_match(const gchar *string, gint curl_compression_index)
{
  return (strcmp(string, curl_compression_types[curl_compression_index]) == 0);
}

gboolean http_dd_check_curl_compression(const gchar *type)
{
  if(http_dd_curl_compression_string_match(type, CURL_COMPRESSION_UNCOMPRESSED)) return TRUE;
#ifdef SYSLOG_NG_HAVE_ZLIB
  if(http_dd_curl_compression_string_match(type, CURL_COMPRESSION_GZIP)) return TRUE;
#endif
#ifdef SYSLOG_NG_HAVE_ZLIB
  if(http_dd_curl_compression_string_match(type, CURL_COMPRESSION_DEFLATE)) return TRUE;
#endif
  return FALSE;
}

gchar *_compression_error_message = "Failed due to %s error.";
static inline void _handle_compression_error(GString *compression_dest, gchar *error_description)
{
  msg_error("compression", evt_tag_printf("error", _compression_error_message, error_description));
  g_string_free(compression_dest, TRUE);
}
static inline gboolean _raise_compression_status(GString *compression_dest, _CompressionUnifiedErrorCode algorithm_exit)
{
  switch (algorithm_exit)
    {
    case _COMPRESSION_OK:
      return TRUE;
    case _COMPRESSION_ERR_BUFFER:
      _handle_compression_error(compression_dest, "buffer");
      return FALSE;
    case _COMPRESSION_ERR_MEMORY:
      _handle_compression_error(compression_dest, "memory");
      return FALSE;
    case _COMPRESSION_ERR_STREAM:
      _handle_compression_error(compression_dest, "stream");
      return FALSE;
    case _COMPRESSION_ERR_DATA:
      _handle_compression_error(compression_dest, "data");
      return FALSE;
    case _COMPRESSION_ERR_UNSPECIFIED:
      _handle_compression_error(compression_dest, "unspecified");
      return FALSE;
    default:
      g_assert_not_reached();
    }
}

void _allocate_compression_output_buffer(GString *compression_buffer, guint input_size)
{
  g_string_set_size(compression_buffer, (guint)(input_size * 1.1) + 22);
}

_CompressionUnifiedErrorCode _error_code_swap_zlib(int z_err)
{
  switch (z_err)
    {
    case Z_OK:
      return _COMPRESSION_OK;
    case Z_BUF_ERROR:
      return _COMPRESSION_ERR_BUFFER;
    case Z_MEM_ERROR:
      return _COMPRESSION_ERR_MEMORY;
    case Z_STREAM_ERROR:
      return _COMPRESSION_ERR_STREAM;
    case Z_DATA_ERROR:
      return _COMPRESSION_ERR_DATA;
    default:
      return _COMPRESSION_ERR_UNSPECIFIED;
    }
}

_CompressionUnifiedErrorCode _gzip_string(GString *compressed, const GString *message);
_CompressionUnifiedErrorCode _deflate_string(GString *compressed, const GString *message);

_CompressionUnifiedErrorCode _deflate_type_compression(GString *compressed, const GString *message, const gint deflate_algorithm_type);

gboolean http_dd_compress_string(GString *compression_destination, const GString *message, const gint compression)
{
  _CompressionUnifiedErrorCode err_compr;
  switch (compression)
    {
    case CURL_COMPRESSION_GZIP:
      err_compr = _gzip_string(compression_destination, message);
      break;
    case CURL_COMPRESSION_DEFLATE:
      err_compr = _deflate_string(compression_destination, message);
      break;
    default:
      g_assert_not_reached();
    }
  return _raise_compression_status(compression_destination, err_compr);
}

_CompressionUnifiedErrorCode _gzip_string(GString *compressed, const GString *message)
{
  return _deflate_type_compression(compressed, message, DEFLATE_TYPE_GZIP);
}

_CompressionUnifiedErrorCode _deflate_string(GString *compressed, const GString *message)
{
  return _deflate_type_compression(compressed, message, DEFLATE_TYPE_DEFLATE);
}

int _set_deflate_type_wbit(enum _DeflateAlgorithmTypes deflate_algorithm_type)
{
  switch (deflate_algorithm_type)
    {
    case DEFLATE_TYPE_DEFLATE:
      return _DEFLATE_WBITS_DEFLATE;
    case DEFLATE_TYPE_GZIP:
      return _DEFLATE_WBITS_GZIP;
    default:
      g_assert_not_reached();
    }
}

_CompressionUnifiedErrorCode _deflate_type_compression(GString *compressed, const GString *message, const gint deflate_algorithm_type)
{
  z_stream _compress_stream = {0};
  gint err;

  _compress_stream.data_type = Z_TEXT;

  _compress_stream.next_in = (guchar *)message->str;
  _compress_stream.avail_in = message->len;
  _compress_stream.total_in = _compress_stream.avail_in;

  _allocate_compression_output_buffer(compressed, _compress_stream.avail_in);

  //Check buffer overrun
  if(_compress_stream.avail_in != message->len)
    {
      return _COMPRESSION_ERR_BUFFER;
    }
  _compress_stream.next_out = (guchar *)compressed->str;
  _compress_stream.avail_out = compressed->len;
  _compress_stream.total_out = _compress_stream.avail_out;
  if(_compress_stream.avail_out != compressed->len)
    {
      return _COMPRESSION_ERR_BUFFER;
    }

  gint _wbits = _set_deflate_type_wbit(deflate_algorithm_type);

  err = deflateInit2(&_compress_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, _wbits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
  if (err != Z_OK && err != Z_STREAM_END) return _error_code_swap_zlib(err);
  err = Z_OK;
  while(TRUE)
    {
      err = deflate(&_compress_stream, Z_FINISH);
      if (err != Z_OK && err != Z_STREAM_END)
        break;
      if (err == Z_STREAM_END)
        {
          err = Z_OK;
          deflateEnd(&_compress_stream);
          g_string_set_size(compressed, compressed->len - _compress_stream.avail_out);
          break;
        }
    }
  return _error_code_swap_zlib(err);
}

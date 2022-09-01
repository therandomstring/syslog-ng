/*
 * Copyright (c) 2022 One Identity, Ltd.
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

#define _COMPRESSION_OK 0
#define _COMPRESSION_ERR_BUFFER 1
#define _COMPRESSION_ERR_DATA 2
#define _COMPRESSION_ERR_STREAM 3
#define _COMPRESSION_ERR_MEMORY 4
#define _COMPRESSION_ERR_UNSPECIFIED 255

#define _DEFLATE_WBITS_DEFLATE MAX_WBITS
#define _DEFLATE_WBITS_GZIP MAX_WBITS + 16

#include "http-curl-compression.h"
#include "messages.h"
#include <zlib.h>

enum _DeflateAlgorithmTypes
{
    DEFLATE_TYPE_DEFLATE,
    DEFLATE_TYPE_GZIP

};

gchar *_compression_error_message = "Failed due to %s error. Sending uncompressed data.";
inline void _handle_compression_error(GString *compression_dest, gchar* error_description)
{
  msg_error("compression", evt_tag_printf("error", _compression_error_message, error_description));
  g_string_free(compression_dest, TRUE);
}
inline gboolean _raise_compression_status(GString *compression_dest, int algorithm_exit)
{
  switch (algorithm_exit) {
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

int _gzip_string(GString *compressed, const GString *message);
int _deflate_string(GString *compressed, const GString *message);
int _brotli_string(GString *compressed, const GString *message);
int _zstd_string(GString *compressed, const GString *message);

int _deflate_type_compression(GString *compressed, const GString *message, const gint deflate_algorithm_type);

gboolean http_dd_compress_string(GString *compression_destination, const GString *message, const gint compression)
{
  
  int err_compr;
  switch (compression) {
    case CURL_COMPRESSION_GZIP:
      err_compr = _gzip_string(compression_destination, message);
      break;
    case CURL_COMPRESSION_DEFLATE:
      err_compr = _deflate_string(compression_destination, message);
      break;
    case CURL_COMPRESSION_BROTLI:
      err_compr = _brotli_string(compression_destination, message);
      break;
    case CURL_COMPRESSION_ZSTD:
      err_compr = _zstd_string(compression_destination, message);
      break;
    default:
      g_assert_not_reached();
  }
  return _raise_compression_status(compression_destination, err_compr);
}

int _gzip_string(GString *compressed, const GString *message)
{
  return _deflate_type_compression(compressed, message, DEFLATE_TYPE_GZIP);
}

int _deflate_string(GString *compressed, const GString *message)
{
  return _deflate_type_compression(compressed, message, DEFLATE_TYPE_DEFLATE);
}

int _brotli_string(GString *compressed, const GString *message)
{
  //not yet implemented
}

int _zstd_string(GString *compressed, const GString *message)
{
  //not yet implemented
}

int _deflate_type_compression(GString *compressed, const GString *message, const gint deflate_algorithm_type)
{
  z_stream _compress_stream;
  gint err;

  _compress_stream.next_in = (guchar *)message->str;
  _compress_stream.avail_in = message->len;

  //Check buffer overrun
  if(_compress_stream.avail_in != message->len)
  {
    return _COMPRESSION_ERR_BUFFER;
  }
  _compress_stream.next_out = (guchar *)compressed->str;
  _compress_stream.avail_out = (guint)(_compress_stream.avail_in * 1.1) + 22; //This might not work
  if(_compress_stream.avail_out != (guint)(message->len * 1.1) + 22)
  {
    return _COMPRESSION_ERR_BUFFER;
  }

  gint _wbits;
  switch (deflate_algorithm_type) {
    case DEFLATE_TYPE_DEFLATE:
      _wbits = _DEFLATE_WBITS_DEFLATE;
      break;
    case DEFLATE_TYPE_GZIP:
      _wbits = _DEFLATE_WBITS_GZIP;
      break;
    default:
      g_assert_not_reached();
  }

  deflateInit2(&_compress_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, _wbits, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
  err = deflate(&_compress_stream, Z_NO_FLUSH);
  deflateEnd(&_compress_stream);

  switch (err) {
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
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

#ifndef SYSLOG_NG_HTTP_CURL_COMPRESSION_H
#define SYSLOG_NG_HTTP_CURL_COMPRESSION_H

#define CURL_OPTION_DELIMITER ", "

/* Uncomment upon implementation */
#define CURL_COMPRESSION_IMPLEMENTED_GZIP
#define CURL_COMPRESSION_IMPLEMENTED_DEFLATE
//#define CURL_COMPRESSION_IMPLEMENTED_BROTLI
//#define CURL_COMPRESSION_IMPLEMENTED_ZSTD

//TODO: revise includes
#include <curl/curl.h>
#include <glib.h>

enum CurlCompressionTypes{
  CURL_COMPRESSION_UNCOMPRESSED = 0,
  CURL_COMPRESSION_GZIP = 1,
  CURL_COMPRESSION_DEFLATE = 2,
  CURL_COMPRESSION_BROTLI = 3,
  CURL_COMPRESSION_ZSTD = 4
} CurlCompressionTypes;
gint8 CURL_COMPRESSION_TYPES_LEN = 5;

gint8 CURL_COMPRESSION_DEFAULT = CURL_COMPRESSION_UNCOMPRESSED;
gchar *CURL_COMPRESSION_LITERAL_ALL = "all";

gchar *curl_compression_types[] = {"identity", "gzip", "deflate", "brotli", "zstd"};
gboolean http_dd_compress_string(GString *compression_destination, const GString *message, const gint compression);

#endif //SYSLOG_NG_HTTP_CURL_COMPRESSION_H

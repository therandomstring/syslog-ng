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

#include <malloc.h>
#include <iv.h>
#include "fuzzing_helper.h"
#include "apphook.h"
#include "../../../modules/syslogformat/syslog-format.h"


AppInfo *
app_init(const char *module)
{
  AppInfo *app = malloc(sizeof(AppInfo));
  app->config = cfg_new_snippet();
  app_startup();
  cfg_load_module(app->config, module);
  cfg_load_module(app->config, "syslogformat");
  msg_format_options_defaults(&app->parse_options);

  return app;
}

void
app_deinit(AppInfo *app)
{
  cfg_free(app->config);
  app_shutdown();
  iv_deinit();

  free(app);
}

LogMessage *
syslog_message_new(AppInfo *app, const uint8_t *data, size_t size)
{
  LogMessage *message = log_msg_new_empty();
  syslog_format_handler(&app->parse_options, message, data, size, NULL);
  return message;
}

void
syslog_message_free(LogMessage *message)
{
  log_msg_unref(message);
}

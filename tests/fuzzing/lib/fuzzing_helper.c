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
#include "fuzzing_malloc.h"
#include "apphook.h"
#include "../../../modules/syslogformat/syslog-format.h"

//FIXME: can't load module due to module path being unset
void
_app_load_extra_module_method(AppInfo *app, const char *module){
  cfg_load_module(app->config, module);
}

void
_app_init(AppInfo *app)
{
  app->load_module = _app_load_extra_module_method;
  app->config = cfg_new_snippet();
  app_startup();
  cfg_load_module(app->config, "syslogformat");
  msg_format_options_defaults(&app->parse_options);
}

AppInfo *
app_new(void)
{
  AppInfo *app = malloc(sizeof(AppInfo));
  _app_init(app);
  return app;
}

void
_app_deinit(AppInfo *app)
{
  cfg_free(app->config);
  app_shutdown();
  iv_deinit();
}

void
app_free(AppInfo *app)
{
  _app_deinit(app);
  free(app);
}

LogMessage *
syslog_message_new(AppInfo *app, const uint8_t *data, size_t size, int *success)
{
  LogMessage *message = log_msg_new_empty();
  gsize problem_index;
  *success = syslog_format_handler(&app->parse_options, message, data, size, &problem_index);
  return message;
}

void
syslog_message_free(LogMessage *message)
{
  log_msg_unref(message);
}

void
destination_worker_connect_and_insert(LogThreadedDestWorker *destination_worker, LogMessage *message)
{
  log_threaded_dest_worker_connect((LogThreadedDestWorker *) destination_worker);
  log_threaded_dest_worker_insert((LogThreadedDestWorker *) destination_worker, message);
  log_threaded_dest_worker_disconnect((LogThreadedDestWorker *) destination_worker);
}

int
test_init(gint8 initialize_memory_functions)
{
  int malloc_register_success = register_memory_function_hooks(initialize_memory_functions);
  switch (malloc_register_success)
  {
    case -1:
      printf("WARNING: registering memory management functions wit sanitizer failed.\n");
      break;
    case 0:
      printf("Memory management hook functions registered successfully.\n");
      break;
    case 1:
      printf("WARNING: registering memory management functions wit sanitizer impossible. No sanitizer found");
      break;
    default:
      g_assert_not_reached();
  }
  return -1;
}

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
#include "http.h"
#include "http-worker.h"
#include "test_compression_helpers.h"
#include "apphook.h"
#include "syslog-format.h"
#include "msg-format.h"

MainLoop *main_loop;
MainLoopOptions main_loop_options;
GlobalConfig *config;
MsgFormatOptions parse_options;

HTTPDestinationDriver *driver;
HTTPDestinationWorker *worker;

LogMessage *message;

static void
_init_worker(gchar *compression_type)
{
  http_dd_set_message_compression((LogDriver *) driver, compression_type);
  worker = (HTTPDestinationWorker *) driver->super.workers[0];
  log_threaded_dest_worker_init((LogThreadedDestWorker *) worker);
}

/* FIXME: Admittedly a bit of a hack-job.
* Something has to set the batch_size and number of targets,
* or no insertion wil happen, resulting in no flush,
* and consecutively no sent message.
* Please notify me if there is a better solution
*
* RandomString */
static void
_bypass_insertion_precheck(void)
{
  log_threaded_dest_worker_ack_messages((LogThreadedDestWorker *) worker, 1);
  http_load_balancer_add_target(driver->load_balancer, "dummy.url");
}

static void
_start_main_loop(void)
{
  app_startup();
  main_loop = main_loop_get_instance();
  main_loop_init(main_loop, &main_loop_options);
  config = main_loop_get_current_config(main_loop);
}

static void
_init_driver(void)
{
  driver = (HTTPDestinationDriver *) http_dd_new(main_loop_get_current_config(main_loop));
  cr_assert_not_null(driver);
  log_threaded_dest_driver_set_num_workers((LogDriver *) driver, 1);
  log_dest_driver_init_instance((LogDestDriver *) driver, config);
  log_threaded_dest_driver_init_method((LogPipe *) driver);
}

static void
_init_test_message(void)
{
  msg_format_options_init(&parse_options, config);
  parse_options.flags = LP_NO_HEADER;
  message = log_msg_new_empty();
  gsize problem_index;
  gboolean success = syslog_format_handler(&parse_options, message, (const guchar *) test_message, strlen(test_message),
                                           &problem_index);
  cr_assert(success);
}

static void
_check_driver_header_contains(const char *field)
{
  GList *header = driver->headers;
  gboolean result = false;
  while(header != NULL)
    {
      if (strcmp((gchar *)header->data, field) == 0)
        {
          result = true;
          break;
        }
      header = header->next;
    }
  cr_assert(result);
}

static void
setup(void)
{
  debug_flag = TRUE;
  log_stderr = TRUE;

  _start_main_loop();
  _init_driver();
  _init_test_message();
};

static void
teardown(void)
{
  log_threaded_dest_worker_deinit((LogThreadedDestWorker *) worker);
  log_threaded_dest_worker_free((LogThreadedDestWorker *) worker);
  log_msg_unref(message);

  main_loop_sync_worker_startup_and_teardown();
  log_pipe_deinit((LogPipe *) driver);
  log_pipe_unref((LogPipe *) driver);
  main_loop_deinit(main_loop);
  app_shutdown();
};

TestSuite(http_compression, .init=setup, .fini=teardown);

Test(http_compression, no_compression)
{
  _init_worker("identity");

  log_threaded_dest_worker_insert((LogThreadedDestWorker *) worker, message);
  cr_assert_str_eq(worker->request_body->str, test_message);
  cr_assert_null(worker->request_body_compressed);
}

Test(http_compression, gzip_compression)
{
  _init_worker("gzip");

  _bypass_insertion_precheck();
  log_threaded_dest_worker_insert((LogThreadedDestWorker *) worker, message);
  cr_assert_not_null(worker->request_body_compressed);
  test_compression_results(worker->request_body_compressed, test_message_gzipped_bytes, test_message_gzipped_length);
}

Test(http_compression, deflate_compression)
{
  _init_worker("deflate");

  _bypass_insertion_precheck();
  log_threaded_dest_worker_insert((LogThreadedDestWorker *) worker, message);
  cr_assert_not_null(worker->request_body_compressed);
  test_compression_results(worker->request_body_compressed, test_message_deflated_bytes, test_message_deflated_length);
}

Test(http_compression, compression_sets_http_header)
{
  _init_worker("deflate");

  _bypass_insertion_precheck();
  log_threaded_dest_worker_insert((LogThreadedDestWorker *) worker, message);
  _check_driver_header_contains("Content-Encoding: deflate");
}
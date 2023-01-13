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

#include "examples/destinations/example_destination/example_destination.h"
#include "examples/destinations/example_destination/example_destination_worker.h"
#include "fuzzing_helper.h"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size){
  if (size <= 1) return 0;

  AppInfo *app = app_new();
  app->load_module(app, "example_destination");

  //TODO: maybe this also can be a helper macro/template
  ExampleDestinationDriver *dd = (ExampleDestinationDriver *) example_destination_dd_new(app->config);
  ExampleDestinationWorker *dw = (ExampleDestinationWorker *) example_destination_dw_new(&dd->super, 0);

  example_destination_dd_set_filename((LogDriver *) dd, "fuzz_example-test_output");

  int message_parse_success;
  LogMessage *message = syslog_message_new(app, data, size, &message_parse_success);
  if(!message_parse_success)
    {
      return 0;
    }

  destination_worker_connect_and_insert((LogThreadedDestWorker *) dw, message);

  syslog_message_free(message);
  app_free(app);
	return 0;
}

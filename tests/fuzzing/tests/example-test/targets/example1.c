#include "examples/destinations/example_destination/example_destination.h"
#include "examples/destinations/example_destination/example_destination_worker.h"
#include "msg-format.h"
#include "fuzzing_helper.h"

//FIXME: secret storage init failss
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size){
  if (size <= 1) return 0;

  AppInfo *app = app_init("example-destination");

  //TODO: maybe this also can be a helper macro/template
  ExampleDestinationDriver *dd = (ExampleDestinationDriver *) example_destination_dd_new(app->config);
  ExampleDestinationWorker *dw = (ExampleDestinationWorker *) example_destination_dw_new(&dd->super, 0);

  example_destination_dd_set_filename((LogDriver *) dd, "fuzz_example-test_output");

  LogMessage *message = syslog_message_new(app, data, size);

  log_threaded_dest_worker_connect((LogThreadedDestWorker *) dw);
  log_threaded_dest_worker_insert((LogThreadedDestWorker *) dw, message);
  log_threaded_dest_worker_disconnect((LogThreadedDestWorker *) dw);

  syslog_message_free(message);
  app_deinit(app);
	return 0;
}

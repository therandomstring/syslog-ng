`java`: Due to a known JVM dlopen/dlclose limitation, `java` module has been disabled in the
official binaries. The module is still available to compile.

`grpc`: Fixed RSS leak on config reload in the BigQuery, ClickHouse, Loki,
OpenTelemetry, and Pub/Sub destinations.

`http()`: Added compression ability for use with metered egress/ingress

The new features can be accessed with the 
 * `accept-encoding()` for inward http messages (currently not used by syslog-ng). The available options are `identity` (for no compression), `gzip` or `deflate` between quotation marks. If you want the driver to accept multiple compression types, you can list them separated by commas, or write `all`, if you want to enable all available compression types.
 * `content-compression()` for compressing messages sent by syslog-ng. The available options are `identity`(again, for no compression), `gzip`, or `deflate` between quotation marks.
 
 Below you can see a configuration example:
 
```
destination d_http_compressed{
	http(url("127.0.0.1:80"), content-compression("deflate"), accept-encoding("all"));
};
```


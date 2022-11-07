TODO
 - support koofr api
 - use api credentials from config (multiple api support)
 - refactor: use bearer oauth class header method instead of manual input

221107 
 - add base64.encode;
 - rest: use base64 basic authorization instead of sending credentials in url;
 - oauth2: get registration url instead of local const;
 - app config: store scope, support for multiple credentials, keep device code even after refresh, store sync ts; 
 - try to update app config file instead of rewriting it from scratch;
 - json: pretty print to file;

Previous
 - add Makefile to run .self in emulator with debug listener (make -f run)
 - workaround to fix directory reading loops (rpcs3 bug)
 - exctract api credentials into non public ('hidden') credentials.h header
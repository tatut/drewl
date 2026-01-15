# drewl

Drewl is a command line [webview](https://github.com/webview/webview) wrapper that
is meant to be controlled from other software (like [Emacs](https://www.gnu.org/software/emacs/)).

Drewl reads commands from stdin and controls the webview. Responses are returned as JSON wrapped
in ASCII STX (2) and ETX (3) control characters so they can be separated
from any logging the webview is doing.

Command responses are returned as `{"success": true/false, data: <result or error>}`.
If console logging is instrumented on the page, logs are returned as
`{"log": "info", "data":{"0":"arg" ...}}` where log key contains the function used (like log, info or warn)
and data is the `arguments` to the call.

## Quickstart

Compile:
```
cmake .
make
```

Start:
```
$ ./bin/drewl
go http://example.com
eval document.title
{"success": true, "data": "Example Domain"}
eval foo.bar
{"success": false, "data": "ReferenceError: Can't find variable: foo"}
```

## Commands

Drewl supports the following commands:

* `go <url>` navigates to the given URL
* `reload` reload current URL
* `back` go back in history
* `forward` go forward in history
* `click <selector>` queries the given selector and clicks it
* `eval <code>` evaluate given JS code
* `html <selector>` returns the HTML of the given selector
* `console` instrument console logging
* `wsize <w> <h>` set window size
*  `help` print help
*  `exit` `quit` (or just EOF) closes the webview and exits

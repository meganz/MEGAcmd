# Debugging MEGAcmd

There are two different kinds of logging messages:
- MEGAcmd: these messages are reported by MEGAcmd itself. These messages will show information regarding the processing of user commands. They will be labeled with `cmd`.

- SDK: these messages are reported by the sdk and its dependent libraries. These messages will show information regarding requests, transfers, network, etc. They will be labeled with `sdk`.

The MEGAcmd server logs messages depending on the level of log adjusted to those two categories. You can adjust the level of logging for those kinds with `log` command. Log levels range from FATAL (the lowest) to VERBOSE (the highest). The log level of each message is displayed after the cmd/sdk prefix.

Here's an example of some random log messages:
```
2025-02-07_16-47-56.662269 cmd DBG  Registering state listener petition with socket: 85 [comunicationsmanagerfilesockets.cpp:189]
2025-02-07_16-47-56.662366 cmd DTL  Unregistering no longer listening client. Original petition: registerstatelistener [comunicationsmanagerfilesockets.cpp:346]
2025-02-07_16-47-56.662671 sdk INFO Request (RETRY_PENDING_CONNECTIONS) starting [megaapi_impl.cpp:16964]
```
The source file and line the message was logged from is appended at the end of the log message, to help developers quickly find out where they came from.


## How to access the logs

Logs coming from MEGAcmd are written to the `megacmdserver.log` file. This file is not removed across restarts, so when it gets too large it is automatically compressed and renamed to include a timestamp (without stopping the logging).

The log file is located in `$HOME/.megaCmd` for Linux and macOS, and `%LOCALAPPDATA%\MEGAcmd\.megaCmd` for Windows.

## Verbosity on startup

You can start the server with higher level of verbosity in order to have log levels increased at startup, regardless of the log level configured by the `log` command mentioned above.
You will need to pass `--debug-full` as an argument to the executable (e.g: `MEGAcmdServer.exe --debug-full`). Alternatively, you can set the `MEGACMD_LOGLEVEL` environment variable to `FULLVERBOSE` before starting the server.

If you want other startup level of logging, you can use the following:
* `--debug` or `MEGACMD_LOGLEVEL=DEBUG` will set
    ```
    MEGAcmd log level = DEBUG
    SDK log level = DEFAULT
    ```

* `--debug-full` or `MEGACMD_LOGLEVEL=DEBUG` will set
    ```
    MEGAcmd log level = DEBUG
    SDK log level = DEBUG
    ```

* `--verbose` or `MEGACMD_LOGLEVEL=VERBOSE` will set
    ```
    MEGAcmd log level = VERBOSE
    SDK log level = DEFAULT
    ```

* `--verbose-full` or `MEGACMD_LOGLEVEL=FULLVERBOSE` will set
    ```
    MEGAcmd log level = VERBOSE
    SDK log level = VERBOSE
    ```

## Controlling verbosity of a single command
In general, as we've mentioned before, lower verbosity log messages are not printed directly to the console. Only errors are. You can pass `-v`, `-vv`, and `-vvv` when running a command to ensure warning, debug, and verbose messages are printed (respectively). Note that this is only for the console; log level of `megacmdserver.log` will follow the rules explained above.

## JSON logs
When the log level of the SDK is `VERBOSE`, its commands are outputted as JSON. This takes up a bit more space but provides more valuable info. JSON logs can be overwritten independently by setting the environment variable `MEGACMD_JSON_LOGS` to `0` or `1`.

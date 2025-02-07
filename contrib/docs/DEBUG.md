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

You can start the server with higher level of verbosity in order to have log levels increased at startup.
In Windows & Linux you will need to pass `--debug-full` as an argument to the executable (e.g: `MEGAcmdServer.exe --debug-full`).

In MacOS, you can use `MEGACMD_LOGLEVEL` environment variable like this: `MEGACMD_LOGLEVEL=FULLDEBUG ./mega-cmd`.

If you want other startup level of loggin, you can use:

* `--debug`
* `MEGACMD_LOGLEVEL=DEBUG`

This will set:

MEGAcmd log level = DEBUG
SDK log level = DEFAULT

* `--debug-full`
* `MEGACMD_LOGLEVEL=DEBUG`

This will set:

MEGAcmd log level = DEBUG
SDK log level = DEBUG

* `--verbose`
* `MEGACMD_LOGLEVEL=VERBOSE`

This will set:

MEGAcmd log level = VERBOSE
SDK log level = DEFAULT

* `--verbose-full`
* `MEGACMD_LOGLEVEL=FULLVERBOSE`

This will set:

MEGAcmd log level = VERBOSE
SDK log level = VERBOSE


## Controlling verbosity of a single command

You can pass `-v` (`-vv`, `-vvv`, and so on for a more verbose output)
to an specific command and it will use higher level of verbosity of MEGAcmd based messages.


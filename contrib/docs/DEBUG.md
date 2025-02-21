# Debugging MEGAcmd

There are two different kinds of logging messages:
- MEGAcmd based: those messages reported by MEGAcmd itself.

These messages will show information regarding the processing of user commands.

- SDK based: those messages reported by the sdk and dependent libraries.

These messages will show information regarding requests, transfers, network, etc.
They will be labeled with `SDK:`.

MEGAcmdServer logs messages depending on the level of log adjusted to those
two categories. You can adjust the level of logging for those kinds with `log` command.
Log levels range from FATAL (the lowest) to VERBOSE (the highest).

## How to access the logs

Accessing the logs depends on the platform you are in.

### MacOS

By default, whenever MEGAcmdServer is executed, it will log the output to `$HOME/.megaCmd/megacmdserver.log`.

If you want to launch it manually execute in a terminal:

```
export PATH=/Applications/MEGAcmd.app/Contents/MacOS:$PATH
./mega-cmd
```

### Linux
By default, whenever MEGAcmdServer is executed, it will log the output to `$HOME/.megaCmd/megacmdserver.log`.

If you want to launch it manually execute in a terminal:

```
mega-cmd-server
```

### Windows

MEGAcmdServer is executed in the background without saving the log into a file. If you want to
see the output you would need to execute the server (MEGAcmdServer.exe) manually.

## Accessing stdout and stderr

The standard output and error streams can be found in the `megacmdserver.log.out` and `megacmdserver.log.err` files, respectively. They're located in the same directories as the logs.

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

## JSON logs
When the log level of the SDK is `VERBOSE`, its commands are outputted as JSON. This takes up a bit more space but provides more valuable info. JSON logs can be overwritten independently by setting the environment variable `MEGACMD_JSON_LOGS` to `0` or `1`.

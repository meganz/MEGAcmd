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
When the log level of the SDK is `VERBOSE`, the entire JSON payload of the HTTP requests sent and received from the API will be logged. This takes up a bit more space but provides more valuable info. Full JSON logging can be overwritten independently by setting the environment variable `MEGACMD_JSON_LOGS` to `0` or `1`.

## Configuring the Rotating Logger
The MEGAcmd logger rotates and compresses log files to avoid taking up too much space. Some of its values can be configured to fit the needs of specific systems. These are:
* `RotationType`: The type of rotation to use. Possible values are _Timestamp_ and _Numbered_. Defaults to _Timestamp_.
    * Numbered rotation will add a "file number" suffix when rotating files, keeping track of the total number and removing them if there are more than `MaxFilesToKeep`.
    * Timestamp rotation will keep track of the total number (similarly to above), while also keeping track of the creation date of files, removing them if they're older than `MaxFileAge`.
* `CompressionType`: The type of compression to use. Possible values are _Gzip_ and _None_ (which disables compression). Defaults to _Gzip_.
* `MaxFileMB`: The maximum size the `megacmdserver.log` file can be, in megabytes. If it gets over this size, it'll be renamed and compressed according to the rules stated above. Default is usually 50 MB, but will be less for disks with limited space.
* `MaxFilesToKeep`: The maximum amount of rotated files allowed. When the total file count exceeds this value, older files will be removed. Default depends on `MaxFileMB`, the compression used, and the system specs.
* `MaxFileAgeSeconds`: The maximum age the rotated files can be before being deleted, in seconds. Defaults to 1 month. _Note_: Only used by timestamp-based rotation.
* `MaxMessageBusMB`: The maximum memory allowed by the logger's internal bus, in megabytes. Defaults to 512 MB. In most cases, the logger will use way less RAM; it is recommended to check memory usage before changing this value.

To configure them we must manually edit the `megacmd.cfg` file. This file must be present in the same directory as the `megacmdserver.log` file; if not, we can manually create it. The following is an example of the syntax of this file:
```
RotatingLogger:RotationType=Timestamp
RotatingLogger:CompressionType=None
RotatingLogger:MaxFileMB=40.25
RotatingLogger:MaxFilesToKeep=20
RotatingLogger:MaxFileAgeSeconds=3600
RotatingLogger:MaxMessageBusMB=64.0
```
Values not present in it will be set to their default. Invalid values (such as negative sizes) will be silently discarded. Note that this configuration is only loaded at the start, so the MEGAcmd server must be restarted after adding or changing any of the values.

Configuring the Rotating Logger might result in previous log files not being rotated or deleted properly. It is recommended to delete them manually (or moving them somewhere else, if we want to preserve them) before changing the configuration.



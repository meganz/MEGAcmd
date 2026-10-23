### configure
Shows and modifies global configurations.

Usage: `configure [key [value]]`
<pre>
If no keys are provided, it will list all configuration keys and values.
If a key is provided, but no value given, it will only show the value of such key.
If a key and value are provided, it will set the value of that key.

Possible keys:
 - max_nodes_in_cache                 Max nodes loaded in memory.
                                      This controls the number of nodes that the
                                      SDK stores in memory.
 - exported_folders_sdks              Number of additional SDK instances loaded at startup.
                                      This controls the number of SDK instances
                                      that are created at startup in order to
                                      download or import contents from exported
                                      folder links. Default 5. Min 0. Max 20. If
                                      set to 0, you will not be able to download
                                      or import from folder links.
 - file_service_reclaim_age_threshold File-service cache cleanup: minimum file age in minutes.
                                      A cached file is only removed after going
                                      unaccessed for this many minutes, so this
                                      is what decides how much a cleanup can
                                      actually free. Default 4320 (3 days).
 - file_service_reclaim_batch_size    File-service cache cleanup: files removed at a time.
                                      How many files each cleanup removes at a
                                      time. Rarely needs changing. Default 4.
 - file_service_reclaim_delay         File-service cache cleanup: seconds before the first cleanup.
                                      How long (in seconds) to wait before the
                                      first cleanup. The wait restarts on
                                      login/startup and whenever any
                                      file_service_reclaim_* value is changed.
                                      Default 1800 (30 minutes).
 - file_service_reclaim_period        File-service cache cleanup: seconds between cleanups.
                                      How long (in seconds) to wait between one
                                      cleanup and the next. Default 7200 (2
                                      hours).
 - file_service_reclaim_threshold     File-service cache cleanup: cache size in bytes that triggers a cleanup.
                                      A scheduled cleanup only does something
                                      while the cache is above this many bytes;
                                      it then removes old files until the cache
                                      is down to file_service_reclaim_target.
                                      Nothing prevents the cache from growing
                                      past this. Use "off" to disable automatic
                                      cleanup, or 0 to let cleanups run at any
                                      cache size. Default 10737418240 (10 GiB).
 - file_service_reclaim_target        File-service cache cleanup: cache size in bytes left after a cleanup.
                                      The cache size (in bytes) a cleanup shrinks
                                      the cache down to. Default 1073741824 (1
                                      GiB).
 - fuse_log_level                     Log level of the SDK's FUSE component.
                                      Takes the same values as the "log" command:
                                      a name (ERROR, WARNING, INFO, DEBUG,
                                      VERBOSE) or its number. The new level
                                      applies to mounts that are already enabled.
                                      FUSE messages also go through the SDK
                                      logger, so "log -s DEBUG" is needed for
                                      them to reach the log file. Default INFO.

The file_service_reclaim_* keys control the automatic cleanup of the on-disk cache that webdav
streaming and FUSE mounts write to. Cleanups run on a timer, and each one only does something
while the cache is above file_service_reclaim_threshold, so that threshold bounds nothing on its own.
Changes take effect immediately and are re-applied on each login.
For how the cleanup works and for values that keep heavy streaming within a disk budget, see "mega-help --streaming".
</pre>

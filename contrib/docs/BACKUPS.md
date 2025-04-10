# MEGA-BACKUPS - Backing up folders with MEGAcmd
This is a brief tutorial on how to configure backups.

Notice: the commands listed here assume you are using the interactive mode: they are supposed to be executed within MEGAcmdShell.

## Creation
Example: 
backup /path/mega/folder /remote/path --period="0 0 4 * * *" --num-backups=10

This will configure a backup of "myfolder" into /remote/path that will be carried out
 at 4:00 A.M. (UTC) every day. It will store the last 10 copies.
 Notice a first backup will be carried out immediately.
 In this example we are using cron-time expressions.
 You can find extra info on those using "backup --help".
 
Backups will be stored as:
```
 /remote/path/myfolder_bk_TIME1
 /remote/path/myfolder_bk_TIME2
 /remote/path/myfolder_bk_TIME3
 ...
```

## Listing 

You can list the backups configured typing `backup`:

```
TAG   LOCALPATH                 REMOTEPARENTPATH                  STATUS
4     /path/mega/folder            /remote/path                   ACTIVE
```

Notice the TAG. You can use it to refer to the backup if you want to change its configuration 
or delete/abort it.

### Extra info

If you type "backup -l" you will see extra information concerning the backup. Here, you will 
see when the next backup is scheduled for:
```
TAG   LOCALPATH                 REMOTEPARENTPATH                  STATUS
4     /path/mega/folder            /remote/path                  ONGOING
  Max Backups:   4
  Period:         "0 0 4 * * *"
  Next backup scheduled for: Fri, 19 Jan 2018 04:00:00 +0000
   -- CURRENT/LAST BACKUP --
  FILES UP/TOT     FOLDERS CREATED    PROGRESS
       22/33                0         57.86/57.86 KB  1.27%

```

Also, you can see the progress of the current backup 
(or the last one if there is no backup being performed at the moment)

### Backup history:
With "backup -h" you will be able to see the existing backups with their state and start time:

```
TAG   LOCALPATH                 REMOTEPARENTPATH                  STATUS
4     /path/mega/folder            /remote/path                  ONGOING
   -- SAVED BACKUPS --
  NAME                             DATE                    STATUS  FILES FOLDERS
  myfolder_bk_20180115175811       15Jan2018 17:58:11    COMPLETE     33      10
  myfolder_bk_20180116182611       16Jan2018 18:26:11    COMPLETE     33      10
  myfolder_bk_20180117182711       17Jan2018 18:27:11     ABORTED     13      10
  myfolder_bk_20180118182911       18Jan2018 18:29:11     ONGOING     23      10
```

Tip: If you are using linux/mac you can monitor the status actively in non-interactive mode with:
```
watch mega-backup -lh
```

# Control:

## Abort

You can abort an ONGOING backup using its tag or its local path. e.g.:
```
backup -a 4
```
This will cancel all transfers and set the backup as ABORTED

## Delete

Similarly you can remove a backup, to no longer backup that folder with:
```
backup -d /path/mega/folder
``` 
This will not remove the existing backups wich will be available in MEGA.

## Change configuration

Similarly you can change the period or the number of backups to keep with:
```
backup 4 --period=2h
```
This will set our backup with TAG=4 to have a period of 2 hours.
```
backup /path/mega/folder --num-backups=1
```
This will configure the backup to only keep one instance. 
Notice that in order not to lose data, older instances will not be deleted until
the max number of backups is passed.

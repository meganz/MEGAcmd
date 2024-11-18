### sync
Controls synchronizations.

Usage: `sync [localpath dstremotepath| [-dsr] [ID|localpath]`
<pre>
If no argument is provided, it lists current configured synchronizations.

If provided local and remote paths, it will start synchronizing
 a local folder into a remote folder.

If an ID/local path is provided, it will list such synchronization
 unless an option is specified.

Options:
-d | --remove ID|localpath	deletes a synchronization.
-s | --disable ID|localpath	stops(pauses) a synchronization.
-r | --enable ID|localpath	resumes a synchronization.
 --path-display-size=N	Use at least N characters for displaying paths.
 --show-handles	Prints remote nodes handles (H:XXXXXXXX).
 --col-separator=X	Tt will use X as column separator. Otherwise the output will use
                     	 spaces to tabulate in an easy to read output:
 --output-cols=COLUMN_NAME_1,COLUMN_NAME2,...	You can select which columns to show (and their order) with this option

DISPLAYED columns:
 ID: an unique identifier of the sync.
 LOCALPATH: local synced path.
 REMOTEPATH: remote synced path (in MEGA).
 RUN_STATE: Indication of running state, possible values:
 	Pending: Sync config has loaded but we have not attempted to start it yet.
 	Loading: Sync is in the process of loading from disk.
 	Running: Sync loaded and active.
 	Paused: Sync loaded but sync logic is suspended for now..
 	Suspended: Sync is not loaded, but it is on disk with the last known sync state.
 	Disabled: Sync has been disabled (no state cached). Starting it is like configuring a brand new sync with those settings.
 STATUS: State of the sync, possible values:
 	NONE: Status unknown.
 	Synced: Synced, no transfers/pending actions are ongoing.
 	Pending: Sync engine is doing some calculations.
 	Syncing: Transfers/pending actions are being carried out.
 	Processing: State cannot be determined (too busy, try again later).
 ERROR: Error, if any.
 SIZE, FILE & DIRS: size, number of files and number of dirs in the remote folder.
</pre>

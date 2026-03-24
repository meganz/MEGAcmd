### transfers
List or operate with transfers

Usage: `transfers [-c TAG|-a] | [-r TAG|-a]  | [-p TAG|-a] [--only-downloads | --only-uploads] [SHOWOPTIONS]`
<pre>
If executed without option it will list the first 10 transfers
Options:
 -c (TAG|-a)	Cancel transfer with TAG (or all with -a)
 -p (TAG|-a)	Pause transfer with TAG (or all with -a)
 -r (TAG|-a)	Resume transfer with TAG (or all with -a)
 --only-uploads	Show/Operate only upload transfers
 --only-downloads	Show/Operate only download transfers

Show options:
 --summary	Prints summary of on going transfers
 --show-syncs	Show synchronization transfers
 --show-completed	Show completed transfers
 --only-completed	Show only completed download
 --limit=N	Show only first N transfers
 --path-display-size=N	Use at least N characters for displaying paths
 --col-separator=X	Uses the string "X" as column separator. Otherwise, spaces will be added between columns to align them.
 --output-cols=COLUMN_NAME_1,COLUMN_NAME2,...	Selects which columns to show and their order.

TYPE legend correspondence:
  ⇓ = 	Download transfer
  ⇑ = 	Upload transfer
  ⇵ = 	Sync transfer. The transfer is done in the context of a synchronization
  ⏫ = 	Backup transfer. The transfer is done in the context of a backup
</pre>

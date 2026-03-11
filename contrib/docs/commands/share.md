### share
Prints/Modifies the status of current shares

Usage: `share [-p] [-d|-a --with=user@email.com [--level=LEVEL]] [remotepath] [--use-pcre] [--time-format=FORMAT]`
<pre>
Options:
 --use-pcre	use PCRE expressions
 -p	Show pending shares too
 --with=email	Determines the email of the user to [no longer] share with
 -d	Stop sharing with the selected user
 -a	Adds a share (or modifies it if existing)
 --level=LEVEL	Level of access given to the user
              	0: Read access
              	1: Read and write
              	2: Full access
              	3: Owner access

If a remote path is given it'll be used to add/delete or in case
 of no option selected, it will display all the shares existing
 in the tree of that path

When sharing a folder with a user that is not a contact (see "mega-users --help")
  the share will be in a pending state. You can list pending shares with
 "share -p". He would need to accept your invitation (see "mega-ipc")

Sharing folders will require contact verification (see "mega-users --help-verify")

If someone has shared something with you, it will be listed as a root folder
 Use "mega-mount" to list folders shared with you
</pre>

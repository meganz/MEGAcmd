### users
List contacts

Usage: `users [-s] [-h] [-n] [-d contact@email] [--time-format=FORMAT] [--verify|--unverify contact@email.com] [--help-verify [contact@email.com]]`
<pre>
Options:
 -d	contact@email   	Deletes the specified contact.
--help-verify              	Prints general information regarding contact verification.
--help-verify contact@email	This will show credentials of both own user and contact
                           	 and instructions in order to proceed with the verifcation.
--verify contact@email     	Verifies contact@email.
                           	 CAVEAT: First you would need to manually ensure credentials match!
--unverify contact@email   	Sets contact@email as no longer verified. New shares with that user
                           	 will require verification.
Listing Options:
 -s	Show shared folders with listed contacts
 -h	Show all contacts (hidden, blocked, ...)
 -n	Show users names
 --time-format=FORMAT	show time in available formats. Examples:
               RFC2822:  Example: Fri, 06 Apr 2018 13:05:37 +0200
               ISO6081:  Example: 2018-04-06
               ISO6081_WITH_TIME:  Example: 2018-04-06T13:05:37
               SHORT:  Example: 06Apr2018 13:05:37
               SHORT_UTC:  Example: 06Apr2018 13:05:37
               CUSTOM. e.g: --time-format="%Y %b":  Example: 2018 Apr
                 You can use any strftime compliant format: http://www.cplusplus.com/reference/ctime/strftime/

Use "mega-invite" to send/remove invitations to other users
Use "mega-showpcr" to browse incoming/outgoing invitations
Use "mega-ipc" to manage invitations received
Use "mega-users" to see contacts
</pre>

# MEGA-WEBDAV - Serve you files as a WEBDAV server with MEGAcmd
This is a brief tutorial on how to configure [webdav](https://wikipedia.org/wiki/WebDAV) server.

Configuring a WEBDAV server will let you access your MEGA files as if they were located in your computer.
All major platforms support access to WEBDAV server. See [`Platform`](#platforms) usage.

Notice: the commands listed here assume you are using the interactive interaction mode: they are supposed to be executed within MEGAcmdShell.

## Serving a folder
Example: 
```
webdav /path/mega/folder
```

This will configure a WEBDAV server that will serve "myfolder". It'll show you the URL to access that path. You just use that location to configure access [according to your specific OS](#platforms).
Once you have it configured, you can browse, edit, copy and delete your files as if they were local file in your computer. 
Caveat: They are not local, MEGAcmd transparently download/upload decrypt/encrypt those files. 
Hence, throughput will be decreased as compared to accessing to local files. Be patient.

## Streaming
You can "webdav" a file, so as to offer streaming access to it:
```
webdav /path/to/myfile.mp4
```

You will receive an URL that you can use in your favourite video player.

## Streaming cache

Content fetched while streaming over webdav is cached on disk, so seeking backwards or replaying a
part you already watched is fast and does not download it again. Streaming over [ftp](FTP.md) does
not use it; that path is served through memory.

[FUSE mounts](FUSE.md) read and write the same cache, so the settings below bound both.

The cache belongs to the session. Logging out removes it; `logout --keep-session` and restarting the
MEGAcmd Server keep it.

Running `webdav` with no arguments reports how much the cache is using, under the list of served
locations.

### What streaming cannot check

When you download a file with `get`, MEGAcmd checks the whole file against the checksum held in its
key, and the download fails if they do not match. That check needs every byte, so it cannot be done
while streaming, which reads only the parts that are asked for. Streaming is still encrypted in
transit and decrypted with your key; what it cannot tell you is whether the bytes are the ones that
were uploaded.

This applies to webdav, [ftp](FTP.md) and [FUSE mounts](FUSE.md) alike. Before running or installing
something, fetch it with `get` and use that copy.

The cache is cleaned up automatically, and you can tune it through the `file_service_reclaim_*` keys
of the `configure` command. `configure --help` describes each key, and `help --streaming` explains
the mechanism from within MEGAcmd.

### How the cleanup works

A cleanup runs on a timer. The first one runs `file_service_reclaim_delay` seconds after login, or
after any `file_service_reclaim_*` value is changed, and further ones every
`file_service_reclaim_period` seconds.

Each cleanup does nothing at all unless the cache is above `file_service_reclaim_threshold` bytes.
When it is above, files that have gone unaccessed for at least `file_service_reclaim_age_threshold`
minutes are removed, least recently used first, until the cache is down to
`file_service_reclaim_target` bytes.

The defaults are the same ones MEGAsync uses: a cleanup every 2 hours, triggered above 10 GiB,
shrinking to 1 GiB, and only touching files unaccessed for 3 days.

A cleanup only drops content that is already in your cloud. A file you wrote through a FUSE mount is
uploaded first, and a file you are reading right now counts as accessed, so it is not touched.

### The threshold is not a hard limit

Nothing stops the cache growing past `file_service_reclaim_threshold`. It only decides whether the
next scheduled cleanup does any work, and a cleanup can only remove files that are already old
enough. So with the defaults, a long streaming session can hold much more than 10 GiB for as long as
it keeps reading, because none of that content has been idle for 3 days yet.

The value that actually limits how large the cache can get is `file_service_reclaim_age_threshold`.

### Keeping disk usage down

If you stream a lot and want the cache genuinely bounded, use a short age and a short period. For a
budget of about 2 GiB:

```
configure file_service_reclaim_age_threshold 10
configure file_service_reclaim_period 300
configure file_service_reclaim_threshold 2147483648
configure file_service_reclaim_target 536870912
```

That looks every 5 minutes and, above 2 GiB, drops back to 512 MiB everything untouched for the last
10 minutes. A file you are streaming right now counts as accessed, so it is never removed
mid-playback. The trade-off is that replaying something you watched a while ago downloads it again.

Values are kept per account and are reapplied on each login. They are cleared on logout, which
brings the defaults back.

To disable automatic cleanup entirely:

```
configure file_service_reclaim_threshold off
```

## Issues
We have detected some issues with different software, when trying to save a file into a webdav served locations. Typically with software that creates temporary files. 
We will keep on trying to circumvent those. 

In Linux, using gvfsd-dav (Gnome's default webdav client), we have occasionally seen problems trying to open text files that have already been modified using some graphic editors.
This is due to that gvfsd-dav tries to retrieve a URL different to the actual URL of the files. Reading the files through the console works just fine. This has been detected in Ubuntu 16.04.

In Windows XP, copying a file from a MEGA webdav location, and pasting in a local folder does nothing.

If you find any more issues, don't hesitate to write to support@mega.io, explaining what the problem is and how to reproduce it.

## Listing 

You can list the webdav served locations typing `webdav`:

```
WEBDAV SERVED LOCATIONS:                                                        
/path/mega/folder: http://127.0.0.1:4443/XXXXXXX/myfolder
/path/to/myfile.mp4: http://127.0.0.1:4443/YYYYYYY/myfile.mp4
```

These locations will be available as long as MEGAcmd is running. The configuration is persisted, and will be restored everytime you restart MEGAcmd

# Additional features/configurations

## Port & public server

When you serve your first location, a WEBDAV server is configured in port `4443`. 
You can change the port passing `--port=PORT` to your webdav command.
By default, the server is only accessible from the local machine. 
You can pass `--public` to your webdav command so as to allow remote access. 
In that case, use the IP of your server to access to it.

## HTTPS

Files in MEGA are encrypted, but you should bear in mind that the HTTP webdav server offers your files unencrypted. \
If you wish to add authenticity to your webdav server and integrity & privacy of the data transfered to/from the clients, 
you can secure it with [TLS](https://wikipedia.org/wiki/Transport_Layer_Security). 
You just need to pass `--tls` and the paths* to your certificate and key files (in PEM format):

```
webdav /path/mega/folder --tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key
```

*Those paths are local paths in your machine, not in MEGA.

Currently, MEGAcmd only supports one server: although you can serve different locations, only one configuration is possible. 
The configuration used will be the one on your first served location. 
If you want to change that configuration you will need to stop serving each and every path and start over.


## Stop serving

You can stop serving a MEGA location with:
```
webdav -d /path/mega/folder
```
If successfully, it will show a message indicating that the path is no longer served:
```
/path/mega/folder no longer served via webdav
```

## Platforms

All major platforms support accesing/mounting a webdav location. Here are some instructions to do that in Windows, Linux & Mac.

### Windows

This instructions refer to Windows 10, but they are similar in other windows.

Open an Explorer window, and then do right click on "This PC", and then "Map network drive...".

![webdavMenuWin.png](pics/webdavMenuWin.png?raw=true "webdavMenuWin.png")

Then enter the URL MEGAcmd gave you

![webdavConnectToServerWin.png](pics/webdavConnectToServerWin.png?raw=true "webdavConnectToServerWin.png")

Then, you should see the new location in the navigation panel now.

### Mac

Open Find and in the Menu "Go", select "Connect to Server", or type **&#x2318; - k**:

![webdavMenuMac.png](pics/webdavMenuMac.png?raw=true "webdavMenuMac.png")

Then enter the URL MEGAcmd gave you

![webdavConnectToServerMac.png](pics/webdavConnectToServerMac.png?raw=true "webdavConnectToServerMac.png")

At the moment of writing this tutorial, there is no authentication mechanisms, 
hence you don't need to worry about providing a user name/password. Just proceed if you are prompted with default options.
You should see the new location in the navigation panel now.

### Linux

This instructions are for Nautilus, it should be similar using another file browser. 
Click on File -> Connect to Server:

![webdavMenuLinux.png](pics/webdavMenuLinux.png?raw=true "webdavMenuLinux.png")

Then enter the URL MEGAcmd gave you

![webdavConnectToServerLinux.png](pics/webdavConnectToServerLinux.png?raw=true "webdavConnectToServerLinux.png")

You should see the new location in the navigation panel now.

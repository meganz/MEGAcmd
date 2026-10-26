
# MEGA FTP - Serve your files as  FTP server with MEGAcmd
This is a brief tutorial on how to configure an [ftp](https://en.wikipedia.org/wiki/File_Transfer_Protocol) server.

Configuring a FTP server will let you access your MEGA files as if they were located in your computer. All major platforms support access to FTP server. See [`Platform`](#platforms) usage.

Notice: the commands listed here assume you are using the interactive interaction mode: they are supposed to be executed within MEGAcmd shell.

## Serving a folder
**Example:** 
```
ftp /path/mega/folder
```

This will configure a FTP server that will serve "myfolder". It'll show you the URL to access that path. You just use that location to configure access [according to your specific OS](#platforms).

Once you have it configured, you can browse, edit, copy and delete your files using your favourite FTP client or add mount the network location and browse your files as if they were local file in your computer.

**Caveat**: They are not local - MEGAcmd transparently downloads/uploads decrypts/encrypts the files. Hence, throughput will be decreased as compared to accessing to local files. Be patient.

## Streaming
You can "ftp" a file, so as to offer streaming access to it:
```
ftp /path/to/myfile.mp4
```

You will receive an URL that you can use in your preferred video player.

## Issues
We have detected some issues with different software when trying to save a file into a ftp served locations, typically with software that creates temporary files.  We will keep on trying to circumvent those. 

In **FileZilla**, by default there is a 20 seconds timeout (Edit -> Preferences -> Connection) which will likely stop uploads before they are completed: once a upload is started, it begins to populate a temporary file. Once completed, the actual upload begins (the file gets transfered to MEGA), but no extra traffic is perceived by FileZilla during this time. If the upload takes longer than 20 seconds, it times out. You might want to increase that number for a correct behaviour.

In Linux, using **gvfsd-dav** (Gnome's default ftp client), we have seen problems when trying to reproduce videos within an FTP served location. Recommended alternative: Use VLC. Open it, and drag & drop the file into VLC window. This will open the FTP URL and start streaming you video.

If you find any more issues, don't hesitate to write to support@mega.io, explaining what the problem is and how to reproduce it.

## Listing 

You can list the ftp served locations by typing `ftp`:

**FTP SERVED LOCATIONS:**
```
/path/mega/folder: ftp://127.0.0.1:4990/XXXXXXX/myfolder
/path/to/myfile.mp4: ftp://127.0.0.1:4990/YYYYYYY/myfile.mp4
```

These locations will be available as long as MEGAcmd is running. The configuration is persisted, and will be restored every time you restart MEGAcmd

# Additional features/configurations

## Port & public server

When you serve your first location, a FTP server is configured in port `4990`.  You can change the port passing `--port=PORT` to your ftp command.

Currently *only passive mode* is available. If your client does not work with passive FTP mode, it won't work.

Data connections will work in the range of ports 1500 to 1600 by default.
You can change those passing `--data-port=BEGIN-END`
By default, the server is only accessible from the local machine. 

You can pass `--public` to your ftp command so as to allow remote access. In that case, use the IP address of your server to access to it.

## FTPS

Files in MEGA are encrypted, but you should bear in mind that the bare FTP server offers your files unencrypted.

If you wish to add authenticity to your ftp server and integrity & privacy of the data transferred to/from the clients, you can secure it with [TLS](https://wikipedia.org/wiki/Transport_Layer_Security). Notice that FTPs is not the same as [SFTP](https://es.wikipedia.org/wiki/SSH_File_Transfer_Protocol), which is a completely different and currently unsupported protocol.

To serve via FTPS, you just need to pass `--tls` and the paths to your certificate and key files (in PEM format):

```
ftp /path/mega/folder --tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key
```

*Paths are local paths in your machine, not in MEGA.*

Currently, MEGAcmd only supports one server: although you can serve different locations, only one configuration is possible. The configuration used will be the one on your first served location. If you want to change that configuration you will need to stop serving each and every path and start over.

## Stop serving

You can stop serving a MEGA location with:
```
ftp -d /path/mega/folder
```
If successful, it will show a message indicating that the path is no longer served:
```
/path/mega/folder no longer served via ftp
```

## Platforms

All major platforms support accessing/mounting a ftp location. Here are some instructions to do that in Windows, Linux & Mac.

### Windows

These instructions refer to Windows 10, but they are similar in other Windows versions.

Open an Explorer window, and then do right click on "This PC", and then "Add a network location...".

![ftpMenuWin.png](pics/ftpMenuWin.png?raw=true "ftpMenuWin.png")

Then enter the URL MEGAcmd gave you

![ftpConnectToServerWin.png](pics/ftpConnectToServerWin.png?raw=true "ftpConnectToServerWin.png")

And select "Log on Anonymously"

![ftpConnectToServerWin2.png](pics/ftpConnectToServerWin2.png?raw=true "ftpConnectToServerWin2.png")

Then, you should see the new location in the navigation panel now. Windows FTP client does not allow writing, if you wish to modify your files you should either use an alternative client (like FileZilla), or perhaps serve your files using WEBDAV instead of FTP.


### Mac

Open Find and in the Menu "Go", select "Connect to Server", or type **&#x2318; - k**:

![webdavMenuMac.png](pics/webdavMenuMac.png?raw=true "webdavMenuMac.png")

Then enter the URL MEGAcmd gave you

![ftpConnectToServerMac.png](pics/ftpConnectToServerMac.png?raw=true "ftpConnectToServerMac.png")

At the moment of writing this tutorial, there is no authentication mechanisms, hence you don't need to worry about providing a user name/password. Just proceed if you are prompted with default options. You should see the new location in the navigation panel now.

### Linux

This instructions are for Nautilus, it should be similar using another file browser. Notice that `gvfs` currently does not support FTPs

Click on File -> Connect to Server:

![webdavMenuLinux.png](pics/webdavMenuLinux.png?raw=true "webdavMenuLinux.png")

Then enter the URL MEGAcmd gave you

![ftpConnectToServerLinux.png](pics/ftpConnectToServerLinux.png?raw=true "ftpConnectToServerLinux.png")

You should see the new location in the navigation panel now. 

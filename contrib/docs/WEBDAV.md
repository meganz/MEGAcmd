# MEGA-WEBDAV - Serve you files as a WEBDAV server with MEGAcmd
This is a brief tutorial on how to configure [webdav](https://wikipedia.org/wiki/WebDAV) server.

Configuring a WEBDAV server will let you access your MEGA files as if they were located in your computer.
All major platforms support access to WEBDAV server. See [`Platform`](#platforms) usage.

## Serving a folder
Example: 
```
webdav /path/to/myfolder
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

## Issues
We have detected some issues with different software, when trying to save a file into a webdav served locations. Typically with software that creates temporary files. 
We will keep on trying to circumvent those. 

In Linux, using gvfsd-dav (Gnome's default webdav client), we have ocasionally seen problems trying to open text files that have already been modified using some graphic editors.
This is due to that gvfsd-dav tries to retrieve a URL different to the actual URL of the files. Reading the files through the console works just fine. This has been detected in Ubuntu 16.04.

In Windows XP, copying a file from a MEGA webdav location, and pasting in a local folder does nothing.

If you find any more issues, don't hesitate to write to support@mega.nz, explaining what the problem is and how to reproduce it.

## Listing 

You can list the webdav served locations typing `webdav`:

```
WEBDAV SERVED LOCATIONS:                                                        
/path/to/myfolder: http://127.0.0.1:4443/XXXXXXX/myfolder
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
If you wish to add authenticity to your webdav server and integrity & privacy of the data trasnfered to/from the clients, 
you can secure it with [TLS](https://wikipedia.org/wiki/Transport_Layer_Security). 
You just need to pass `--tls` and the paths* to your certificate and key files (in PEM format):

```
webdav /path/to/myfolder --tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key
```

*Those paths are local paths in your machine, not in MEGA.

Currently, MEGAcmd only supports one server: although you can serve different locations, only one configuration is possible. 
The configuration used will be the one on your first served location. 
If you want to change that configuration you will need to stop serving each and every path and start over.


## Stop serving

You can stop serving a MEGA location with:
```
webdav -d /path/to/myfolder
```
If successfully, it will show a message indicating that the path is no longer served:
```
/path/to/myfolder no longer served via webdav                                  
```

## Platforms

We will include contents for this section soon

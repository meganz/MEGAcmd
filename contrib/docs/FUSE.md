# MEGA-FUSE - Serve you files as a Filesystem in User SpacE server with MEGAcmd

This is a brief tutorial on how to configure [fuse](https://wikipedia.org/wiki/WebDAV) mount point.

Currently this is only supported for Linux.

Configuring a FUSE mount will let you access your MEGA files as if they were located in your computer. After configuring a FUSE mount, you can use your favourite tools to browse, play and edit your MEGA files.

Notice: the commands listed here assume you are using the interactive interaction mode: they are supposed to be executed within MEGAcmd Shell.

## Local cache

MEGAcmd will use a cache to place files while working with mount points. Such cache will be used to store both files downloaded from MEGA and files being uploaded to MEGA. These will be created under the hood in a cache folder.

FUSE cache is locate at .... TODO

Bear in mind that this cache is fundamental to be able to work with FUSE mounts.

Cache files are removed automatically. Restarting MEGAcmd Server may help reduce the space used by the cache.

### Important caveats

Files/folders created within your local computer may not be immediately available in your Cloud. The mount engine will transfer your files in the background and create folders in your cloud. This will work transparantly. But its important to
be aware that MEGAcmd Server need to be running in order for these actions to complete.


## Streaming

Currently, streaming is not directly supported. In order to open a file within a FUSE mount point, the file need to be downloaded completely within the local cache folder. You will need space in your hard-drive to accomodate for these files.


## Usage


### Creating a new mount point
....

### Disable/enable

### Adjusting configuration

### Remove


## Issues

Occasionally you may encounter a situation in which there is a FUSE mount configured. e.g: if MEGAcmd Server is closed abruptly. Consider using fusermount command to disable the mounting:
```
fusermount -u /local/path/to/use/mountpoint
```

Adding a `-z` may help if the above fails. See ... TODO: link man page.


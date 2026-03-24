### ftp
Configures a FTP server to serve a location in MEGA

Usage: `ftp [-d ( --all | remotepath ) ] [ remotepath [--port=PORT] [--data-ports=BEGIN-END] [--public] [--tls --certificate=/path/to/certificate.pem --key=/path/to/certificate.key]] [--use-pcre]`
<pre>
This can also be used for streaming files. The server will be running as long as MEGAcmd Server is.
If no argument is given, it will list the ftp enabled locations.

Options:
 --d        	Stops serving that location
 --all      	When used with -d, stops serving all locations (and stops the server)
 --public   	*Allow access from outside localhost
 --port=PORT	*Port to serve. DEFAULT=4990
 --data-ports=BEGIN-END	*Ports range used for data channel (in passive mode). DEFAULT=1500-1600
 --tls      	*Serve with TLS (FTPs)
 --certificate=/path/to/certificate.pem	*Path to PEM formatted certificate
 --key=/path/to/certificate.key	*Path to PEM formatted key
 --use-pcre	use PCRE expressions

*If you serve more than one location, these parameters will be ignored and used those of the first location served.
 If you want to change those parameters, you need to stop serving all locations and configure them again.
Note: FTP settings and locations will be saved for the next time you open MEGAcmd, but will be removed if you logout.

Caveat: This functionality is in BETA state. It might not be available on all platforms. If you experience any issue with this, please contact: support@mega.nz
</pre>

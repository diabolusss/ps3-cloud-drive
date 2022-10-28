# PS3-Cloud-Drive-Mod
Based on ps3-cloud-drive by mhaqs.

I created this version, because version 1.1.7 by mhaqs was no longer working for me.

1. You will need the latest ps3toolchain to compile this.
2. You will need to create a project and enable the Google Drive API using the Google API console. 
	- Enter the client_id and client_secret into the main.cpp file before compilation.

Changes:
- Removed the need for an SSL certificate.
- Changed the old Google OAuth2 URLs to the new URLs.


For more info on ps3-cloud-drive itself, check its [original readme](README_Original.md).

Note
Installing Automatically Converted CA Certificates from mozilla.org
 Place Mozilla [cacert.pem](https://curl.haxx.se/ca/cacert.pem) into apps USERDIR.

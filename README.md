# sota-com
Communication core for SOTA - Software update Over The Air. This project has 3 main packs
1. server - the PC side of the SOTA
2. client - the vehicle side of the SOTA
3. common - the classes that are common to both


## Pre-requisites / dependencies

1. JANSSON - C library for processing json objects / files. Refer http://www.digip.org/jansson/ for more details

2. MYSQL - This is required only on server. Install it on the system and create an user named "sota" with all privileges. 
	sudo apt-get install mysql-server
	sudo apt-get install libmysqlclient-dev
	search google on how to create user named "sota"

3. XDELTA3 - This is the diff / patch tool used to generate diff before download. 

4. Apache2 & PHP for the WEB Interface - Install the following on a ubuntu system. And delete /var/www/html/index.html and copy the files in web/ directory of this project to /var/www/html/ and ensure the apache2 is configured to load the index.php file by default.
	sudo apt-get install apache2
	sudo apt-get install php5 libapache2-mod-php5
	sudo apt-get install php5-mysql 

## Design

Please check Visteon_SOTA_HLD.pdf for more details.


## To Build

Please pull my yocto recipes in meta-aananth (https://gitlab.com/aananthcn/meta-aananth) layer and add this to your bblayer.conf and type the following <br>

<b>bitbake rpi-sota-image</b> - to build an SD Card image<br>
<b>bitbake sota-com sota-updater</b> - to build individual images<br>

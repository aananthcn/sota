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

## Design

Please check Visteon_SOTA_HLD.pdf for more details.

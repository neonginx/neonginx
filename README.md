[![N|Solid](https://github.com/neonginx/neonginx/raw/master/logo.png)](https://www.neonginx.com/)

[![Build Status](https://travis-ci.com/neonginx/neonginx.svg?branch=master)](https://travis-ci.com/neonginx/neonginx)

# NeoNginx
NEONGINX is a web server based on the Nginx HTTP server and has many advanced features.

## Dependencies
Ubuntu/Debian distro:

	apt-get install libssl-dev libpcre3 libpcre3-dev zlib1g zlib1g-dev libgeoip-dev libgeoip1 geoip-bin perl libperl-dev libgd3 libgd-dev libxml2 libxml2-dev -y

## Installation

	git clone https://github.com/neonginx/neonginx
	cd neonginx
	cp /etc/nginx/ /root/nginx_folder_backup # backup your nginx folder
	rm -rf /etc/nginx/ # delete your old nginx folder to initialize nginx correctly
	./AUTOINSTALL
Done!
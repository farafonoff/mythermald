thermald: thermald.cpp
	g++ -std=c++11 thermald.cpp ini.c -I. -o thermald

release: thermald.cpp
	g++ -O2 -std=c++11 thermald.cpp ini.c -I. -o thermald

install:
	cp thermald.conf /etc/thermald.conf
	cp thermald /usr/bin/thermald

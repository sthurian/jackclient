CC=/usr/bin/g++
CFLAGS=--std=c++14 -pthread -Os -Wall
JACKFLAGS=`pkg-config --cflags --libs jack`
TARGETS=main.cpp jackclient/jackclient.cpp
all :
	$(CC) $(CFLAGS) $(TARGETS) $(JACKFLAGS) -o example_client

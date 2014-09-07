TYPE = exe
SOURCES = httpserv.cpp
INCLUDES = -I. -I/usr/local/include -I../include
INSLIBS = -L/usr/lib/x86_64-linux-gnu -levent -lrt
OUT = httpserv

#-----------------------------------------------------------------
include ../build.mk


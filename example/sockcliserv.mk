TYPE = exe
SOURCES = sockcliserv.cpp
INCLUDES = -I. -I/usr/local/include -I../include
INSLIBS = -L/usr/lib/x86_64-linux-gnu -levent -lrt
OUT = sockcliserv

#-----------------------------------------------------------------
include ../build.mk


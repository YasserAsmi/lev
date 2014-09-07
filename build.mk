# Copyright (c) 2014 Yasser Asmi
# Released under the MIT License (http://opensource.org/licenses/MIT)
#
# build.mk:
# 	CONFIG    Build config (debug, release)
# 	TYPE      Type of project (lib, exe)
# 	SOURCES   Names of cpp files; ex: str.cpp util.cpp
# 	INCLUDES  Include directories with -I compiler option; ex: -I../include -I/usr/local/include
# 	INSLIBS   Libs to be linked specified with compiler options -l or -L; ex: -lpthread -L/usr/lib/x86_64-linux-gnu
#   LIBS      Project libs to be linked--paths to .a files; ex: libRegExp.a ../lib/libtest.a
# 	MKDIRS    Names of dirs to create; ex: ../lib
# 	OUT       Name of the output file; ex: ../lib/libtest.a
#	DIRS      Names of directories with makefiles to build; ex: src test
#   EXTMAKES  Names of makefiles to build; ex: proja.mk projb.mk
#
#   $(PWD)    Current directory (ie with current makefile)
#
#   make          -- builds
#   make -B       -- builds even if up to date
#   make cleanall -- cleans
#   make clean    -- cleans current folder/makefile only
#   make xx._s    -- generates assembly/listing file for xx.cpp
#   make xx._p    -- generates preprocessed file for xx.cpp

CONFIG ?= debug
TYPE ?= exe
INCLUDES ?= -I/usr/local/include

OBJS = $(SOURCES:.cpp=.o)
PWD = $(shell pwd)

ifeq ($(CONFIG)	,debug)
CXXFLAGS += -g -Wall
else
CXXFLAGS += -O3 -Wall -DNDEBUG
endif

all: dirs extmakes cfgcheck mkdirs $(OUT)

.PHONY: clean cleanall all $(DIRS) $(MKDIRS) $(EXTMAKES)

cfgcheck:
ifneq ($(CONFIG)	,release)
ifneq ($(CONFIG)	,debug)
	@echo "Error: Invalid CONFIG 	'$(CONFIG)	'' (options: debug,release)"
	@exit 1
endif
endif
	@echo "Making '$(PWD)' CONFIG="$(CONFIG)

$(OUT): $(OBJS) $(LIBS)
	echo $(LIBS)
ifeq ($(TYPE),lib)
	$(AR) rcs $(OUT) $(OBJS)
endif
ifeq ($(TYPE),exe)
	$(CXX) -o $@ $^ ${LDFLAGS} $(LIBS) $(INSLIBS)
endif

-include $(OBJS:.o=._d)

%.o: %.cpp
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $(PWD)/$*.cpp -o $*.o
	$(CXX) -MM $(INCLUDES) $(CXXFLAGS) $*.cpp > $*._d
	@cp -f $*._d $*._d.tmp
	@sed -e 's/.*://' -e 's/\\$$//' < $*._d.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> $*._d
	@rm -f $*._d.tmp

%._p: %.cpp
	$(CXX) -E -P $(INCLUDES) $(CXXFLAGS) $(PWD)/$*.cpp -o $*._p

%._s: %.cpp
	$(CXX) -c -Wa,-adhln -g $(INCLUDES) $(CXXFLAGS) $*.cpp > $*._s

dirs: $(DIRS)

extmakes: $(EXTMAKES)

mkdirs: $(MKDIRS)

clean:
	rm -f $(OUT) *.o *._d *._p *._s

cleanall: TARG:=cleanall
cleanall: $(DIRS) $(EXTMAKES)
	rm -f $(OUT) *.o *._d *._p *._s

$(DIRS):
	@$(MAKE) -C $@ $(TARG)

$(EXTMAKES):
	@$(MAKE) -f $@ $(TARG)

$(MKDIRS):
	@mkdir -p $@
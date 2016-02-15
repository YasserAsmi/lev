# Copyright (c) 2014 Yasser Asmi
# Released under the MIT License (http://opensource.org/licenses/MIT)
#
# build.mk:
# 	CONFIG    Build config (debug, opttest, release)
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
#   make cleanall -- cleans all DIRS
#   make clean    -- cleans current folder/makefile only
#   make xx._s    -- generates assembly/listing file for xx.cpp
#   make xx._p    -- generates preprocessed file for xx.cpp

CONFIG ?= debug
TYPE ?= exe
INCLUDES ?= -I/usr/local/include

OBJS = $(patsubst %.cpp,oo/%.o,$(SOURCES))

PWD = $(shell pwd)

ifeq ($(CONFIG)	,debug)
CXXFLAGS += -g -Wall
else
ifeq ($(CONFIG)	,opttest)
CXXFLAGS += -g -O3 -Wall
else
CXXFLAGS += -O3 -Wall -DNDEBUG
endif
endif

all: dirs extmakes cfgcheck mkdirs $(OUT)

.PHONY: clean cleanall all $(DIRS) $(MKDIRS) $(EXTMAKES)

cfgcheck:
ifneq ($(CONFIG)	,release)
ifneq ($(CONFIG)	,debug)
ifneq ($(CONFIG)	,opttest)
	@echo "Error: Invalid CONFIG '$(CONFIG)	'' (options: debug, opttest, release)"
	@exit 1
endif
endif
endif
	@echo "Making '$(PWD)' CONFIG="$(CONFIG)

$(OUT): $(OBJS) $(LIBS)
	@echo $(LIBS)
ifeq ($(TYPE),lib)
	$(AR) rcs $(OUT) $(OBJS)
endif
ifeq ($(TYPE),exe)
	$(CXX) -o $@ $^ ${LDFLAGS} $(LIBS) $(INSLIBS)
endif

-include $(OBJS:.o=.i)

oo/%.o: %.cpp
	@mkdir -p oo
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $(PWD)/$*.cpp -o oo/$*.o
	@$(CXX) -MM $(INCLUDES) $(CXXFLAGS) $*.cpp > oo/$*.i.tmp
	@echo -n "oo/" > oo/$*.i
	@cat oo/$*.i.tmp >> oo/$*.i
	@sed -e 's/.*://' -e 's/\\$$//' < oo/$*.i.tmp | fmt -1 | sed -e 's/^ *//' -e 's/$$/:/' >> oo/$*.i
	@rm -f oo/$*.i.tmp

%._p: %.cpp
	@mkdir -p oo
	$(CXX) -E -P $(INCLUDES) $(CXXFLAGS) $(PWD)/$*.cpp -o oo/$*._p
	@echo "Created preprocessor output: oo/"$*._s

%._s: %.cpp
	@mkdir -p oo
	$(CXX) -c -Wa,-adhln -g $(INCLUDES) $(CXXFLAGS) $*.cpp > oo/$*._s
	@echo "Created assembly output: oo/"$*._s

dirs: $(DIRS)

extmakes: $(EXTMAKES)

mkdirs: $(MKDIRS)

clean:
	rm -f $(OUT) oo/*.o oo/*.i oo/*._p oo/*._s
	rmdir oo

cleanall: TARG:=cleanall
cleanall: $(DIRS) $(EXTMAKES)
	rm -f $(OUT) oo/*.o oo/*.i oo/*._p oo/*._s
	rmdir oo

$(DIRS):
	@$(MAKE) -C $@ $(TARG)

$(EXTMAKES):
	@$(MAKE) -f $@ $(TARG)

$(MKDIRS):
	@mkdir -p $@
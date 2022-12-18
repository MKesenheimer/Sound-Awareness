########################################################################
#                          -*- Makefile -*-                            #
########################################################################
# Force rebuild on these rules
.PHONY: all libs clean clean-libs
.DEFAULT_GOAL := sound-awareness

COMPILER = g++

########################################################################
## Flags
FLAGS   = -g -std=c++17
#FLAGS   = -g -std=c++17
## find shared libraries during runtime: set rpath:
LDFLAGS = -rpath @executable_path/libs
PREPRO  =
##verbose level 1
#DEBUG   = -D DEBUGV1
##verbose level 2
#DEBUG  += -D DEBUGV2
##verbose level 3
#DEBUG  += -D DEBUGV3
OPT     = -O2
WARN    = -Wall -Wno-missing-braces

### generate directory obj, if not yet existing
$(shell mkdir -p build)

########################################################################
## Paths, modify if necessary
WORKINGDIR = $(shell pwd)
PARENTDIR  = $(WORKINGDIR)/..
SYSTEMINC  = /opt/local/include
LIBS       = $(WORKINGDIR)/libs
GTEST      = $(LIBS)/googletest
LIBGTEST   = $(LIBS)/googletest/build/lib

########################################################################
### DO NOT MODIFY BELOW THIS LINE ######################################
########################################################################

########################################################################
## search for the files and set paths
vpath %.cpp $(WORKINGDIR) $(WORKINGDIR)/GameLibrary $(WORKINGDIR)/unittests
vpath %.m $(WORKINGDIR)
vpath %.a $(WORKINGDIR)/build
vpath %.o $(WORKINGDIR)/build

########################################################################
## Includes
CXX  = $(COMPILER) $(FLAGS) $(OPT) $(WARN) $(DEBUG) $(PREPRO) -I$(SYSTEMINC) -I$(WORKINGDIR) -I$(LIBS) -I$(EIGEN) -I$(GTEST)/googletest/include
INCLUDE = $(wildcard *.h $(UINCLUDE)/*.h)

########################################################################
## libraries
### SDL
CXX += $(shell sdl2-config --cflags)
LDFLAGS += $(shell sdl2-config --libs)

# Frameworks
# -framework SDL_gfx 
#FRM = -framework Cocoa

### Unittests
LDFLAGS_U = $(LDFLAGS)
LDFLAGS_U += -L$(LIBGTEST) -lgtest

########################################################################
## Build rules
%.a: %.cpp $(INCLUDE)
	$(CXX) -c -o build/$@ $<

%.a: %.m $(INCLUDE)
	$(CXX) -c -o build/$@ $<

########################################################################
## BUILD Files
BUILD = Main.a

## BUILD files for unittests
BUILD_U = 


########################################################################
## Rules
## type make -j4 [rule] to speed up the compilation
all: libs sound-awareness gtest

sound-awareness: $(BUILD)
	$(CXX) $(patsubst %,build/%,$(BUILD)) $(LDFLAGS) $(FRM) -o $@

gtest: $(BUILD_U)
	$(CXX) $(patsubst %,build/%,$(BUILD_U)) $(LDFLAGS_U) -o $@

libs:
	cd $(GTEST) && mkdir -p $(GTEST)/build && cd $(GTEST)/build && \
	cmake -DBUILD_SHARED_LIBS=ON .. && make
	cp $(LIBGTEST)/* $(LIBS)
	cd $(LIBLUMAX) && ./make.sh
	cp $(LIBLUMAX)/*.dylib $(LIBS)

clean-all: clean clean-libs

clean:
	rm -f build/*.a sound-awareness gtest

clean-libs:
	cd $(GTEST) && rm -rf build 
	cd $(LIBLUMAX) && rm -f *.dylib *.so
	rm -f $(LIBS)/*.dylib $(LIBS)/*.so

do:
	make && ./sound-awareness

########################################################################
#                       -*- End of Makefile -*-                        #
########################################################################

#
# Daniel Shervheim © 2020
# https://www.danielshervheim.com/
#

CXX = gcc

LIBS = `pkg-config --cflags --libs gtk+-3.0` -rdynamic -lm

# Append correct GL library based on linux vs osx.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LIBS += -framework OpenGL
else
	LIBS += -lGL
endif

LIBDIRS = -I dependencies -I data/gresource/compiled
# CXXFLAGS = -Wall $(LIBS) $(LIBDIRS)
CXXFLAGS = -w $(LIBS) $(LIBDIRS)
BIN = tinypaint



# DEFAULT target

all: $(BIN)



# GRESOURCE rules
# compiles the icons and glade files into a .c and .h file to be linked in at compile time

FILE = data/gresource/$(BIN).gresource.xml
SOURCE = --sourcedir data/glade/ --sourcedir data/icons/
TARGET = --c-name $(BIN) --target data/gresource/compiled/$(BIN)_gresource

data/gresource/compiled:
	mkdir -p data/gresource/compiled

compile_resources: data/gresource/compiled
	glib-compile-resources $(SOURCE) --generate-source $(TARGET).c $(FILE)
	glib-compile-resources $(SOURCE) --generate-header $(TARGET).h $(FILE)

clean_resources:
	rm -rf data/gresource/compiled



# DEPENDENCY rules
# clones the necessary dependencies from github

dependencies:
	mkdir -p dependencies

get_dependencies: dependencies
	git clone https://github.com/lvandeve/lodepng.git dependencies;\
	mv dependencies/lodepng.cpp dependencies/lodepng.c

clean_dependencies:
	rm -rf dependencies



# APPLICATION COMPILATION rules
# compiles and links the source code into a finished executable

build:
	mkdir -p build

$(BIN): build src/*.c data/gresource/compiled/*.c dependencies/*.c
	$(CXX) -o build/$(BIN) src/*.c data/gresource/compiled/*.c dependencies/*.c $(CXXFLAGS)

clean_build:
	rm -rf build



# CLEAN rules
# cleans all the build, dep, resourec files

clean: clean_build clean_dependencies clean_resources



# INSTALL rules
# I removed these since they were extremely specific to Ubuntu linux.
# You can still run the program by compiling and running ./build/tinypaint



# STATS rules
# prints out stats about the project

src_stats:
	cd src; ls | xargs wc -l; cd ..

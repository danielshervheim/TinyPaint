#
# Daniel Shervheim © 2019
# danielshervheim@gmail.com
# www.github.com/danielshervheim
#

CXX = gcc

LIBS = `pkg-config --cflags --libs gtk+-3.0` -rdynamic -lm -lGL
LIBDIRS = -I dependencies -I data/gresource/compiled
CXXFLAGS = -Wall $(LIBS) $(LIBDIRS)
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



# INSTALLATION rules
# copies the executable and .desktop file to their respective destinations
# (this target most likely will need to be run as sudo)

install: $(BIN)
	cp build/$(BIN) /usr/bin/
	cp data/desktop/TinyPaint.desktop /usr/share/applications/

uninstall:
	rm -rf /usr/bin/$(BIN)
	rm -rf /usr/share/applications/TinyPaint.desktop

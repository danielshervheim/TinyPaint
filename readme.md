# TinyPaint

TinyPaint is a lightweight painting and image editing application for Unix, written in C. (The compiled executable is less than 350 kilobytes)!

I wrote it as an exercise in UI programming, and to better familiarize myself with the Gtk, Gdk, and Glib libraries.

Currently it features a multitude of brushes, filters, undo/redo, and saving/loading images in png format.

![TinyPaint screenshot](https://i.imgur.com/CrieDeR.png)

## Dependencies

TinyPaint requires the following packages be installed

- libgtk-3-dev
- libglu1-mesa-dev
- mesa-common-dev

TinyPaint also depends on the [lodepng](https://lodev.org/lodepng/) library, by github user [lvandeve](https://github.com/lvandeve/lodepng). There is a rule in the makefile provided to automatically clone lodepng into the directory that TinyPaint expects. Just run `$ make get_dependencies`.


## Compiling

#### build

TinyPaint expects the lodepng library to be in a specific folder. It also uses the Glib Gresource library to compile the icons and ui definitions into linkable source code. Therefore, **the order you run the make targets in is important**!

`make get_dependencies` and `make compile_resources` **must** be run before `make`!

```
$ git clone XXX
$ cd tinypaint
$ make get_dependencies
$ make compile_resources
$ make
$ ./build/tinypaint
```

#### install

(build first)

```
$ sudo make install
```

#### uninstall

(install first)

```
$ sudo make uninstall
```


## To-do

##### Update OpenGL drawing method
Currently, TinyPaint uses the rather archaic `glDrawPixels()` function for drawing the pixels to the screen via OpenGL. Eventually I would like to update this to the more modern textured quad, fragment/vertex shader approach.

##### Improve filter application times
I would also like to implement multi-threading during filter application. The convolution filters especially would benefit from some parallelization I think.

##### Make `kernel` a struct type
Include the radius and edgeLength as members, and make `kernel_new`, `kernel_destroy`, and `kernel_set_value(x, y)` methods.

##### General cleanup
Simplify messy code, clarify variable names, write more thorough comments, etc etc

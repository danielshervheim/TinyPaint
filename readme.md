# TinyPaint

TinyPaint is a lightweight painting and image editing application for Unix, written in C. (The compiled executable is less than 350 kilobytes)!

I wrote it as an exercise in UI programming, and to better familiarize myself with the Gtk, Gdk, and Glib libraries.

Currently it features a multitude of brushes, filters, undo/redo, and saving/loading images in png format.

![TinyPaint screenshot](https://i.imgur.com/CrieDeR.png)

## Contents

- [Features](#features)
    - [Brushes](#brushes)
    - [Filters](#filters)
    - [Keyboard Shortcuts](#keyboardshortcuts)
- [Dependencies](#dependencies)
- [Compiling](#compiling)
    - [Building](#building)
    - [Installing](#installing)
    - [Uninstalling](#uninstalling)
- [Todo](#todo)

<a name="features"></a>
## Features

<a name="brushes"></a>
#### Brushes

Pen
![pen](https://i.imgur.com/YcIyjX2.png)

Brush
![brush](https://i.imgur.com/Ly2U1h3.png)

Marker
![marker](https://i.imgur.com/JSwkGIX.png)

Airbrush
![airbrush](https://i.imgur.com/mUJrzzZ.png)

Flood Fill
![flood fill](https://i.imgur.com/pY4mipe.png)

Eraser
![eraser](https://i.imgur.com/y9dNTjl.png)

<a name="filters"></a>
#### Filters

Saturation
![saturation](https://i.imgur.com/a490B7Z.png)

Channels
![channels](https://i.imgur.com/NQqbb99.png)

Invert
![invert](https://i.imgur.com/jjtbpCh.png)

Brightness / Contrast
![brightnesscontrast](https://i.imgur.com/VY9Oort.png)

Gaussian Blur
![gaussianblur](https://i.imgur.com/I4ZUrcv.png)

Motion Blur
![motionblur](https://i.imgur.com/dJBZ5pN.png)

Sharpen
![sharpen](https://i.imgur.com/grHOXWD.png)

Edge Detection
![edgedetection](https://i.imgur.com/BlzKl5T.png)

Posterize
![posterize](https://i.imgur.com/quzt1qY.png)

Threshold
![threshold](https://i.imgur.com/aFZBgSx.png)

<a name="keyboardshortcuts"></a>
#### Keyboard Shortcuts

**New Image**: Ctrl+N

**Open Image**: Ctrl+O

**Save Image**: Ctrl+S

**Quit**: Ctrl+Q

**Undo**: Ctrl+Z

**Redo**: Ctrl+Shift+Z

<a name="dependencies"></a>
## Dependencies

TinyPaint requires the following packages be installed

- libgtk-3-dev
- libglu1-mesa-dev
- mesa-common-dev

TinyPaint also depends on the [lodepng](https://lodev.org/lodepng/) library, by github user [lvandeve](https://github.com/lvandeve/lodepng). There is a rule in the makefile provided to automatically clone lodepng into the directory that TinyPaint expects. Just run `$ make get_dependencies`.

<a name="compiling"></a>
## Compiling

<a name="building"></a>
#### Building

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

<a name="installing"></a>
#### Installing

(build first)

```
$ sudo make install
```

<a name="uninstalling"></a>
#### Uninstalling

(install first)

```
$ sudo make uninstall
```

*(Note: if you just run TinyPaint (i.e. via `$ ./build/tinypaint`), then the New and Open buttons will cause the program to crash. This is because TinyPaint `fork`s a new process for each new file/open file intention, and then `exec`s the tinypaint executable from the install directory, hence it must be installed to work).*

<a name="todo"></a>
## To-do

##### Update OpenGL drawing method
Currently, TinyPaint uses the rather archaic `glDrawPixels()` function for drawing the pixels to the screen via OpenGL. Eventually I would like to update this to the more modern textured quad/fragment/vertex shader approach.

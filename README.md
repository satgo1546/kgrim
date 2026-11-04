kgrim
=====

Grab images from KWin Wayland compositor. A drop-in replacement for [grim](https://gitlab.freedesktop.org/emersion/grim) on KDE Plasma.

## Install

Requires Qt6 and CMake.

```sh
cmake -B build
cmake --build build
cmake --install build
```

## Usage

Take a screenshot of the entire workspace and save it in `$GRIM_DEFAULT_DIR` or the pictures directory:

```sh
kgrim
```

Take a screenshot of a specific output:

```sh
kgrim -o DP-1
```

Take a screenshot of a region:

```sh
kgrim -g "10,20 300x400"
```

Select a region with [slurp](https://github.com/emersion/slurp) and take a screenshot of it:

```sh
kgrim -g "$(slurp)"
```

Name the image as the number of seconds since 1970:

```sh
kgrim $(xdg-user-dir PICTURES)/$(date +%s).png
```

Take a screenshot and copy it to clipboard:

```sh
kgrim - | wl-copy
```

Take a screenshot of the focused monitor:

```sh
kgrim -o ""
```

Take a screenshot of the focused window:

```sh
kgrim -T ""
```

Pick a color, using slurp and ImageMagick:

```sh
kgrim -g "$(slurp -p)" -t ppm - | convert - -format '%[pixel:p{0,0}]' txt:-
```

## How it works

kgrim uses the same D-Bus interface that Spectacle [calls](https://invent.kde.org/plasma/spectacle/-/blob/v6.8.2/src/Platforms/ImagePlatformKWin.cpp).

## License

kgrim is distributed under the X11 license.

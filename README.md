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

There are no packages for distributions currently.
Contributions are welcome.

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

Select a window and take a screenshot of it:

```sh
kgrim -T "$(qdbus org.kde.KWin /KWin org.kde.KWin.queryWindowInfo | sed -n 's/^uuid: //p')"
```

Pick a color, using slurp and ImageMagick:

```sh
kgrim -g "$(slurp -p)" -t ppm - | convert - -format '%[pixel:p{0,0}]' txt:-
```

Note the use of PPM,
a simple, uncompressed format.
PPM is more efficient than PNG in a pipeline
where compression between stages is only a waste of time.

## Notes

### Implementation

kgrim uses the same
[D-Bus interface](https://invent.kde.org/plasma/kwin/-/blob/v6.8.2/src/plugins/screenshot/org.kde.KWin.ScreenShot2.xml)
that Spectacle
[calls](https://invent.kde.org/plasma/spectacle/-/blob/v6.8.2/src/Platforms/ImagePlatformKWin.cpp).

The methods return raw image bytes in
[formats supported by QImage](https://doc.qt.io/qt-6/qimage.html#Format-enum).
The exact format they return is not stable across KWin versions.
For instance,
the format of full screen and rectangular area shots has been
[changed](https://invent.kde.org/plasma/kwin/-/merge_requests/9798)
from `QImage::Format_RGBA8888_Premultiplied` to `QImage::Format_RGBX8888`
in Plasma 6.8.
Which means proper use of the interface practically requires Qt.

### Compatibility with older KWin versions

Before Plasma 6.8,
a relevant desktop file (either system-wide or user-local)
was required for the D-Bus interface to function.
Such restriction is subsequently
[lifted](https://invent.kde.org/plasma/kwin/-/merge_requests/6057).
If you are on the old versions,
put the following in `$XDG_DATA_HOME/applications/whatever.desktop`
in order for kgrim to work.

```ini
[Desktop Entry]
Type=Application
Exec="/full/path/to/kgrim"
NoDisplay=true
X-KDE-DBUS-Restricted-Interfaces=org.kde.KWin.ScreenShot2
```

### Benchmark

The D-Bus interface is fast.
kgrim is 90x faster than the `spectacle` command line on my machine:

```console
$ kwin_wayland --version
kwin 6.8.2
$ spectacle --version
spectacle 6.8.2
$ kgrim --version
kgrim r1
$ hyperfine --warmup=3 --shell=none 'spectacle --background --fullscreen --output /tmp/a.ppm' 'kgrim -t ppm /tmp/a.ppm'
Benchmark 1: spectacle --background --fullscreen --output /tmp/a.ppm (10 runs)
                  mean ±     σ    min …   max
  Wall Time [s]  2.498 ± 0.005  2.484 … 2.501
  Memory [MiB]   294.1 ±   0.1  293.7 … 294.3
 
Benchmark 2: kgrim -t ppm /tmp/a.ppm (107 runs)
                  mean ±     σ    min …   max
  Wall Time [s]  0.028 ± 0.001  0.026 … 0.030   -98.9%  (90.8x faster)
  Memory [MiB]    34.9 ±   0.2   34.4 …  35.2   -88.1%  (8.4x less)
 
$ file /tmp/a.ppm
/tmp/a.ppm: Netpbm image data, size = 1920 x 1080, rawbits, pixmap
```

## License

kgrim is distributed under the MIT license.

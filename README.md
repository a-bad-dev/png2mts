# PNG2MTS

<img src="misc/demo.png" alt="showcase"/>

A WIP converter that takes a PNG image and converts it to a Luanti schematic.
(WARNING: Success converting images not guaranteed.)

# How to compile
### Installing Dependencies:

Ubuntu/Debian:
```
sudo apt install g++ zlib1g-dev
```

Fedora:
```
sudo dnf install gcc-g++ kernel-devel zlib-devel
```

openSUSE:
```
sudo zypper install gcc-c++
```

Arch (btw):
```
sudo pacman -S base-devel
```

Alpine:
```
sudo apk add build-base zlib-dev
```

Void:
```
sudo xbps-install zlib-devel
```

Windows/MacOS:
good luck have fun (PRs welcome)

### Compiling:
```
g++ -o png2mts -lz -O3 src/main.cpp
```

# How to use
```
./png2mts -i <image>.png -o <schematic file>.mts
```
This will create a schematic using nodes from the `default` mod. To use a different color file, use `-p <file>`

# PNG2MTS

A WIP converter that takes in a image and converts it to a Luanti Schematic
(WARNING: Success converting images not guaranteed)

# How to compile
### Installing Dependencies:
Ubuntu/Debian:
```
sudo apt install zlib1g-dev
```

Windows/MacOS:
good luck have fun (PRs welcome)

### Compiling:
```
g++ src/main.cpp -o png2mts -lz
```

# How to use
First you need a image to convert. (keep the size <500px in any axis unless your crazy or want even more detail)
And second you need [Minetest/Luanti](https://github.com/luanti-org/luanti) and a way to load schematics (i recemmend [Worldedit](https://github.com/Uberi/Minetest-WorldEdit))

Run `png2mts --image <your image path here> --output <your output path here>` (e.g `png2mts --image ~/luanti-64.png --output ~/luanti-64.mts`) and move the resulting .mts file to your Luanti world's schematic directory (generally at `~/.minetest/worlds/<world name>/schems`)

Run Luanti and load the schematic and profit!
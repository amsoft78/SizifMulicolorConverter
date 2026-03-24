## Converter for JPG/BMP/PNG/ Commodore FLI images to various ZX Spectrum graphical C code.
This program reads and input file and prepares .h and .c file with image content in one of the extended ZX Spectrum format.

Usage: conv_zx file_path project_name color_mode
Color mode can be one of:
    mc   - Timex 2048 HiColor mode (8x1 attributes)
    hr   - Timex 2048 HiRes mode (512x192 mono)
    4    - 256x192 4 color mode with ULA+ registers
    4np  - 4 color mode, but without ULA+ registers
    16   - 128x192 16 colors mode
    dual - second playfield generation for 128x192 x 4 colors Dual Playfields mode

Note, that all other modes than Timex2048 HiRes and MultiColor are experimental Sizif 512 Multicolor modes from the following project fork:
[Sizif512 Multicolor](https://github.com/amsoft78/zx-sizif-512Timex/tree/CGA_4_16col).

To use the generated code in ZX Spectrum program, the content of the generated arrays must be copied to video memory, for example:

```
    memcpy((void*)0x5800, roses16_attribs0, 768);
    memcpy((void*)0x7800, roses16_attribs1, 768);
    memcpy((void*)0x4000, roses16_page_0, 6144);
    memcpy((void*)0x6000, roses16_page_1, 6144);
```

The code above is already present in the generated .c file.
The example usage in program is in the "zx" directory. The complete program can be compiled with z88SDK with the following command:

```
zcc +zx -compiler=sccz80 -lndos -create-app view16.c yourimagename16.c -o yourprogramname
```

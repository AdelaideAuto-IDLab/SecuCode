# Symbol Gen

Parses the linker info from the bootloader and builds a linker command file that allows an OTA firmware to reference symbols in the bootloader.

```
USAGE:
    symbol_gen [OPTIONS]

FLAGS:
    -h, --help       Prints help information
    -V, --version    Prints version information

OPTIONS:
    -i, --input <input>       [default: ../wisp5-bootloader/Debug/wisp5-bootloader_linkInfo.xml]
    -o, --output <output>     [default: ./bootloader_symbols.cmd]
```

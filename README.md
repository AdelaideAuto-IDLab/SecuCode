# SecuCode

Source code and additional documentation for *SecuCode: Intrinsic PUF Entangled Secure Wireless Code Dissemination for Computational RFID Devices*

Note: _Code will be released after publication of the paper_

## Overview

* `/demo`: Contains a video demonstrating the end-to-end system.
* `/experiments`: Raw data for hash and key derivation peformance tests.
* `/src` - To be released on paper publication.
    - `/wisp5-base`: The base WISP5 firmware forked from: https://github.com/wisp/wisp5/tree/e053c297d89913d8c78a08b69bc18cbbd22ad7af with modifications to support SecuCode (compiled as library)
    - `/wisp5-bootloader`: The main bootloader project forked from [MSP430FRBoot](http://www.ti.com/tool/mspbsl) with the SecuCode comm interface.
    - `/symbol_gen`: Parses the linker info from the bootloader and builds a linker command file that allows an firmware image to reference symbols in the bootloader.
    - `/secucode-demo`: Several sample firmwares compatible with SecuCode.
    - `/SecuCodeApp`: GUI Application for loading ELF files and sending them to a target tag.
    - `WISP_Enrollment_AIO.m`: Matlab tool to convert WISP SRAM dump to stable debiased crp-block and database file.

## Flashing the bootloader

The easiest way to flash the bootloader is using TI's Code Composer Studio (tested using CCS 7.2.0):

1. Import the `src` folder into CCS
2. Select the `wisp5-bootloader`
3. Update `puf.h` with the appropriate SRAM PUF mapping. (OR update the fixed key and use the SecuCode App in fixed key mode.)
4. Run the bootloader.

If the process was successful, the tag will be running in firmware accept mode, and should be visible to inventory commands.

## Building a new firmware image

It is highly recommended that you base your new image on the `secucode-demo` CCS project, as it has a correctly configured linker config file and build config. Otherwise carefully inspect the `lnk_msp430fr5969_wisp5_App.cmd` file before building a new firmware image from scratch.

Using CCS, build the project to obtain an ELF binary (default is `secucode-demo.out`) and then use the SecuCode App tool to send it to a tag running the bootloader (see the next section for more details).

NOTE: The firmware image must be compiled with the same version of the `wisp5-bootloader`. The CCS project is configured (via a pre-build step) to generate the appropriate linker config file that resolves missing symbols that can be found in the bootloader from the lastest built of the `wisp5-bootloader` code.

There are three example firmwares included that show off various bootloader features:

* `debug_firware.c`: uses a custom interrupt vector to blink an LED using a timer.
* `run_accel.c`: samples the accelerometer.
* `run_temp.c`: samples the temperature sensor.

## SecuCode App usage guide

Before running the SecuCodeApp, ensure that the SRAM PUF mapping in the app (`SecuCodeApp/bch/db.h`) matches the mapping in the bootloader (`wisp5-bootloader/puf.h`), or make sure you select the fix key flag from within the app.

The SecuCode App requires that an Impinj R420 reader is available to the host machine.

1. Connect to the reader by entering its address (e.g. `169.254.18.*` for link-local connections) and clicking the 'Connect' button.
2. Load the desired ELF file by clicking the 'Load Program'.
3. Send the firmware to the tag by clicking the 'Send Data' button.
4. Validate that firmware was successfully loaded by clicking the 'Inventory' button.

When debugging, it is recommended that you use the fixed key mode as a physical debugger connection can prevent the tag's SRAM from being reset on start up.

## Questions?

* Bootloader, SecuCode App - Michael Chesser (michael.chesser@adelaide.edu.au)
* Hardware, SRAM PUF - Yang Su (yang.su01@adelaide.edu.au) or (a1601999@gmail.com)

# SecuCode

Source code and additional documentation for *SecuCode: Intrinsic PUF Entangled Secure Wireless Code Dissemination for Computational RFID Devices*

## Overview

- `/wisp5-base`: The base WISP5 firmware forked from: https://github.com/wisp/wisp5/tree/e053c297d89913d8c78a08b69bc18cbbd22ad7af with modifications to support SecuCode (compiled as library)
    - Changes are primarily restricted to the `internals` folder, (`authenticate.c` is a good starting point), and `RFID/rfid_BlockWriteHandle.asm`
- `/wisp5-bootloader`: The main bootloader project forked from [MSP430FRBoot](http://www.ti.com/tool/mspbsl) with the SecuCode comm interface.
- `/symbol_gen`: Parses the linker info from the bootloader and builds a linker command file that allows a firmware image to reference symbols in the bootloader.
- `/secucode-demo`: Several sample firmwares compatible with SecuCode.
- `/SecuCodeApp`: GUI Application for loading ELF files and sending them to a target tag. Overview of important files:
    - `MspFirmware.cs` - Contains firmware parsing code.
    - `MspBoot.cs` - Contains code for framing firmware code according to the MSPBoot spec.
    - `SecuCode.cs` - Main SecuCode protocol implementation.
    - `ReaderManagement/ReaderManager.cs` - Handles the lowering of SecuCode protocol primitives to LLRP commands.
- `WISP_Enrollment_AIO.m`: Matlab tool to convert WISP SRAM dump to stable debiased crp-block and database file.

## Flashing the bootloader

The easiest way to flash the bootloader is using TI's Code Composer Studio (tested using CCS 9.0.1):

1. Import project into CCS
2. Select the `wisp5-bootloader`
3. Update `puf.h` with the appropriate SRAM PUF mapping. (OR update the fixed key and use the SecuCode App in fixed key mode.)
4. Run the bootloader.

If the process was successful, the tag will be running in firmware accept mode, and should be visible to inventory commands.

NOTE: Be careful with optimization settings, the TI compiler likes to aggressively unroll loops which can cause code size bloat. If more space for the bootloader is required, consider adjusting `__Boot_Start` in the linker config file (`lnk_msp430fr5969_wisp5_Boot.cmd`) and all other associated values. Note that `__Boot_Start` must be 1KB aligned for the MPU memory segmentation to behave correctly.

## Using the SecuCode App

Before running the SecuCodeApp, ensure that the SRAM PUF mapping in the app (`SecuCodeApp/bch/db.h`) matches the mapping in the bootloader (`wisp5-bootloader/puf.h`), or make sure you select the fix key flag from within the app.

The SecuCode App requires that an Impinj R420 reader is available to the host machine.

1. Connect to the reader by entering its address (e.g. `169.254.18.*` for link-local connections) and clicking the 'Connect' button.
2. Load a valid ELF file by clicking the 'Load Program'.
3. Send the firmware to the tag by clicking the 'Send Data' button.
4. Validate that firmware was successfully loaded by clicking the 'Inventory' button.

When debugging, it is recommended that you use the fixed key mode as a physical debugger connection can prevent the tag's SRAM from being reset on start-up causing continuous key decoding failures.

## Building a new firmware image

SecuCode firmware images are regular MSP430X ELF executables, and may consist of multiple internal functions, interrupt service routines (ISRs), and calls to existing functions within the bootloader.

It is highly recommended that you base your new image on the `secucode-demo` CCS project, as it has a correctly configured linker config file and build config. Otherwise carefully inspect the `lnk_msp430fr5969_wisp5_App.cmd` file before building a new firmware image from scratch.

Using CCS, build the project to obtain an ELF binary (default is `secucode-demo.out`) and then use the SecuCode App tool to send it to a tag running the bootloader (see the next section for more details).

NOTE: The firmware image must be compiled with the same version of the `wisp5-bootloader`. The CCS project is configured (via a pre-build step) to generate the appropriate linker config file that resolves missing symbols that can be found in the bootloader from the latest built of the `wisp5-bootloader` code.

Firmware images must also satisfy the restrictions outlined in the firmware loading process described below.

### Overview of firmware loading process

New firmware is written to the bootloader in a multi-stage process:

1. The firmware is parsed directly from the ELF file, the `PT_LOAD` segments are encoded are encoded as a sequence of bootloader `LOAD` commands (the bootloader currently doesn't support any other segment types).
    * Each command contains the length of the segment (`p_memsz`), physical address of the (`p_paddr`) and the segment data.
    * Typical firmware will have two non-zero length `PT_LOAD` segments, one for the `.text` segment, and one for the `.text:_isr` segment (it is possible to have more segments -- e.g. for initialization tables and constant data, however it is usually better to inline this into the `.text` segment and perform initialization on first boot).
    * Large `PT_LOAD` segments -- i.e. > 254 bytes -- are split into multiple smaller `PT_LOAD` segments and then encoded.

2. Each of the commands are written to the tag using BlockWrite commands. The `WordPtr` of the BlockWrite command is treated as a virtual address and is mapped to an unused part of memory (the download zone)
    * In the current implementation the download zone is limited to ~16kB in size. This is easily large enough for almost all firmware.

3. An additional `JUMP_TO_APP` command is written after all `LOAD` commands.

4. After writing all of the commands, a "Done" message is sent containing a MAC for the data in the download zone. If the MAC is valid, the bootloader then starts interpreting the messages. If a brownout occurs at any stage before this message all commands will be lost without being interpreted (since the tag will lose its copy of the key, and never be able to validate the MAC).

5. After validation the bootloader starts executing the `LOAD` commands and writing to executable memory, the LOAD command should not write to memory segments reserved by the bootloader (see discussion MPU configuration below). If a brownout occurs any time during this process, the new firmware will be partially written to tag, but will be dead code (i.e. will not be ever executed by the bootloader) since the `app_valid` flag is not set.

6. The final `JUMP_TO_APP` command sets an `app_valid` flag in shared, persistent memory and triggers a restart of the tag.

7. After the restart, the bootloader performs the following steps (note all the following steps are re-executed correctly in the case of brownout):
    - The watchdog timer is stopped.

    - The bootloader checks if it was restarted due to a watchdog timeout. If it was, the bootloader clears the `app_valid` flag.
    - The bootloader checks whether the `app_valid` flag is set. If `app_valid` is not set, the bootloader waits for new firmware to be sent.

    - Relevant entries from the application firmware's interrupt vector table (IVT) are copied into the bootloader's IVT (i.e. to allow the app to use timers, etc..)
        - Note: the RESET ISR is *not* copied into the bootloader's IVT, which allows the bootloader to regain control whenever the tag is restarted (e.g. on brownout), and reconfigure the MPU registers.

    - The MPU is configured in the following way:
        - `[0x10000-0x13FFF]` Download segment: Read + Write
        - `[bootloader_start-0x10000]` Bootloader segment (including the IVT) = Read + Execute
        - `[0x4400-bootloader_start]` Shared memory + App segment = Read + Write + Execute

    - The bootloader then enables MPU protection, locking the MPU configuration until reset.

    - The watchdog is then re-enabled, and the bootloader jumps to the application code via its RESET ISR.

8. After starting, the application firmware must clear the watchdog, and then can execute
    - The application firmware can perform it's own validation if required, handling corruption to the validation mechanism through judicious use of the watchdog mechanism (since a watchdog timeout will reset the bootloader into new firmware receive mode).

### Examples

The source code for 3 example firmware files are included, which show off various bootloader features:

* `debug_firware.c`: uses a custom interrupt vector to blink an LED using a timer.
* `run_accel.c`: samples the accelerometer.
* `run_temp.c`: samples the temperature sensor.

## Reference

This repository is provided as part of the following paper:

Y. Su, Y. Gao, M. Chesser, O. Kavehei, A. Sample and D. Ranasinghe, "SecuCode: Intrinsic PUF Entangled Secure Wireless Code Dissemination for Computational RFID Devices," in _IEEE Transactions on Dependable and Secure Computing_.

Cite using:

```
@ARTICLE{secucode2018,
    author={Y. {Su} and Y. {Gao} and M. {Chesser} and O. {Kavehei} and A. {Sample} and D. {Ranasinghe}},
    journal={IEEE Transactions on Dependable and Secure Computing},
    title={SecuCode: Intrinsic PUF Entangled Secure Wireless Code Dissemination for Computational RFID Devices},
    year={2019},
}
```

## License

The SecuCode project primarily distributed under the terms of the [GPL-3.0 License](./LICENSE), with portions of the code covered by various BSD-like licenses.


# UoS³ Flight Computer Firmware [![Build Status](https://travis-ci.org/uos3/obc-firmware.svg?branch=master)](https://travis-ci.org/uos3/obc-firmware)

This folder contains all the source (apart from licensed drivers) to build the executable file to be flashed directly to the hardware.

The firmware is built from a top-level C program file and resolves all the references and includes from the src/ and specified hardware folders

The design is such so that the main hardware code and hardware can be easily varied. eg camera code which can be used with different boards goes in the src folder, but the actual board specific drivers go in the relevant board folder.

## To use the build and flash tools

Open a terminal in the root folder of this firmware repository, then run the following commands:

`./build uos3-proto demo_blinky` - will compile demo_blinky.c with the folder 'src/board/uos3-proto/' included

  * The resultant ELF filename will be 'builds/_<board>-<program>-<gitref>_' eg 'builds/uos3-proto-blinky-fd8ea0c'

`./flash <binary>` - will flash a board (using a USB blackmagic probe) with specified ELF file

  * `./flash` with no arguments will use the most recent build from _builds/_


## To write a test program for the 'uos3-proto' circuit board

1. Install the software dependencies below

2. Create a C source file in _src/main/_, eg. *test.c*. A minimal example is below.
```
#include "../firmware.h"

int main(void)
{
  Board_init();

  /* Test Code goes here */
}
```

  * *firmware.h* should include all board header files.
  * `Board_init()` should run hardware setup functions required for the board.
  * The C source file must contain *main()* as this is the function that will be run on the hardware.

3. Compile the application with: `./build uos3-proto test`

  * `./build test` will compile by default for uos3-proto (The March '17 UoS3 OBC Development board)

4. Flash the latest compiled program (here, 'test') to the circuit board with: `./flash`

  * `./build -F test` will automatically flash after a successful build

5. (If needed) Debug the latest compiled program with `./debug`

  * This will flash the latest compiled program to the board and then leave you at an attached GDB shell
  * `./build -D test` will automatically debug after a successful build
  
  * Useful GDB Commands
    * 'r' - Run program from beginning
    * 'Ctrl+c' - Interrupt running of program
    * 'bt' - show stacktrace
    * 'info locals' - Show values of local variables
    * 'p a' - Show value of variable 'a'
    * 'c' - Continue program from current position
    * 'b demo_rtc.c:42' - Set a breakpoint at line 42 of demo_rtc.c
    * 'info breakpoints' - List current breakpoints
    * 'del br 1' - Delete breakpoint 1
    * 'quit' - Exit GDB
    * Resoures: [GDB Walkthrough](https://www.cs.umd.edu/~srhuang/teaching/cmsc212/gdb-tutorial-handout.pdf), [GDB Commands](http://www.yolinux.com/TUTORIALS/GDB-Commands.html)

## Dependencies 

1. Enable user access to serial devices (programmer)
    * Ubuntu:
      ```
      sudo gpasswd --add "$USER" dialout
      ```

2. Install Latest Official ARM-Embedded Toolchain
    * Ubuntu:
      ```
      sudo apt-add-repository -y ppa:team-gcc-arm-embedded/ppa
      sudo apt update
      sudo apt install gcc-arm-embedded
      ```

3. Install matching version of GNU/Linux Toolchain (only needed for compiling module tests)
    * Ubuntu:
      ```
      sudo apt-add-repository -y ppa:ubuntu-toolchain-r/test
      sudo apt update
      sudo apt install gcc-7
      ```

### For TI tm4c (eg. UoS3)

3. Acquire and unpack TI Tiva Driver Package
  * Due to the licensing agreement, this has to be manually added.
  * Download [SW-TM4C-2.1.4.178.exe](http://software-dl.ti.com/tiva-c/SW-TM4C/latest/index_FDS.html) (login required)
  * Unpack to _src/board/uos3-proto/TivaWare_C_Series-2.1.4.178_

# Authors

Phil Crump <phil@philcrump.co.uk>

Matthew Brejza

Suzanna Lucarotti

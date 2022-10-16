# Digital Voice Modem Transcoder

The DVM Transcoder software provides the protocol transcoding from DMR <-> P25.

## Building

This project utilizes CMake for its build system. (All following information assumes familiarity with the standard Linux make system.)

The DVM Transcoder software does not have any specific library dependancies and is written to be as library-free as possible. A basic GCC/G++ install is usually all thats needed to compile.

### Build Instructions
1. Clone the repository. ```git clone https://github.com/DVMProject/dvmtranscode.git```
2. Switch into the "dvmhost" folder. Create a new folder named "build" and switch into it. 
    ``` 
    # cd dvmtranscode
    dvmtranscode # mkdir build
    dvmtranscode # cd build
    ``` 
3. Run CMake with any specific options required. (Where [options] is any various compilation options you require.)
    ``` 
    dvmtranscode/build # cmake [options] ..
    ... 
    -- Build files have been written to: dvmhost/transcode
    dvmtranscode/build # make
    ``` 
If cross-compiling is required (for either ARM 32bit, 64bit or old Raspberry Pi ARM 32bit), the CMake build system has some options:
* ```-DCROSS_COMPILE_ARM=1``` - This will cross-compile dvmhost for ARM 32bit.
* ```-DCROSS_COMPILE_AARCH64=1``` - This will cross-compile dvmhost for ARM 64bit.
* ```-DCROSS_COMPILE_RPI_ARM=1``` - This will cross-compile for old Raspberry Pi ARM 32 bit. (see below)

Please note cross-compliation requires you to have the appropriate development packages installed for your system. For ARM 32-bit, on Debian/Ubuntu OS install the "arm-linux-gnueabihf-gcc" and "arm-linux-gnueabihf-g++" packages. For ARM 64-bit, on Debian/Ubuntu OS install the "aarch64-linux-gnu-gcc" and "aarch64-linux-gnu-g++" packages.

* For old RPi 1 using Debian/Ubuntu OS install the standard ARM embedded toolchain (typically "arm-none-eabi-gcc" and "arm-none-eabi-g++").
  1. Switch to "/opt" and checkout ```https://github.com/raspberrypi/tools.git```.

## License

This project is licensed under the GPLv2 License - see the [LICENSE.md](LICENSE.md) file for details. Use of this project is intended, strictly for amateur and educational use ONLY. Any other use is at the risk of user and all commercial purposes are strictly forbidden.


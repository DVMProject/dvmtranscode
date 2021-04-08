# Digital Voice Modem Transcoder

The DVM Transcoder software provides the protocol transcoding from DMR <-> P25.

## Building

Please see the various Makefile included in the project for more information. (All following information assumes familiarity with the standard Linux make system.)

The DVM Host software does not have any specific library dependancies and is written to be as library-free as possible. A basic GCC install is usually all thats needed to compile.

* Makefile - This makefile is used for building binaries for the native installed GCC.
* Makefile.arm - This makefile is used for cross-compiling for a ARM platform.

Use the ```make``` command to build the software.

## License

This project is licensed under the GPLv2 License - see the [LICENSE.md](LICENSE.md) file for details. Use of this project is intended, strictly for amateur and educational use ONLY. Any other use is at the risk of user and all commercial purposes are strictly forbidden.


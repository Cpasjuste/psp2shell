==========================
| PSP2SHELL By Cpasjuste |
==========================

ABOUT
=====

- psp2shell is an utility/remote shell (library+client) to ease the development process on ps vita devices.
    The client is currently available for linux x64 (ubuntu) and windows x64 (wip).

BUILD (LINUX)
=============

- library :
    - mkdir build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../Vita.cmake ../
    - make psp2shell

- client :
    - cd psp2shell_cli && mkdir build && cd build && cmake ../
    - make psp2shell_cli

BUILD (WINDOWS)
===============

- seriously ? :)

BUILD (MACOS)
=============

Library:

* mkdir build && cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../Vita.cmake ..
* make psp2shell

Client: In order to build the client on macOS, install `readline` from
Homebrew first. The "readline" provided by Apple is not actually readline.

* cd psp2shell_cli && mkdir build && cd build && cmake
  -DCMAKE_C_FLAGS="-I/usr/local/opt/readline/include"
  -DCMAKE_EXE_LINKER_FLAGS="-L/usr/local/opt/readline/lib"
  -DCMAKE_INSTALL_PREFIX="$VITASDK"
* make
* make install

USAGE
=====

- add libpsp2shell.a and psp2shell.h to your app..

- add psp2shell needed library flags to your app :
	-lpsp2shell -lSceSysmodule_stub -lSceNet_stub \
	-lSceNetCtl_stub -lSceKernel_stub -lScePower_stub -lSceAppMgr_stub

- DO NOT use secure flag "-s" (vita-make-fself -s), this is needed for reloading,
    you should then add the flag back when releasing your application.

- Please note that when using psp2shell library, you're application
    will never enter "deep sleep" ("power lock"), so the network connexion will not be lost.

- init psp2shell in your app : psp2shell_init(3333, 0)

- to print to the shell from your app use : psp2shell_print(..) / psp2shell_print_color(..)

- connect to your app with psp2shell_cli : ./psp2shell_cli 192.168.x.x 3333

- update/reload your app from the shell : reload "path/to/eboot.bin"

- type help (or ?) for more commands
    (cd, ls, pwd, rm, rmdir, mv, put, reset, reload...)


THANKS
======
- consolex-bordeaux.fr : for offering a ps vita device
- TheFl0w : for helping on a few functions
- vitasdk developers : for the sdk :)
- binn library author (https://github.com/liteserver/binn)

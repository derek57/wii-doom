cd data
make -f Makefile.SDL
cd ..
cd opl
make -f Makefile.SDL
cd ..
cd pcsound
make -f Makefile.SDL
cd ..
cd textscreen
make -f Makefile.SDL
cd ..
cd src
cd doom
make -f Makefile.SDL
cd ..
cd setup
make -f Makefile.SDL
cd ..
make -f Makefile.SDL chocolate-doom
make -f Makefile.SDL chocolate-setup
make -f Makefile.SDL chocolate-doom-setup
make -f Makefile.SDL chocolate-server
cd ..
cd man
make -f Makefile.SDL
cd bash-completion
make -f Makefile.SDL


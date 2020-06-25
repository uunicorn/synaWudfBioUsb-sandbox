rm -f a.exe
x86_64-w64-mingw32-dlltool -k -d /usr/lib/x86_64-linux-gnu/wine/libpropsys.def -l libpropsys.a
x86_64-w64-mingw32-g++ -g -I ~/opt/wdk-10/Include/wdf/umdf/1.11/ -Wall hello.cc breakpoints.c -luuid -lole32 -L. -lpropsys 
# -Wl,--stack,4194304 
ls -la a.exe

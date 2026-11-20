main: 
	gcc  -Werror *.c -I . -o target/ldc -g -O3 -fopenmp

install: target/ldc
	cp target/ldc /usr/bin/

windows: target/ldc.exe

target/ldc.exe: target
	x86_64-w64-mingw32-gcc -Werror *.c -I . -o target/ldc.exe -g -O3 -fopenmp

target/ldc: target
	gcc  -Werror *.c -I . -o target/ldc -g -O3 -fopenmp

target:
	mkdir target

clean:
	rm -r target/*

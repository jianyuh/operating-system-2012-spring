all: curveinstall ram1mount filesys

curveinstall:
	sudo apt-get install libncurses5-dev libncursesw5-dev

ram1mount: ram1mount.sh
	sudo sh ram1mount.sh

filesys:cache.c cache.h common.c common.h crtdef.c crtdef.h debug.c debug.h dir.c dir.h dirent.c dirent.h fat.c fat.h fatfilesys.h file.c file.h filesys filesys.c global.h globalstruct.h hai.h hai_file.c hai_file.h initfs.c initfs.h
	gcc -g   -c   -o debug.o debug.c
	gcc -g   -c   -o filesys.o filesys.c
	gcc -g   -c   -o common.o common.c
	gcc -g   -c   -o cache.o cache.c
	gcc -g   -c   -o dirent.o dirent.c
	gcc -g   -c   -o crtdef.o crtdef.c
	gcc -g   -c   -o hai_file.o hai_file.c
	gcc -g   -c   -o dir.o dir.c
	gcc -g   -c   -o file.o file.c
	gcc -g   -c   -o initfs.o initfs.c
	gcc -g   -c   -o fat.o fat.c
	gcc -g	 -c   -o filesys.o filesys.c
	gcc -g   -o filesys *.c
	rm 	*.o

clean:
	rm filesys




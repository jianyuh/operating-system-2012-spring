all:ram1mount fatsys

ram1mount: ram1mount.sh
	sudo sh ram1mount.sh

fatsys: fatsys.c fatsys.h
	gcc -Wall -O2 -o fatsys fatsys.c
clean:	rm fatsys

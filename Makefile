all:polld

polld:polld.c
	gcc -o polld -O2 -Wall polld.c

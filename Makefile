all:polld

polld:polld.c
	gcc -o polld -ggdb3 -Wall polld.c

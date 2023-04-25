all: project4 p4diff
project4: project4.c
	gcc -Wall -g project4.c -o project4 -fsanitize=address
p4diff: p4diff.c
	gcc -Wall -g p4diff.c -o p4diff -fsanitize=address


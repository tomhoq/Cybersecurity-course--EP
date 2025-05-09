all:  test attack attack2 format

c:
	rm test attack attack2 format

attack:
	gcc -g -O0 -z execstack -fno-stack-protector attack.c -o attack

attack2:
	gcc -g -O0 -z execstack -fno-stack-protector attack2.c -o attack2

test:
	gcc -g -O0 -z execstack -fno-stack-protector test.c -o test


format: 
	gcc -g -O0 format_vulnerability.c -o format

all:  test shell-test attack shell-instructions

clean:
	rm test shell-test attack shell-instructions

attack:
	gcc -g -O0 -z execstack -fno-stack-protector attack.c -o attack

test:
	gcc -g -O0 -z execstack -fno-stack-protector test.c -o test

shell-test:
	gcc -g -O0 -z execstack -no-pie -fno-stack-protector shell-test.c -o shell-test

shell-instructions:
	gcc -c -O0 shell-instructions.c -o shell-instructions

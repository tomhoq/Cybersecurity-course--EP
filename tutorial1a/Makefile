LANGUAGE=FRENCH
all: international-hello-world hello-world fancy-hello-world

fancy-hello-world: fancy-hello-world.c
	gcc -Wall -o fancy-hello-world fancy-hello-world.c
hello_world: hello-world.c
	gcc -Wall -o hello-world hello-world.c
international-hello-world: international-hello-world.c
	gcc -Wall -o international-hello-world international-hello-world.c -D${LANGUAGE}
love:
	echo "Hi"
clean:
	rm  international-hello-world hello-world fancy-hello-world

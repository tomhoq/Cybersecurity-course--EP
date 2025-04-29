all: shellcode

shellcode.o: shellcode.asm
	nasm -f elf shellcode.asm

shellcode: shellcode.o
	ld -m elf_i386 -o shellcode shellcode.o

.PHONY: clean
clean:
	rm shellcode.o
	rm shellcode
.PHONY: raw
raw:
	printf '\\x'
	printf '\\x' && objdump -d shellcode | grep "^ " | cut -f2 | tr -d ' ' | tr -d '\n' | sed 's/.\{2\}/&\\x /g'| head -c-3 | tr -d ' ' && echo ' '
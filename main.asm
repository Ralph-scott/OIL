[BITS 64]
global _start
section .text
_start:
    sub rsp, 16
    mov ecx, 0
    mov DWORD [rsp + 8], ecx
    mov ecx, DWORD [rsp + 8]
    mov DWORD [rsp + 0], ecx
    mov ecx, 0
    mov eax, ecx
    add rsp, 16
exit:
    mov rdi, rax
    mov rax, 60
    syscall
section .bss

[BITS 64]
global _start
section .text
_start:
    sub rsp, 24
    mov ecx, 0
    mov DWORD [rsp + 16], ecx
    mov ecx, 1
    mov DWORD [rsp + 8], ecx
    mov ecx, 0
    mov DWORD [rsp + 0], ecx
.L0:
    mov ecx, DWORD [rsp + 0]
    mov edi, 10000000
    cmp ecx, edi
    setl al
    mov ecx, eax
    mov eax, ecx
    test eax, eax
    jz .L1
    mov ecx, DWORD [rsp + 16]
    mov edi, DWORD [rsp + 8]
    add ecx, edi
    mov DWORD [rsp + 16], ecx
    mov ecx, DWORD [rsp + 16]
    mov edi, DWORD [rsp + 8]
    sub ecx, edi
    mov DWORD [rsp + 8], ecx
    mov ecx, DWORD [rsp + 0]
    mov edi, 1
    add ecx, edi
    mov DWORD [rsp + 0], ecx
    jmp .L0
.L1:
    mov eax, DWORD [rsp + 16]
    add rsp, 24
exit:
    mov rdi, rax
    mov rax, 60
    syscall
section .bss

[BITS 64]
global _start
section .text
_start:
    sub rsp, 8
    mov rcx, 0
    mov QWORD [rsp + 0], rcx
.L0:
    mov ecx, DWORD [rsp + 0]
    mov rdi, 10
    cmp ecx, edi
    setl al
    mov ecx, eax
    mov eax, ecx
    test eax, eax
    jz .L1
    mov ecx, DWORD [rsp + 0]
    mov rdi, 1
    add rcx, rdi
    mov DWORD [rsp + 0], ecx
    jmp .L0
.L1:
    mov eax, DWORD [rsp + 0]
    add rsp, 8
exit:
    mov rdi, rax
    mov rax, 60
    syscall
section .bss

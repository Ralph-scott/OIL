[BITS 64]
global _start
section .text
_start:
    sub rsp, 24
    mov rcx, 0
    mov QWORD [rsp + 24], rcx
    mov rcx, 1
    mov QWORD [rsp + 16], rcx
    mov rcx, 0
    mov QWORD [rsp + 8], rcx
.L0:
    mov rcx, 10000000
    mov rax, QWORD [rsp + 8]
    cmp rax, rcx
    setl al
    and rax, 0xff
    mov rdi, rax
    test rdi, rdi
    jz .L1
    mov rdi, QWORD [rsp + 24]
    mov rax, QWORD [rsp + 16]
    add rdi, rax
    mov QWORD [rsp + 24], rdi
    mov rcx, QWORD [rsp + 24]
    mov rcx, QWORD [rsp + 24]
    mov rax, QWORD [rsp + 16]
    sub rcx, rax
    mov QWORD [rsp + 16], rcx
    mov rdi, QWORD [rsp + 16]
    mov rdi, 1
    mov rcx, QWORD [rsp + 8]
    add rcx, rdi
    mov QWORD [rsp + 8], rcx
    mov rdi, QWORD [rsp + 8]
    jmp .L0
.L1:
    mov rax, QWORD [rsp + 24]
    sub rsp, 24
exit:
    mov rdi, rax
    mov rax, 60
    syscall

use64
    mov rax, 1
    xor rdx, rdx
    div r8
    mov rax, rdx
    add rdx, r10

    cmp rdx, r8
; if jumps are allowed
    js 6
    sub r10, r8
    sub rdi, r8
; else
    js 9
    sub rdx, r8
    sub rax, rdx
    sub rax, 0x10000001
; endif

    add r10, rax
    add rdi, rax
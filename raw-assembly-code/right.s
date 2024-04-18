use64
    mov rax, operand
    add r10, rax
    cmp r8, r10
    jns 8
    mov rax, 1
    ret
    add rdi, rax

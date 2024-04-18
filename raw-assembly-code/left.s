use64
    mov rax, operand
    sub r10, rax
    jns 8
    mov rax, 1
    ret
    sub rdi, rax

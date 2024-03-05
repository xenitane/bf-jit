use64
    mov rax, 1

; if jumps are allowed
    xor rdx, rdx
    div r8
    mov rax, rdx
; endif

    cmp r10, rax

; if jumps are allowed
    jns 6
    add rdi, r8
    add r10, r8
; else    
    jns 3
    mov rax, r10
; endif

    sub r10, rax
    sub rdi, rax

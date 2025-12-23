global asm_mov_all_bytes
global asm_nop_all_bytes
global asm_inc_cmp_empty_loop
global asm_dec_empty_loop

global asm_read_mov_x1
global asm_read_mov_x2
global asm_read_mov_x3
global asm_read_mov_x4

global asm_wide_read_mov_4x2
global asm_wide_read_mov_8x2
global asm_wide_read_mov_16x2
global asm_wide_read_mov_32x2

section .text

asm_mov_all_bytes:
    xor rax, rax
    align 64
.loop:
    mov [rdx + rax], al
    inc rax
    cmp rax, rcx
    jb .loop
    ret

asm_nop_all_bytes:
    xor rax, rax
    align 64
.loop:
    db 0x0f, 0x1f, 0x00 ; 3-byte nop
    inc rax
    cmp rax, rcx
    jb .loop
    ret

asm_inc_cmp_empty_loop:
    xor rax, rax
    align 64
.loop:
    inc rax
    cmp rax, rcx
    jb .loop
    ret

asm_dec_empty_loop:
    align 64
.loop:
    dec rcx
    jnz .loop
    ret
    
asm_read_mov_x1:
    align 64
.loop:
    mov rax, [rdx]
    sub rcx, 1
    jnle .loop
    ret
    
asm_read_mov_x2:
    align 64
.loop:
    mov rax, [rdx]
    mov rax, [rdx]
    sub rcx, 2
    jnle .loop
    ret
    
asm_read_mov_x3:
    align 64
.loop:
    mov rax, [rdx]
    mov rax, [rdx]
    mov rax, [rdx]
    sub rcx, 3
    jnle .loop
    ret
    
asm_read_mov_x4:
    align 64
.loop:
    mov rax, [rdx]
    mov rax, [rdx]
    mov rax, [rdx]
    mov rax, [rdx]
    sub rcx, 4
    jnle .loop
    ret

asm_wide_read_mov_4x2:
    xor rax, rax
    align 64
.loop:
    mov r8d, [rdx]
    mov r8d, [rdx + 4]
    add rax, 8
    cmp rax, rcx
    jb .loop
    ret

asm_wide_read_mov_8x2:
    xor rax, rax
    align 64
.loop:
    mov r8, [rdx]
    mov r8, [rdx + 8]
    add rax, 16
    cmp rax, rcx
    jb .loop
    ret

asm_wide_read_mov_16x2:
    xor rax, rax
    align 64
.loop:
    vmovdqu xmm0, [rdx]
    vmovdqu xmm0, [rdx + 16]
    add rax, 32
    cmp rax, rcx
    jb .loop
    ret

asm_wide_read_mov_32x2:
    xor rax, rax
    align 64
.loop:
    vmovdqu ymm0, [rdx]
    vmovdqu ymm0, [rdx + 32]
    add rax, 64
    cmp rax, rcx
    jb .loop
    ret
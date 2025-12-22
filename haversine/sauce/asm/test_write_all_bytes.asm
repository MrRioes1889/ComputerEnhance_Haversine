global asm_write_all_bytes_mov
global asm_write_all_bytes_nop
global asm_write_all_bytes_cmp
global asm_write_all_bytes_dec

section .text

asm_write_all_bytes_mov:
    xor rax, rax
.loop:
    mov [rdx + rax], al
    inc rax
    cmp rax, rcx
    jb .loop
    ret

asm_write_all_bytes_nop:
    xor rax, rax
.loop:
    db 0x0f, 0x1f, 0x00
    inc rax
    cmp rax, rcx
    jb .loop
    ret

asm_write_all_bytes_cmp:
    xor rax, rax
.loop:
    inc rax
    cmp rax, rcx
    jb .loop
    ret

asm_write_all_bytes_dec:
.loop:
    dec rcx
    jnz .loop
    ret
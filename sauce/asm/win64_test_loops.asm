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

global asm_cache_size_test_128
global asm_cache_size_test_256
global asm_cache_size_test_256_non_pow_of_2

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

; void asm_cache_size_test_128(uint64 buffer_size : rcx, uint8* buffer : rdx, uint64 slice_size : r8)
asm_cache_size_test_128:
    xor rax, rax
    dec r8 ; generating mask from slice size
    mov r9, rdx
    align 64
.loop:
    vmovdqu ymm0, [r9]
    vmovdqu ymm0, [r9 + 32]
    vmovdqu ymm0, [r9 + 64]
    vmovdqu ymm0, [r9 + 96]

    add rax, 128
    and rax, r8
    mov r9, rdx
    add r9, rax

    sub rcx, 128
    jnz .loop
    ret

; void asm_cache_size_test_256(uint64 buffer_size : rcx, uint8* buffer : rdx, uint64 slice_size : r8)
asm_cache_size_test_256:
    xor rax, rax
    dec r8 ; generating mask from slice size
    mov r9, rdx
    align 64
.loop:
    vmovdqu ymm0, [r9]
    vmovdqu ymm0, [r9 + 32]
    vmovdqu ymm0, [r9 + 64]
    vmovdqu ymm0, [r9 + 96]
    vmovdqu ymm0, [r9 + 128]
    vmovdqu ymm0, [r9 + 160]
    vmovdqu ymm0, [r9 + 192]
    vmovdqu ymm0, [r9 + 224]

    add rax, 256
    and rax, r8
    mov r9, rdx
    add r9, rax

    sub rcx, 256
    jnz .loop
    ret

; uint64 asm_cache_size_test_256_non_pow_of_2(uint64 buffer_size : rcx, uint8* buffer : rdx, uint64 read_block_256_count : r8)
asm_cache_size_test_256_non_pow_of_2:
    mov rax, rcx
    mov rcx, 256
    mov r10, rdx

    xor edx, edx
    div rcx
    xor edx, edx
    div r8

    mov rcx, rax
    mov rdx, r10
    xor rax, rax

    align 64
.outer:
    mov r9, rdx
    mov r10, r8

.inner:
    vmovdqu ymm0, [r9]
    vmovdqu ymm0, [r9 + 32]
    vmovdqu ymm0, [r9 + 64]
    vmovdqu ymm0, [r9 + 96]
    vmovdqu ymm0, [r9 + 128]
    vmovdqu ymm0, [r9 + 160]
    vmovdqu ymm0, [r9 + 192]
    vmovdqu ymm0, [r9 + 224]

    add r9, 256
    add rax, 256
    dec r10
    jnz .inner

    dec rcx
    jnz .outer

    ret




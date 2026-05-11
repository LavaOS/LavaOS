[BITS 64]
section .text
global enable_cpu_features

enable_cpu_features:
    call enable_sse
    fninit
    ret

enable_sse:
    mov rax, cr0
    and eax, ~(1 << 2)
    or  eax, (1 << 1)
    mov cr0, rax
    mov rax, cr4
    or  eax, (1 << 9) | (1 << 10)
    mov cr4, rax
    ret

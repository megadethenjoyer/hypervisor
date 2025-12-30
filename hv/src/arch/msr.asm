global msr_rdmsr

msr_rdmsr:
    mov ecx, edi
    xor rax, rax
    xor rdx, rdx
    rdmsr
    shl rdx, 32
    or rax, rdx
    ret


global arch_cpuid
global arch_read_cr0
global arch_write_cr0
global arch_read_cr3
global arch_write_cr3
global arch_read_cr4
global arch_write_cr4
global arch_hcf

arch_cpuid:
    mov eax, edi
    cpuid
    mov [rsi], eax
    mov [rsi + 4], ebx
    mov [rsi + 8], ecx
    mov [rsi + 12], edx
    ret

arch_read_cr0:
    mov rax, cr0
    ret

arch_write_cr0:
    mov cr0, rdi
    ret

arch_read_cr3:
    mov rax, cr3
    ret

arch_write_cr3:
    mov cr3, rdi
    ret

arch_read_cr4:
    mov rax, cr4
    ret

arch_write_cr4:
    mov cr4, rdi
    ret

arch_hcf:
    hlt
    jmp arch_hcf
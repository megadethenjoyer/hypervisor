global arch_cpuid
global arch_read_cr0
global arch_write_cr0
global arch_read_cr3
global arch_write_cr3
global arch_read_cr4
global arch_write_cr4
global arch_hcf
global arch_read_es
global arch_read_cs
global arch_read_ss
global arch_read_ds
global arch_read_fs
global arch_read_gs
global arch_read_tr
global arch_sgdt
global arch_sidt

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

arch_read_es:
    mov ax, es
    ret

arch_read_cs:
    mov ax, cs
    ret

arch_read_ss:
    mov ax, ss
    ret

arch_read_ds:
    mov ax, ds
    ret

arch_read_fs:
    mov ax, fs
    ret

arch_read_gs:
    mov ax, gs
    ret

arch_read_tr:
    str ax
    ret

arch_sgdt:
    sgdt [rdi]
    ret

arch_sidt:
    sgdt [rdi]
    ret
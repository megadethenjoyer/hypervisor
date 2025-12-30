bits 64
default rel

global io_outb

io_outb:
    mov edx, edi
    mov eax, esi
    out dx, al
    ret

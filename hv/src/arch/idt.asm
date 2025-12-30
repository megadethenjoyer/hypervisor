bits 64
default rel

section .text
extern handle_interrupt

global idt_lidt
global idt_sti

idt_lidt:
    lidt [rdi]
    ret

idt_sti:
    sti
    ret

idt_common_isr:
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi

    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    mov rax, cr2
    push rax

    cld
    
    mov rdi, rsp
    call handle_interrupt

    ; Add returning, right now useless


%macro NOERROR 1
isr_%1:
    push 0 ; error = 0

    push %1 ; vector
    jmp idt_common_isr
%endmacro

%macro ERROR 1
isr_%1:
    push %1 ; vector
    jmp idt_common_isr
%endmacro

NOERROR 0
NOERROR 1
NOERROR 2
NOERROR 3
NOERROR 4
NOERROR 5
NOERROR 6
NOERROR 7
ERROR 8
NOERROR 9
ERROR 10
ERROR 11
ERROR 12
ERROR 13
ERROR 14
NOERROR 15
NOERROR 16
ERROR 17
NOERROR 18
NOERROR 19
NOERROR 20
ERROR 21
NOERROR 22
NOERROR 23
NOERROR 24
NOERROR 25
NOERROR 26
NOERROR 27
NOERROR 28
NOERROR 29
NOERROR 30
NOERROR 31

%assign i 32
%rep 224
    NOERROR i
    %assign i i+1
%endrep

section .data
    global isrs
    isrs:
        %assign i 0
        %rep 256
            dq isr_%+i
        %assign i i+1
        %endrep
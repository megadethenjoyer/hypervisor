global vmx_vmxon
global vmx_vmclear
global vmx_vmptrld
global vmx_vmread
global vmx_vmwrite
; global vmx_vmlaunch
global vmx_launch_vm

vmx_error_handle:
    ; Intel SDM 3.31.2
    jc .invalid ; CF
    jz .valid   ; ZF

    ; All good
    mov rax, 0
    ret

    .valid;
        mov rax, 1
        ret
    
    .invalid:
        mov rax, 2
        ret

vmx_vmxon:
    vmxon [rdi]
    jmp vmx_error_handle

vmx_vmclear:
    vmclear [rdi]
    jmp vmx_error_handle

vmx_vmptrld:
    vmptrld [rdi]
    jmp vmx_error_handle

vmx_vmread:
    ;vmread [rsi], rdi
    ;vmread rax, rdi
    xor rax, rax
    vmread rax, rdi
    ret
    jmp vmx_error_handle

vmx_vmwrite:
    vmwrite rdi, rsi
    jmp vmx_error_handle

; vmx_vmlaunch:
    ; vmlaunch
    ; jmp vmx_error_handle

vmx_launch_vm:
    mov rax, 0x00006C14 ; VMCS_HOST_RSP
    vmwrite rax, rsp

    mov rax, 0x00006C16 ; VMCS_HOST_RIP
    lea rcx, .return
    vmwrite rax, rcx
    vmlaunch
    jmp vmx_error_handle

    .return:
    mov rax, 10
    ret

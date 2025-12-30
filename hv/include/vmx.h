#ifndef VMX_H
#define VMX_H

#include <pmm.h>
#include <ia32.h>
#include <stdbool.h>
#include <stdint.h>

enum vmx_error {
    vmx_ok = 0,
    vmx_error_valid = 1,
    vmx_error_invalid = 2
};

enum vmx_error vmx_vmxon( pa_t *p_vmxon_region_pa );
enum vmx_error vmx_vmclear( pa_t *p_vmcs_pa );
enum vmx_error vmx_vmptrld( pa_t *p_vmcs_pa );
//enum vmx_error vmx_vmread( uint64_t field_id, uint64_t *p_value );
uint64_t vmx_vmread( uint64_t field_id );
enum vmx_error vmx_vmwrite( uint64_t field_id, uint64_t value );
// enum vmx_error vmx_vmlaunch( );
void vmx_launch_vm( );

struct vmx_vcpu {
    uintptr_t hhdm;

    VMXON *vmxon;
    pa_t vmxon_pa;   

    VMCS *vmcs;
    pa_t vmcs_pa;
};

bool vmx_create_vcpu( struct vmx_vcpu *vcpu, uintptr_t hhdm );
bool vmx_check_cr_fixed( );
uint32_t vmx_get_revision_id( );
bool vmx_do_vmxon( struct vmx_vcpu *vcpu );
bool vmx_setup_vmcs( struct vmx_vcpu *vcpu );

#endif // VMX_H
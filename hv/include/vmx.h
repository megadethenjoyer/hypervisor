#ifndef VMX_H
#define VMX_H

#include <pmm.h>
#include <ia32.h>
#include <stdbool.h>
#include <stdint.h>

enum vmx_error {
    vmx_ok = 0,
    vmx_error_valid = 1,
    vmx_error_invalid = 2,
    vmx_vmlaunch_ok = 10,
};

enum vmx_error vmx_vmxon( pa_t *p_vmxon_region_pa );
enum vmx_error vmx_vmclear( pa_t *p_vmcs_pa );
enum vmx_error vmx_vmptrld( pa_t *p_vmcs_pa );
//enum vmx_error vmx_vmread( uint64_t field_id, uint64_t *p_value );
uint64_t vmx_vmread( uint64_t field_id );
enum vmx_error vmx_vmwrite( uint64_t field_id, uint64_t value );
// enum vmx_error vmx_vmlaunch( );
enum vmx_error vmx_launch_vm( );

struct vmx_vcpu {
    uintptr_t hhdm;

    VMXON *vmxon;
    pa_t vmxon_pa;   

    VMCS *vmcs;
    pa_t vmcs_pa;

    uint8_t *page;
    pa_t page_pa;

    EPT_PML4E *pml4;
    pa_t pml4_pa;

    EPT_PDPTE *pdpt;
    pa_t pdpt_pa;

    EPT_PDE *pd;
    pa_t pd_pa;

    EPT_PTE *pt;
    pa_t pt_pa;
};

bool vmx_create_vcpu( struct vmx_vcpu *vcpu, uintptr_t hhdm );
bool vmx_check_cr_fixed( );
uint32_t vmx_get_revision_id( );
bool vmx_do_vmxon( struct vmx_vcpu *vcpu );

bool vmx_setup_vmcs( struct vmx_vcpu *vcpu );
void vmx_save_host_state( );
void vmx_setup_ept( struct vmx_vcpu *vcpu );

#endif // VMX_H
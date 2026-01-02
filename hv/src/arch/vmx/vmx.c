#include <vmx.h>

#include <ia32.h>
#include <arch.h>
#include <mem.h>
#include <msr.h>
#include <io.h>
#include <log.h>

// TODO: fix
bool vmx_check_cr_fixed( ) {
    uint32_t required_cr0 = msr_rdmsr( IA32_VMX_CR0_FIXED0 );
    uint32_t required_cr4 = msr_rdmsr( IA32_VMX_CR4_FIXED0 );

    uint32_t optional_cr0 = msr_rdmsr( IA32_VMX_CR0_FIXED1 );
    uint32_t optional_cr4 = msr_rdmsr( IA32_VMX_CR4_FIXED1 );

    uint64_t cr0 = arch_read_cr0( ).AsUInt;
    uint64_t cr4 = arch_read_cr0( ).AsUInt;

    uint32_t cr0_missing = ( cr0 & required_cr0 ) ^ required_cr0;
    uint32_t cr4_missing = ( cr4 & required_cr4 ) ^ required_cr4;

    uint32_t cr0_forbidden = ( uint32_t )( cr0 ) & ~optional_cr0;
    uint32_t cr4_forbidden = ( uint32_t )( cr4 ) & ~optional_cr4;

    // If any of these is non-zero, return false
    uint32_t all = cr0_missing | cr4_missing | cr0_forbidden | cr4_forbidden;
    return all == 0;
}

uint32_t vmx_get_revision_id( ) {
    IA32_VMX_BASIC_REGISTER vmx_basic = { 0 };
    vmx_basic.AsUInt = msr_rdmsr( IA32_VMX_BASIC );
    return vmx_basic.VmcsRevisionId;
}

bool vmx_create_vcpu( struct vmx_vcpu *vcpu, uintptr_t hhdm ) {
    // if ( !vmx_check_cr_fixed( ) ) {
        // return false;
    // }

    vcpu->hhdm = hhdm;
    vcpu->vmxon_pa = pmm_alloc( 1 );
    LOGLN( LOG( "[vmx] vcpu->vmxon_pa = " ); LOG_HEX( vcpu->vmxon_pa ) );
    vcpu->vmcs_pa = pmm_alloc( 1 );
    LOGLN( LOG( "[vmx] vcpu->vmcs_pa = " ); LOG_HEX( vcpu->vmcs_pa ) );

    vcpu->vmxon = ( VMXON * )( hhdm + vcpu->vmxon_pa );
    vcpu->vmcs = ( VMCS * )( hhdm + vcpu->vmcs_pa );
    memset( vcpu->vmxon, 0, sizeof( VMXON ) );
    //memset( vcpu->vmcs, 0, sizeof( VMCS ) );

    vcpu->vmcs->AbortIndicator = 0;
    vcpu->vmxon->RevisionId = vmx_get_revision_id( );
    vcpu->vmcs->RevisionId = vmx_get_revision_id( );

    return true;
}

bool vmx_do_vmxon( struct vmx_vcpu *vcpu ) {
    return vmx_vmxon( &vcpu->vmxon_pa ) == vmx_ok;
}

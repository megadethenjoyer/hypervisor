#include <vmx.h>

#include <arch.h>
#include <msr.h>

#include <log.h>
#include <mem.h>

// Helper functions
bool check_true( uint64_t msr_index, uint64_t value, bool check_high ) {
    uint64_t msr = msr_rdmsr( msr_index );

    uint32_t low_value = ( uint32_t )( value );

    uint32_t low_msr = ( uint32_t )( msr );
    uint32_t high_msr = ( uint32_t )( msr >> 32 );

    uint32_t low = low_value & low_msr;
    uint32_t high = low_value & ( ~high_msr );

    if ( low != low_msr ) {
        return false;
    }

    if ( high != 0 && check_high ) {
        return false;
    }

    return true;
}

#define TRUECHECK( msr, ctrl ) if ( !check_true( IA32_VMX_##msr, ctrl.AsUInt, true ) ) {\
    LOGLN( LOG( "[vmx] Failed TRUECHECK of " ); LOG( "IA32_VMX_" #msr ) ); \
    hcf( );\
}

#define LEGCHECK( msr, ctrl ) TRUECHECK( TRUE_##msr, ctrl );  \
if ( !check_true( IA32_VMX_##msr, ctrl.AsUInt, false ) ) {\
    LOGLN( LOG( "[vmx] Failed LEGCHECK of " ); LOG( "IA32_VMX" #msr ) ); \
    hcf( );\
}

// legacy required
#define LEGREQ( msr ) ( ( msr_rdmsr( IA32_VMX_##msr ) & 0xFFFFFFFF ) | ( msr_rdmsr( IA32_VMX_TRUE_##msr ) & 0xFFFFFFFF ) )

// true required
#define TRUEREQ( msr ) ( ( msr_rdmsr( IA32_VMX_##msr ) & 0xFFFFFFFF ) )

void vmx_save_host_state( ) {
	SEGMENT_DESCRIPTOR_REGISTER_64 gdtr = { 0 };
	SEGMENT_DESCRIPTOR_REGISTER_64 idtr = { 0 };
    arch_sgdt( &gdtr );
    arch_sidt( &idtr );

    uint64_t host_fs_base = msr_rdmsr( IA32_FS_BASE );
	uint64_t host_gs_base = msr_rdmsr( IA32_GS_BASE );
    SEGMENT_DESCRIPTOR_64 *gdt = ( void * )( gdtr.BaseAddress );
    SEGMENT_DESCRIPTOR_64 host_tr = gdt[ arch_read_tr( ) >> 3 ];
	uint64_t host_tr_base = ARCH_BASE_ADDR_FROM_SEGMENT( host_tr );

	vmx_vmwrite( VMCS_HOST_ES_SELECTOR, arch_read_es( ) & 0xF8 );
	vmx_vmwrite( VMCS_HOST_CS_SELECTOR, arch_read_cs( ) & 0xF8 );
	vmx_vmwrite( VMCS_HOST_SS_SELECTOR, arch_read_ss( ) & 0xF8 );
	vmx_vmwrite( VMCS_HOST_DS_SELECTOR, arch_read_ds( ) & 0xF8 );
	vmx_vmwrite( VMCS_HOST_FS_SELECTOR, arch_read_fs( ) & 0xF8 );
	vmx_vmwrite( VMCS_HOST_GS_SELECTOR, arch_read_gs( ) & 0xF8 );
	vmx_vmwrite( VMCS_HOST_TR_SELECTOR, arch_read_tr( ) & 0xF8 );

	vmx_vmwrite( VMCS_HOST_CR0, arch_read_cr0( ).AsUInt );
	vmx_vmwrite( VMCS_HOST_CR3, arch_read_cr3( ).AsUInt );
	vmx_vmwrite( VMCS_HOST_CR4, arch_read_cr4( ).AsUInt );

	vmx_vmwrite( VMCS_HOST_GDTR_BASE, gdtr.BaseAddress );
	vmx_vmwrite( VMCS_HOST_IDTR_BASE, idtr.BaseAddress );
	vmx_vmwrite( VMCS_HOST_FS_BASE, host_fs_base );
	vmx_vmwrite( VMCS_HOST_GS_BASE, host_gs_base );
	vmx_vmwrite( VMCS_HOST_TR_BASE, host_tr_base );

	vmx_vmwrite( VMCS_HOST_SYSENTER_CS, msr_rdmsr( IA32_SYSENTER_CS ) );
	vmx_vmwrite( VMCS_HOST_SYSENTER_ESP, msr_rdmsr( IA32_SYSENTER_ESP ) );
	vmx_vmwrite( VMCS_HOST_SYSENTER_EIP, msr_rdmsr( IA32_SYSENTER_EIP ) );

	vmx_vmwrite( VMCS_HOST_PAT, msr_rdmsr( IA32_PAT ) );
	vmx_vmwrite( VMCS_HOST_EFER, msr_rdmsr( IA32_EFER ) );
}

void write_ctrl_fields( ) {
    vmx_vmwrite( VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS,
        ( msr_rdmsr( IA32_VMX_PINBASED_CTLS ) & 0xFFFFFFFF ) | ( 1 << 1 ) | ( 1 << 2 ) | ( 1 << 4 ) );
        
    IA32_VMX_PROCBASED_CTLS_REGISTER proc_based = { 0 };
    proc_based.AsUInt = LEGREQ( PROCBASED_CTLS );
    // Probably not required because of LEGREQ
    // proc_based.AsUInt |= 1 << 1;
    // proc_based.AsUInt |= ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 );
    // proc_based.AsUInt |= ( 1 << 13 ) | ( 1 << 14 ) | ( 1 << 15 ) | ( 1 << 16 );
    // proc_based.AsUInt |= 1 << 26;
    proc_based.ActivateSecondaryControls = 1;
    proc_based.HltExiting = 1;
    LEGCHECK( PROCBASED_CTLS, proc_based );

    IA32_VMX_PROCBASED_CTLS2_REGISTER proc_based2 = { 0 };
    proc_based2.AsUInt = TRUEREQ( PROCBASED_CTLS2 );
    proc_based2.EnableEpt = 1;
    // proc_based2.EptViolation = 1;
    proc_based2.UnrestrictedGuest = 1;
    TRUECHECK( PROCBASED_CTLS2, proc_based2 );

    vmx_vmwrite( VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
        proc_based.AsUInt );
    vmx_vmwrite( VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
        proc_based2.AsUInt );

    // vmx_vmwrite( VMCS_CTRL_EXCEPTION_BITMAP, 0xFFFFFFFF );
    vmx_vmwrite( VMCS_CTRL_EXCEPTION_BITMAP, 0  );

    IA32_VMX_EXIT_CTLS_REGISTER vmexit = { 0 };
    vmexit.AsUInt = ( msr_rdmsr( IA32_VMX_EXIT_CTLS ) & 0xFFFFFFFF ) | ( msr_rdmsr( IA32_VMX_TRUE_EXIT_CTLS ) & 0xFFFFFFFF );
    vmexit.HostAddressSpaceSize = 1;
    LEGCHECK( EXIT_CTLS, vmexit );
    vmx_vmwrite( VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, vmexit.AsUInt );

    IA32_VMX_ENTRY_CTLS_REGISTER vmentry = { 0 };
    vmentry.AsUInt = LEGREQ( ENTRY_CTLS );
    LEGCHECK( ENTRY_CTLS, vmentry );
    vmx_vmwrite( VMCS_CTRL_VMENTRY_CONTROLS, vmentry.AsUInt );

    vmx_vmwrite( VMCS_CTRL_CR3_TARGET_COUNT, 0 );
}

void setup_initial_guest_state( struct vmx_vcpu *vcpu ) {
    // TODO refactor these access rights
    VMX_SEGMENT_ACCESS_RIGHTS code_ar = { 0 };
    code_ar.AsUInt = 0x009B;
    VMX_SEGMENT_ACCESS_RIGHTS data_ar = { 0 };
    data_ar.AsUInt = 0x0093;
    VMX_SEGMENT_ACCESS_RIGHTS tss_ar = { 0 };
    tss_ar.AsUInt = 0x008B;
    VMX_SEGMENT_ACCESS_RIGHTS unusable_ar = { 0 };
    unusable_ar.Unusable = 1;

	vmx_vmwrite( VMCS_GUEST_VMCS_LINK_POINTER, ~0ull );
	vmx_vmwrite( VMCS_GUEST_DEBUGCTL, 0 );
	vmx_vmwrite( VMCS_GUEST_DR7, 0x400 );
	vmx_vmwrite( VMCS_GUEST_INTERRUPTIBILITY_STATE, 0 );
	vmx_vmwrite( VMCS_GUEST_ACTIVITY_STATE, 0 );
	vmx_vmwrite( VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0 );

	vmx_vmwrite( VMCS_GUEST_DS_SELECTOR, 0 );
	vmx_vmwrite( VMCS_GUEST_ES_SELECTOR, 0 );
	vmx_vmwrite( VMCS_GUEST_SS_SELECTOR, 0 );
	vmx_vmwrite( VMCS_GUEST_FS_SELECTOR, 0 );
	vmx_vmwrite( VMCS_GUEST_GS_SELECTOR, 0 );

	vmx_vmwrite( VMCS_GUEST_DS_BASE, 0 );
	vmx_vmwrite( VMCS_GUEST_ES_BASE, 0 );
	vmx_vmwrite( VMCS_GUEST_SS_BASE, 0 );
	vmx_vmwrite( VMCS_GUEST_FS_BASE, 0 );
	vmx_vmwrite( VMCS_GUEST_GS_BASE, 0 );

	vmx_vmwrite( VMCS_GUEST_DS_LIMIT, 0xFFFF );
	vmx_vmwrite( VMCS_GUEST_ES_LIMIT, 0xFFFF );
	vmx_vmwrite( VMCS_GUEST_SS_LIMIT, 0xFFFF );
	vmx_vmwrite( VMCS_GUEST_FS_LIMIT, 0xFFFF );
	vmx_vmwrite( VMCS_GUEST_GS_LIMIT, 0xFFFF );

	vmx_vmwrite( VMCS_GUEST_DS_ACCESS_RIGHTS, data_ar.AsUInt );
	vmx_vmwrite( VMCS_GUEST_ES_ACCESS_RIGHTS, data_ar.AsUInt );
	vmx_vmwrite( VMCS_GUEST_SS_ACCESS_RIGHTS, data_ar.AsUInt );
	vmx_vmwrite( VMCS_GUEST_FS_ACCESS_RIGHTS, data_ar.AsUInt );
	vmx_vmwrite( VMCS_GUEST_GS_ACCESS_RIGHTS, data_ar.AsUInt );

	vmx_vmwrite( VMCS_GUEST_CS_SELECTOR, 0xF000 );
	vmx_vmwrite( VMCS_GUEST_CS_BASE, 0xFFFF0000 );
	vmx_vmwrite( VMCS_GUEST_CS_LIMIT, 0xFFFF );
	vmx_vmwrite( VMCS_GUEST_CS_ACCESS_RIGHTS, code_ar.AsUInt );

	vmx_vmwrite( VMCS_GUEST_GDTR_BASE, 0 );
	vmx_vmwrite( VMCS_GUEST_GDTR_LIMIT, 0xFFFF );
	vmx_vmwrite( VMCS_GUEST_IDTR_BASE, 0 );
	vmx_vmwrite( VMCS_GUEST_IDTR_LIMIT, 0x03FF );

	vmx_vmwrite( VMCS_GUEST_TR_SELECTOR, 0 );
	vmx_vmwrite( VMCS_GUEST_TR_BASE, 0 );
	vmx_vmwrite( VMCS_GUEST_TR_LIMIT, 0xFFFF );
	vmx_vmwrite( VMCS_GUEST_TR_ACCESS_RIGHTS, tss_ar.AsUInt );

	vmx_vmwrite( VMCS_GUEST_LDTR_SELECTOR, 0 );
	vmx_vmwrite( VMCS_GUEST_LDTR_BASE, 0 );
	vmx_vmwrite( VMCS_GUEST_LDTR_LIMIT, 0 );
	vmx_vmwrite( VMCS_GUEST_LDTR_ACCESS_RIGHTS, unusable_ar.AsUInt );

    CR0 gcr0 = { 0 };
    gcr0.AsUInt = msr_rdmsr( IA32_VMX_CR0_FIXED0 ) & msr_rdmsr( IA32_VMX_CR0_FIXED1 );
    gcr0.PagingEnable = 0;
    gcr0.ProtectionEnable = 0;
    CR4 gcr4 = { 0 };
    gcr4.AsUInt = msr_rdmsr( IA32_VMX_CR4_FIXED0 ) & msr_rdmsr( IA32_VMX_CR4_FIXED1 );

	vmx_vmwrite( VMCS_GUEST_CR0, gcr0.AsUInt );
	vmx_vmwrite( VMCS_GUEST_CR3, 0 );
	vmx_vmwrite( VMCS_GUEST_CR4, gcr4.AsUInt );

	vmx_vmwrite( VMCS_GUEST_RIP, 0xFFF0 );
	vmx_vmwrite( VMCS_GUEST_RSP, 0x00000000 );
	vmx_vmwrite( VMCS_GUEST_RFLAGS, 0x2 );

	vmx_vmwrite( VMCS_GUEST_PAT, msr_rdmsr( IA32_PAT ) );
	vmx_vmwrite( VMCS_GUEST_EFER, 0 );
	vmx_vmwrite( VMCS_GUEST_SYSENTER_CS, 0 );
	vmx_vmwrite( VMCS_GUEST_SYSENTER_ESP, 0 );
	vmx_vmwrite( VMCS_GUEST_SYSENTER_EIP, 0 );

	vmx_vmwrite( VMCS_CTRL_CR0_GUEST_HOST_MASK, 0 );
	vmx_vmwrite( VMCS_CTRL_CR4_GUEST_HOST_MASK, 0 );
	vmx_vmwrite( VMCS_CTRL_CR0_READ_SHADOW, gcr0.AsUInt );
	vmx_vmwrite( VMCS_CTRL_CR4_READ_SHADOW, gcr4.AsUInt );

    EPT_POINTER eptp = { 0 };
    eptp.PageFrameNumber = vcpu->pml4_pa >> 12;
    eptp.MemoryType = 6;
    eptp.PageWalkLength = 3;
    vmx_vmwrite( VMCS_CTRL_EPT_POINTER, eptp.AsUInt );
}

void write_vmcs_fields( struct vmx_vcpu *vcpu ) {
    vmx_save_host_state( );

    write_ctrl_fields( );

    setup_initial_guest_state( vcpu );
}

bool vmx_setup_vmcs( struct vmx_vcpu *vcpu ) {
    if ( vmx_vmclear( &vcpu->vmcs_pa ) != vmx_ok ) {
        return false;
    }

    if ( vmx_vmptrld( &vcpu->vmcs_pa ) != vmx_ok ) {
        return false;
    }

    vmx_setup_ept( vcpu );

    write_vmcs_fields( vcpu );

    return true;
}
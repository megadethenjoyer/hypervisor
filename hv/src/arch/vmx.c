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

bool check_true( uint64_t msr_index, uint64_t value ) {
    uint64_t msr = msr_rdmsr( msr_index );

    uint32_t low_value = ( uint32_t )( value );

    uint32_t low_msr = ( uint32_t )( msr );
    uint32_t high_msr = ( uint32_t )( msr >> 32 );

    uint32_t low = low_value & low_msr;
    uint32_t high = low_value & ( ~high_msr );

    if ( low != low_msr ) {
        return false;
    }

    if ( high != 0 ) {
        return false;
    }

    return true;
}
bool check_legacy( uint64_t msr_index, uint64_t value ) {
    uint64_t msr = msr_rdmsr( msr_index );

    uint32_t low_value = ( uint32_t )( value );

    uint32_t low_msr = ( uint32_t )( msr );

    uint32_t low = low_value & low_msr;

    if ( low != low_msr ) {
        return false;
    }

    return true;
}

#define TRUECHECK( msr, ctrl ) if ( !check_true( IA32_VMX_##msr, ctrl.AsUInt ) ) {\
    LOGLN( LOG( "[vmx] Failed TRUECHECK of " ); LOG( "IA32_VMX_" #msr ) ); \
    hcf( );\
}

#define LEGCHECK( msr, ctrl ) TRUECHECK( TRUE_##msr, ctrl );  \
if ( !check_legacy( IA32_VMX_##msr, ctrl.AsUInt ) ) {\
    LOGLN( LOG( "[vmx] Failed LEGCHECK of " ); LOG( "IA32_VMX" #msr ) ); \
    hcf( );\
}

// legacy required
#define LEGREQ( msr ) ( ( msr_rdmsr( IA32_VMX_##msr ) & 0xFFFFFFFF ) | ( msr_rdmsr( IA32_VMX_TRUE_##msr ) & 0xFFFFFFFF ) )

// true required
#define TRUEREQ( msr ) ( ( msr_rdmsr( IA32_VMX_##msr ) & 0xFFFFFFFF ) )

void write_vmcs_fields( struct vmx_vcpu *vcpu ) {
    vmx_vmwrite( VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS,
        ( msr_rdmsr( IA32_VMX_PINBASED_CTLS ) & 0xFFFFFFFF ) | ( 1 << 1 ) | ( 1 << 2 ) | ( 1 << 4 ) );
        
    IA32_VMX_PROCBASED_CTLS_REGISTER proc_based = { 0 };
    proc_based.AsUInt = LEGREQ( PROCBASED_CTLS );
    proc_based.AsUInt |= 1 << 1;
    proc_based.AsUInt |= ( 1 << 4 ) | ( 1 << 5 ) | ( 1 << 6 );
    proc_based.AsUInt |= ( 1 << 13 ) | ( 1 << 14 ) | ( 1 << 15 ) | ( 1 << 16 );
    proc_based.AsUInt |= 1 << 26;
    proc_based.ActivateSecondaryControls = 1;
    proc_based.HltExiting = 1;
    LEGCHECK( PROCBASED_CTLS, proc_based );

    IA32_VMX_PROCBASED_CTLS2_REGISTER proc_based2 = { 0 };
    proc_based2.AsUInt = TRUEREQ( PROCBASED_CTLS2 );
    proc_based2.EnableEpt = 1;
    // proc_based2.EptViolation = 1;
    proc_based2.UnrestrictedGuest = 1;
    LOGLN( LOG( "[vmx] procbased2 = " ); LOG_HEX( (uint64_t)proc_based2.AsUInt ); LOG( " true = " ); LOG_HEX( msr_rdmsr( IA32_VMX_PROCBASED_CTLS2 ) ) );
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

    uint16_t es, cs, ss, ds, fs, gs, tr;
	asm volatile("mov %%es,%0" : "=r"(es));
	asm volatile("mov %%cs,%0" : "=r"(cs));
	asm volatile("mov %%ss,%0" : "=r"(ss));
	asm volatile("mov %%ds,%0" : "=r"(ds));
	asm volatile("mov %%fs,%0" : "=r"(fs));
	asm volatile("mov %%gs,%0" : "=r"(gs));
	asm volatile("str %0" : "=r"(tr));

	SEGMENT_DESCRIPTOR_REGISTER_64 gdtr = {}, idtr = {};
	asm volatile("sgdt %0" : "=m"(gdtr));
	asm volatile("sidt %0" : "=m"(idtr));

    uint64_t host_fs_base = msr_rdmsr(IA32_FS_BASE);
	uint64_t host_gs_base = msr_rdmsr(IA32_GS_BASE);
	// uint64_t host_tr_base = gdtr.BaseAddress + ;
        SEGMENT_DESCRIPTOR_64 host_tr = ((SEGMENT_DESCRIPTOR_64*)(gdtr.BaseAddress))[tr>>3];
	uint64_t host_tr_base = host_tr.BaseAddressLow | ( (uint64_t)host_tr.BaseAddressMiddle << 16 ) | ( (uint64_t)host_tr.BaseAddressHigh << 24) | ( (uint64_t)host_tr.BaseAddressUpper << 32);
    LOGLN(LOG("[vmx] host_tr_base = "); LOG_HEX(host_tr_base));

	vmx_vmwrite(VMCS_HOST_ES_SELECTOR, es & 0xF8);
	vmx_vmwrite(VMCS_HOST_CS_SELECTOR, cs & 0xF8);
	vmx_vmwrite(VMCS_HOST_SS_SELECTOR, ss & 0xF8);
	vmx_vmwrite(VMCS_HOST_DS_SELECTOR, ds & 0xF8);
	vmx_vmwrite(VMCS_HOST_FS_SELECTOR, fs & 0xF8);
	vmx_vmwrite(VMCS_HOST_GS_SELECTOR, gs & 0xF8);
	vmx_vmwrite(VMCS_HOST_TR_SELECTOR, tr & 0xF8);

	vmx_vmwrite(VMCS_HOST_CR0, arch_read_cr0().AsUInt);
	vmx_vmwrite(VMCS_HOST_CR3, arch_read_cr3().AsUInt);
	vmx_vmwrite(VMCS_HOST_CR4, arch_read_cr4().AsUInt);

	vmx_vmwrite(VMCS_HOST_GDTR_BASE, gdtr.BaseAddress);
	vmx_vmwrite(VMCS_HOST_IDTR_BASE, idtr.BaseAddress);
	vmx_vmwrite(VMCS_HOST_FS_BASE, host_fs_base);
	vmx_vmwrite(VMCS_HOST_GS_BASE, host_gs_base);
	vmx_vmwrite(VMCS_HOST_TR_BASE, host_tr_base);

	vmx_vmwrite(VMCS_HOST_SYSENTER_CS, msr_rdmsr(IA32_SYSENTER_CS));
	vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, msr_rdmsr(IA32_SYSENTER_ESP));
	vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, msr_rdmsr(IA32_SYSENTER_EIP));

	vmx_vmwrite(VMCS_HOST_PAT, msr_rdmsr(IA32_PAT));
	vmx_vmwrite(VMCS_HOST_EFER, msr_rdmsr(IA32_EFER));

	const uint64_t AR_CODE = 0x009B;
	const uint64_t AR_DATA = 0x0093;
	const uint64_t AR_TSS = 0x008B;
	const uint64_t AR_UNUSABLE = 1ull << 16;

	vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ull);
	vmx_vmwrite(VMCS_GUEST_DEBUGCTL, 0);
	vmx_vmwrite(VMCS_GUEST_DR7, 0x400);
	vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, 0);
	vmx_vmwrite(VMCS_GUEST_ACTIVITY_STATE, 0);
	vmx_vmwrite(VMCS_GUEST_PENDING_DEBUG_EXCEPTIONS, 0);

	vmx_vmwrite(VMCS_GUEST_DS_SELECTOR, 0);
	vmx_vmwrite(VMCS_GUEST_ES_SELECTOR, 0);
	vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, 0);
	vmx_vmwrite(VMCS_GUEST_FS_SELECTOR, 0);
	vmx_vmwrite(VMCS_GUEST_GS_SELECTOR, 0);

	vmx_vmwrite(VMCS_GUEST_DS_BASE, 0);
	vmx_vmwrite(VMCS_GUEST_ES_BASE, 0);
	vmx_vmwrite(VMCS_GUEST_SS_BASE, 0);
	vmx_vmwrite(VMCS_GUEST_FS_BASE, 0);
	vmx_vmwrite(VMCS_GUEST_GS_BASE, 0);

	vmx_vmwrite(VMCS_GUEST_DS_LIMIT, 0xFFFF);
	vmx_vmwrite(VMCS_GUEST_ES_LIMIT, 0xFFFF);
	vmx_vmwrite(VMCS_GUEST_SS_LIMIT, 0xFFFF);
	vmx_vmwrite(VMCS_GUEST_FS_LIMIT, 0xFFFF);
	vmx_vmwrite(VMCS_GUEST_GS_LIMIT, 0xFFFF);

	vmx_vmwrite(VMCS_GUEST_DS_ACCESS_RIGHTS, AR_DATA);
	vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS, AR_DATA);
	vmx_vmwrite(VMCS_GUEST_SS_ACCESS_RIGHTS, AR_DATA);
	vmx_vmwrite(VMCS_GUEST_FS_ACCESS_RIGHTS, AR_DATA);
	vmx_vmwrite(VMCS_GUEST_GS_ACCESS_RIGHTS, AR_DATA);

	vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, 0xF000);
	// vmx_vmwrite(VMCS_GUEST_CS_BASE, 0x00000000ull);
	vmx_vmwrite(VMCS_GUEST_CS_BASE, 0xFFFF0000ull);
	vmx_vmwrite(VMCS_GUEST_CS_LIMIT, 0xFFFF);
	vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, AR_CODE);

	vmx_vmwrite(VMCS_GUEST_GDTR_BASE, 0);
	vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, 0xFFFF);
	vmx_vmwrite(VMCS_GUEST_IDTR_BASE, 0);
	vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, 0x03FF);

	vmx_vmwrite(VMCS_GUEST_TR_SELECTOR, 0);
	vmx_vmwrite(VMCS_GUEST_TR_BASE, 0);
	vmx_vmwrite(VMCS_GUEST_TR_LIMIT, 0xFFFF);
	vmx_vmwrite(VMCS_GUEST_TR_ACCESS_RIGHTS, AR_TSS);

	vmx_vmwrite(VMCS_GUEST_LDTR_SELECTOR, 0);
	vmx_vmwrite(VMCS_GUEST_LDTR_BASE, 0);
	vmx_vmwrite(VMCS_GUEST_LDTR_LIMIT, 0);
	vmx_vmwrite(VMCS_GUEST_LDTR_ACCESS_RIGHTS, AR_UNUSABLE);

	uint64_t rip = 0xFFF0;
	uint64_t rflags = 0x2;

    uint64_t gcr0 = msr_rdmsr(IA32_VMX_CR0_FIXED0) & msr_rdmsr(IA32_VMX_CR0_FIXED1);
    gcr0 = gcr0 & ~((1 << 0) | (1 << 31));
    // uint64_t gcr0 = 0x60000010;
    CR4;
    uint64_t gcr4 = msr_rdmsr(IA32_VMX_CR4_FIXED0) & msr_rdmsr(IA32_VMX_CR4_FIXED1);
    // uint64_t gcr4 = 0;

	vmx_vmwrite(VMCS_GUEST_CR0, gcr0);
	vmx_vmwrite(VMCS_GUEST_CR3, 0);
	vmx_vmwrite(VMCS_GUEST_CR4, gcr4);

	vmx_vmwrite(VMCS_GUEST_RIP, rip);
	vmx_vmwrite(VMCS_GUEST_RSP, 0x00001000);
	vmx_vmwrite(VMCS_GUEST_RFLAGS, rflags);

	vmx_vmwrite(VMCS_GUEST_PAT, msr_rdmsr(IA32_PAT));
	vmx_vmwrite(VMCS_GUEST_EFER, 0);
	vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, 0);
	vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, 0);
	vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, 0);

	vmx_vmwrite(VMCS_CTRL_CR0_GUEST_HOST_MASK, 0);
	vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0);
	vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, gcr0);
	vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, gcr4);

    EPT_POINTER eptp = { 0 };
    eptp.PageFrameNumber = vcpu->pml4_pa >> 12;
    eptp.MemoryType = 6;
    eptp.PageWalkLength = 3;
    vmx_vmwrite( VMCS_CTRL_EPT_POINTER, eptp.AsUInt );
}

void setup_ept( struct vmx_vcpu *vcpu ) {
    vcpu->pml4_pa = pmm_alloc( 1 );
    vcpu->pml4 = ( void* )( vcpu->hhdm + vcpu->pml4_pa );
    vcpu->pdpt_pa = pmm_alloc( 1 );
    vcpu->pdpt = ( void* )( vcpu->hhdm + vcpu->pdpt_pa );
    vcpu->pd_pa = pmm_alloc( 1 );
    vcpu->pd = ( void * )( vcpu->hhdm + vcpu->pd_pa );
    vcpu->pt_pa = pmm_alloc( 1 );
    vcpu->pt = ( void * )( vcpu->hhdm + vcpu->pt_pa );
    
    memset( vcpu->pml4, 0, 0x1000 );
    memset( vcpu->pdpt, 0, 0x1000 );

    vcpu->page_pa = pmm_alloc_bytes( MiB( 2 ) );
    vcpu->page = ( void * )( vcpu->hhdm + vcpu->page_pa );

    LOGLN( LOG( "[vmx] Page: "); LOG_HEX( vcpu->page_pa ) );

    EPT_PTE pte = { 0 };
    pte.ReadAccess = 1;
    pte.WriteAccess = 0;
    pte.ExecuteAccess = 1;
    pte.PageFrameNumber = vcpu->page_pa >> 12;

    EPT_PDE pde = { 0 };
    pde.ReadAccess = 1;
    pde.WriteAccess = 1;
    pde.ExecuteAccess = 1;
    pde.PageFrameNumber = vcpu->pt_pa >> 12;

    EPT_PDPTE pdpte = { 0 };
    pdpte.ReadAccess = 1;   
    pdpte.WriteAccess = 1;   
    pdpte.ExecuteAccess = 1;   
    pdpte.PageFrameNumber = vcpu->pd_pa >> 12;

    EPT_PML4E pml4e = { 0 };
    pml4e.ReadAccess = 1;
    pml4e.WriteAccess = 1;
    pml4e.ExecuteAccess = 1;
    pml4e.PageFrameNumber = vcpu->pdpt_pa >> 12;

    for ( int i = 0; i < 512; i++ ) {
        vcpu->pt[ i ].AsUInt = pte.AsUInt;
    }
    for ( int i = 0; i < 512; i++ ) {
        vcpu->pd[ i ].AsUInt = pde.AsUInt;
    }
    for ( int i = 0; i < 512; i++ ) {
        vcpu->pdpt[ i ].AsUInt = pdpte.AsUInt;
    }
    for ( int i = 0; i < 512; i++ ) {
        vcpu->pml4[ i ].AsUInt = pml4e.AsUInt;
    }
}

bool vmx_setup_vmcs( struct vmx_vcpu *vcpu ) {
    if ( vmx_vmclear( &vcpu->vmcs_pa ) != vmx_ok ) {
        return false;
    }

    if ( vmx_vmptrld( &vcpu->vmcs_pa ) != vmx_ok ) {
        return false;
    }

    setup_ept( vcpu );

    write_vmcs_fields( vcpu );

    return true;
}
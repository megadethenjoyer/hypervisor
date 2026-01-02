#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <ia32.h>

#include <idt.h>
#include <io.h>
#include <msr.h>
#include <arch.h>
#include <pmm.h>
#include <mem.h>
#include <vmx.h>
#include <log.h>

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request limine_memmap = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request limine_hhdm = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request limine_exec = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;


void handle_interrupt( ) {
    LOGLN( LOG( "INT" ) );
    hcf();
}

void kmain( ) {
    if ( LIMINE_BASE_REVISION_SUPPORTED( limine_base_revision ) == false ) {
        hcf();
    }

    idt_setup( );
    IA32_VMX_BASIC_REGISTER basic;
    
    uintptr_t hhdm = limine_hhdm.response->offset;
    
    pmm_init( limine_memmap.response, hhdm, limine_exec.response );
    
    CR0 cr0 = arch_read_cr0( );
    cr0.NumericError = 1;
    cr0.CacheDisable = 1;
    arch_write_cr0( cr0 );
    
    CR4 cr4 = arch_read_cr4( );
    cr4.VmxEnable = 1;
    cr4.PageSizeExtensions = 1;
    arch_write_cr4( cr4 );
    
    // todo: error handling
    struct vmx_vcpu vcpu = { 0 };
    vmx_create_vcpu( &vcpu, hhdm );
    vmx_do_vmxon( &vcpu );
    vmx_setup_vmcs( &vcpu );

    struct cpuid_regs regs = { 0 };
    arch_cpuid( 1, &regs );
    LOGLN( LOG_HEX( regs.ebx ) );
    CPUID_EAX_01 cpuid = { 0 };
    cpuid.CpuidFeatureInformationEdx.AsUInt = regs.edx;
    if ( cpuid.CpuidFeatureInformationEdx.PageSizeExtension == 0 ) {
        LOGLN( LOG( "PSE not supported" ) );
    }

    for ( int i = 0; i < MiB( 2 ); i += 2 ) {
        vcpu.page[ i ] = 0xEB;
        vcpu.page[ i + 1 ] = 0xFE;
    }

    LOGLN( LOG("[vmx] EPTP = " ); LOG_HEX( vmx_vmread( VMCS_CTRL_EPT_POINTER ) ) );

    LOGLN( LOG( "guest rip = " ); LOG_HEX( vmx_vmread( VMCS_GUEST_RIP ) ) );
    LOGLN( LOG( "guest rsp = " ); LOG_HEX( vmx_vmread( VMCS_GUEST_RSP ) ) );
    LOGLN( LOG( "guest cr0 = " ); LOG_HEX( vmx_vmread( VMCS_GUEST_CR0 ) ) );
    LOGLN( LOG( "vmlaunch( ) = " ); LOG_HEX( ( uint32_t )vmx_launch_vm( ) ) );
    LOGLN( LOG( "guest rip = " ); LOG_HEX( vmx_vmread( VMCS_GUEST_RIP ) ) );
    LOGLN( LOG( "guest rsp = " ); LOG_HEX( vmx_vmread( VMCS_GUEST_RSP ) ) );
    LOGLN( LOG( "guest cr0 = " ); LOG_HEX( vmx_vmread( VMCS_GUEST_CR0 ) ) );


    LOGLN( LOG( "GPA: " ); LOG_HEX( vmx_vmread( VMCS_GUEST_PHYSICAL_ADDRESS ) ) );
    LOGLN( LOG( "VMX exit reason: " ); LOG_HEX( vmx_vmread( VMCS_EXIT_REASON ) ) );
    
    LOGLN( LOG( "Finished :)" ) );
    
    hcf( );
}

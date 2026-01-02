#include <vmx.h>
#include <pmm.h>
#include <log.h>
#include <mem.h>
#include <stdbool.h>

void vmx_setup_ept( struct vmx_vcpu *vcpu ) {
    vcpu->pml4_pa = pmm_alloc( 1 );
    vcpu->pml4 = ( void* )( vcpu->hhdm + vcpu->pml4_pa );
}

#define PRESENT( entry ) ( entry->ReadAccess == 1 || entry->WriteAccess == 1 )

pa_t vmx_gpa_to_pa_always( struct vmx_vcpu *vcpu, gpa_t gpa ) {
    uint64_t level4 = gpa >> 39;
    uint64_t level3 = ( gpa >> 30 ) & 0b111111111;
    uint64_t level2 = ( gpa >> 21 ) & 0b111111111;

    LOGLN( LOG( "[vmx] gpa level4 = " ); LOG_HEX( level4 ) );
    LOGLN( LOG( "[vmx] gpa level3 = " ); LOG_HEX( level3 ) );
    LOGLN( LOG( "[vmx] gpa level2 = " ); LOG_HEX( level2 ) );

    EPT_PML4E *pml4e = &vcpu->pml4[ level4 ];
    if ( !PRESENT( pml4e ) ) {
        pa_t pdpt_pa = pmm_alloc( 1 );
        void *pdpt = ( void * )( vcpu->hhdm + pdpt_pa );
        memset( pdpt, 0, 0x1000 );

        pml4e->ReadAccess = 1;
        pml4e->WriteAccess = 1;
        pml4e->ExecuteAccess = 1;
        pml4e->PageFrameNumber = pdpt_pa >> 12;
    }

    LOGLN( LOG( "[vmx] PML4E present, PFN << 12 = " ); LOG_HEX( ( uint64_t )( pml4e->PageFrameNumber << 12 ) ) );

    pa_t pdpt_pa = pml4e->PageFrameNumber << 12;
    EPT_PDPTE *pdpt = ( void * )( vcpu->hhdm + pdpt_pa );
    EPT_PDPTE *pdpte = &pdpt[ level3 ];
    if ( !PRESENT( pdpte ) ) {
        pa_t pd_pa = pmm_alloc( 1 );
        void *pd = ( void * )( vcpu->hhdm + pd_pa );
        memset( pd, 0, 0x1000 );

        pdpte->ReadAccess = 1;   
        pdpte->WriteAccess = 1;   
        pdpte->ExecuteAccess = 1;   
        pdpte->PageFrameNumber = pd_pa >> 12;
    }

    LOGLN( LOG( "[vmx] PDPTE present" ) );

    pa_t pd_pa = pdpte->PageFrameNumber << 12;
    EPT_PDE_2MB *pd = ( void * )( vcpu->hhdm + pd_pa );
    EPT_PDE_2MB *pde = &pd[ level2 ];
    if ( !PRESENT( pde ) ) {
        pa_t page_pa = pmm_alloc_bytes( MiB( 2 ) );
        void *page = ( void * )( vcpu->hhdm + page_pa );
        memset( page, 0, MiB( 2 ) ); // QoL

        LOGLN( LOG( "[vmx] Allocated 2MiB page, "); LOG_HEX( page_pa ); LOG( " at HHDM = " ); LOG_HEX( ( uint64_t )( page ) ) );

        pde->ReadAccess = 1;
        pde->WriteAccess = 1;
        pde->ExecuteAccess = 1;
        pde->LargePage = 1;
        pde->PageFrameNumber = page_pa >> 21;
    }

    LOGLN( LOG( "[vmx] PDE present" ) );

    return ( pde->PageFrameNumber << 21 ) | ( gpa & ~( 0xFFE00000 ) );
}
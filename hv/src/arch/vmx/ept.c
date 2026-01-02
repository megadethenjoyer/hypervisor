#include <vmx.h>
#include <log.h>
#include <mem.h>

void vmx_setup_ept( struct vmx_vcpu *vcpu ) {
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

    // EPT_PTE pte = { 0 };
    // pte.ReadAccess = 1;
    // pte.WriteAccess = 0;
    // pte.ExecuteAccess = 1;
    // pte.PageFrameNumber = vcpu->page_pa >> 12;

    EPT_PDE_2MB pde = { 0 };
    pde.ReadAccess = 1;
    pde.WriteAccess = 1;
    pde.ExecuteAccess = 1;
    // pde.PageFrameNumber = vcpu->pt_pa >> 12;
    pde.LargePage = 1;
    pde.PageFrameNumber = vcpu->page_pa >> 21;

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

    // for ( int i = 0; i < 512; i++ ) {
    //     vcpu->pt[ i ].AsUInt = pte.AsUInt;
    // }
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
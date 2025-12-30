#include <idt.h>

#include <ia32.h>

extern uint64_t isrs[];
SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 idt[ 256 ];

void idt_setup( ) {
    SEGMENT_SELECTOR kernel_code = { 0 };
    kernel_code.Index = 5;
    kernel_code.RequestPrivilegeLevel = 0;
    kernel_code.Table = 0;
    for ( int i = 0; i < 256; i++ ) {
        uint64_t isr = isrs[ i ];
        uint16_t offset_low  = ( isr >>  0 );
        uint16_t offset_mid  = ( isr >> 16 );
        uint32_t offset_high = ( isr >> 32 );

        idt[ i ].Reserved = 0;
        idt[ i ].SegmentSelector = kernel_code.AsUInt;
        idt[ i ].OffsetLow = offset_low;
        idt[ i ].OffsetMiddle = offset_mid;
        idt[ i ].OffsetHigh = offset_high;

        idt[ i ].Type = SEGMENT_DESCRIPTOR_TYPE_TRAP_GATE;
        if ( i == 2 || i > 31 ) {
            idt[ i ].Type = SEGMENT_DESCRIPTOR_TYPE_INTERRUPT_GATE;
        }

        idt[ i ].Present = 1;
    }

    
    SEGMENT_DESCRIPTOR_REGISTER_64 idtr = { 0 };
    idtr.Limit = sizeof( idt ) - 1;
    idtr.BaseAddress = ( uint64_t )idt;

    idt_lidt( &idtr );
    idt_sti( );

}
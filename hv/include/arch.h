#ifndef ARCH_H
#define ARCH_H

#include <ia32.h>
#include <stdint.h>

#define ARCH_BASE_ADDR_FROM_SEGMENT( desc ) \
        ( desc.BaseAddressLow | \
        ( ( uint64_t )( desc.BaseAddressMiddle ) << 16 ) | \
        ( ( uint64_t )( desc.BaseAddressHigh   ) << 24 ) | \
        ( ( uint64_t )( desc.BaseAddressUpper  ) << 32 ) )

struct cpuid_regs {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
};
void arch_cpuid( uint32_t leaf, struct cpuid_regs *regs );

CR0 arch_read_cr0( );
void arch_write_cr0( CR0 cr0 );
CR3 arch_read_cr3( );
void arch_write_cr3( CR3 cr3 );
CR4 arch_read_cr4( );
void arch_write_cr4( CR4 cr4 );

// halt and catch fire
void arch_hcf( );

#define hcf( ) arch_hcf( )

uint16_t arch_read_es( );
uint16_t arch_read_cs( );
uint16_t arch_read_ss( );
uint16_t arch_read_ds( );
uint16_t arch_read_fs( );
uint16_t arch_read_gs( );
uint16_t arch_read_tr( );

void arch_sgdt( void *gdt );
void arch_sidt( void *idt );

#endif // ARCH_H
#ifndef ARCH_H
#define ARCH_H

#include <ia32.h>
#include <stdint.h>

CR0 arch_read_cr0( );
void arch_write_cr0( CR0 cr0 );
CR3 arch_read_cr3( );
void arch_write_cr3( CR3 cr3 );
CR4 arch_read_cr4( );
void arch_write_cr4( CR4 cr4 );

// halt and catch fire
void arch_hcf( );

#define hcf( ) arch_hcf( )

#endif // ARCH_H
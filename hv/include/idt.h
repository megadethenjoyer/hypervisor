#ifndef IDT_H
#define IDT_H

#include <stdint.h>

void idt_lidt( void *idtr );
void idt_sti( );
void idt_setup( );

#endif // IDT_H
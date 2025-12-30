#ifndef MSR_H
#define MSR_H

#include <stdint.h>

uint64_t msr_rdmsr( uint32_t msr );

#endif // MSR_H
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <limine.h>

typedef uint64_t pa_t;

void pmm_init(
    struct limine_memmap_response *memmap,
    uintptr_t hhdm,
    struct limine_executable_address_response *exec );
pa_t pmm_alloc( size_t n );
void pmm_free( uintptr_t base, size_t n );

#endif // PMM_H
#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <limine.h>

typedef uint64_t pa_t;

#define KiB( n ) (      n * 1024   )
#define MiB( n ) ( KiB( n * 1024 ) )
#define GiB( n ) ( MiB( n * 1024 ) )

void pmm_init(
    struct limine_memmap_response *memmap,
    uintptr_t hhdm,
    struct limine_executable_address_response *exec );
pa_t pmm_alloc( size_t n );
pa_t pmm_alloc_bytes( size_t n );
void pmm_free( uintptr_t base, size_t n );

#endif // PMM_H
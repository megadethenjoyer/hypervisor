#include <pmm.h>

#include <stdbool.h>

#include <log.h>
#include <mem.h>

uint8_t *g_bitmap = NULL;
size_t   g_bitmap_size_bits = 0;

#define PAGE ( 0x1000 )

#define IS_ALIGNED( addr ) ( ( ( addr ) & 0xFFF ) == 0x000 )
#define ALIGN_PAGE_UP( addr ) ( IS_ALIGNED( addr ) ? ( addr ) : ( \
    ( addr ) + ( 0x1000 - ( ( addr ) & 0xFFF ) ) \
) )
#define ALIGN_PAGE_DOWN( addr ) ( IS_ALIGNED( addr ) ? ( addr ) : ( \
    ( addr ) - ( ( addr ) & 0xFFF ) \
) )

#define FOREACH_ENTRY( memmap, name ) \
    for ( \
        struct limine_memmap_entry **__curr_##name = memmap->entries, \
            *name = NULL, \
            **__end_##name = memmap->entries + memmap->entry_count; \
        ( __curr_##name < __end_##name ) && ( ( ( name = *__curr_##name ) != NULL ) || 1 ); \
        __curr_##name++ \
    )

size_t get_memmap_page_count( struct limine_memmap_response *memmap ) {
    // todo: track only highest *usable* page
    size_t n_pages = 0;

    FOREACH_ENTRY( memmap, entry ) {

        uintptr_t start = ALIGN_PAGE_UP(   entry->base );
        uintptr_t end   = ALIGN_PAGE_DOWN( entry->base + entry->length );
        size_t length = end - start;

        n_pages += length / PAGE;
    }

    return n_pages;
}

pa_t find_page_of_size( struct limine_memmap_response *memmap, size_t target_size ) {
    FOREACH_ENTRY( memmap, entry ) {
        if ( entry->type != LIMINE_MEMMAP_USABLE ) {
            continue;
        }

        uintptr_t start = ALIGN_PAGE_UP(   entry->base );
        uintptr_t end   = ALIGN_PAGE_DOWN( entry->base + entry->length );
        size_t length = end - start;

        if ( length < target_size ) {
            continue;
        }

        return entry->base;
    }

    return 0;
}


void set_bit( size_t index ) {
    size_t byte = index / 8;
    size_t bit = index % 8;

    g_bitmap[ byte ] |= ( 1 << bit );
}

void clear_bit( size_t index ) {
    size_t byte = index / 8;
    size_t bit = index % 8;

    g_bitmap[ byte ] &= ~( 1 << bit );
}

bool test_bit( size_t index ) {
    size_t byte = index / 8;
    size_t bit = index % 8;

    return ( g_bitmap[ byte ] >> bit ) & 1;
}

// Returns number of set bits
size_t reserve_pages( size_t base_page_index, size_t reserve_size ) {
    size_t n_bits = 0;

    for ( size_t i = 0; i < ALIGN_PAGE_UP( reserve_size ) / PAGE; i++ ) {
        set_bit( base_page_index + i );
        n_bits++;
    }

    return n_bits;
} 

// Returns number of set bits
size_t reserve_exec_pages( struct limine_executable_address_response *exec, size_t reserve_size ) {
    size_t base_page_index = exec->physical_base / PAGE;
    return reserve_pages( base_page_index, reserve_size );
}

#define PAGE_FREE( index ) ( test_bit( index ) == 0 )

#define MiB( n ) ( n * 1024 * 1024 )
void io_outb(uint16_t,uint8_t);
void pmm_init(
    struct limine_memmap_response *memmap,
    uintptr_t hhdm,
    struct limine_executable_address_response *exec ) {

    size_t bitmap_size_bits = get_memmap_page_count( memmap );
    size_t bitmap_size_bytes = ( bitmap_size_bits / 8 ) + 1; // Add one more to be sure
    pa_t bitmap_pa = find_page_of_size( memmap, bitmap_size_bytes );
    // todo: handle bitmap_pa == NULL ??
    // todo: handle bitmap_pa above hhdm_max (4GiB)


    uint8_t *bitmap = ( uint8_t * )( hhdm + bitmap_pa );
    memset( bitmap, 0xFF, bitmap_size_bytes );

    g_bitmap = bitmap;
    g_bitmap_size_bits = bitmap_size_bits;

    size_t n_free_pages = 0;
    int i =0;
    FOREACH_ENTRY( memmap, entry ) {
        if ( entry->type != LIMINE_MEMMAP_USABLE ) {
            continue;
        }

        uintptr_t start = ALIGN_PAGE_UP(   entry->base );
        uintptr_t end   = ALIGN_PAGE_DOWN( entry->base + entry->length );
        size_t length = end - start;

        size_t base_page_index = start  / PAGE;
        size_t n_pages    = length / PAGE;

        LOGLN( LOG( "[pmm] " ); LOG_HEX( start ); LOG( " - " ); LOG_HEX( end ); LOG( " " ); LOG_HEX( entry->type )  );

        for ( size_t i = 0; i < n_pages; i++ ) {
            n_free_pages++;
            clear_bit( base_page_index + i );
        }
    }

    size_t exec_bits = reserve_exec_pages( exec, MiB( 1 ) );
    n_free_pages -= exec_bits;
    
    size_t bitmap_bits = reserve_pages( bitmap_pa / PAGE, bitmap_size_bytes / PAGE );
    n_free_pages -= bitmap_bits;

    LOGLN( LOG( "[pmm] " ); LOG_HEX( n_free_pages ); LOG( "free pages" ) );
}

pa_t pmm_alloc( size_t n ) {
    size_t start_page_index = 0;
    size_t n_contiguous_pages = 0;

    for ( size_t page_index = 0; page_index < g_bitmap_size_bits; page_index++ ) {
        if ( !PAGE_FREE( page_index ) ) {
            n_contiguous_pages = 0;
            start_page_index = 0;
            continue;
        }

        if ( n_contiguous_pages == 0 ) {
            start_page_index = page_index;
        }
        n_contiguous_pages++;

        if ( n_contiguous_pages == n ) {
            reserve_pages( start_page_index, n );
            return start_page_index * PAGE;
        }
    }

    return 0;
}
void pmm_free( uintptr_t base, size_t n ) {
    (void)base;
    (void)n;
    // TODO, barely used anyway =)
}
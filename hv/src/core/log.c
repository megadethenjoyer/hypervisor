#include <log.h>
#include <io.h>

#define HEXALPHA "0123456789ABCDEF"

void log_char( char c ) {
    io_outb( 0xE9, c );
}

void log_string( const char *str ) {
    for ( int i = 0; str[ i ] != '\0'; i++ ) {
        log_char( str[ i ] );
    }
}

void convert_to_hex( uint64_t value, char *str, int n ) {
    int hex_count = n - 1; // nullbyte
    for ( int i = 0; i < hex_count; i++ ) {
        str[ hex_count - 1 - i ] = HEXALPHA[ value & 0xF ];
        value >>= 4;
    }
    str[ n - 1 ] = '\0';
}

void log_hex( const char *str ) {
    log_string( "0x" );
    log_string( str );
}

// TODO: remove boilerplate
void log_hex_u64( uint64_t value ) {
    char hex[ 16 + 1 ] = { 0 };
    convert_to_hex( value, hex, sizeof( hex ) );
    log_hex( hex );
}
void log_hex_u32( uint32_t value ) {
    char hex[ 8 + 1 ] = { 0 };
    convert_to_hex( value, hex, sizeof( hex ) );
    log_hex( hex );
}
void log_hex_u16( uint16_t value ) {
    char hex[ 4 + 1 ] = { 0 };
    convert_to_hex( value, hex, sizeof( hex ) );
    log_hex( hex );
}
void log_hex_u8( uint8_t value ) {
    char hex[ 2 + 1 ] = { 0 };
    convert_to_hex( value, hex, sizeof( hex ) );
    log_hex( hex );
}

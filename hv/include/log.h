#ifndef LOG_H
#define LOG_H

#include <stdint.h>

void log_char( char c );

void log_string( const char *str );

void log_hex_u64( uint64_t value );
void log_hex_u32( uint32_t value );
void log_hex_u16( uint16_t value );
void log_hex_u8( uint8_t value );

#define LOG_HEX( x ) _Generic( ( x ), \
                    uint64_t: log_hex_u64, \
                    uint32_t: log_hex_u32, \
                    uint16_t: log_hex_u16, \
                    uint8_t: log_hex_u8    \
)( ( x ) )

#define LOG( x ) _Generic( ( x ), \
                    const char *: log_string, \
                    char *:       log_string  \
)( ( x ) )
#define LOGLN( x ) x; log_char( '\n' );

#endif // LOG_H
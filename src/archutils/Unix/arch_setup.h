#ifndef ARCH_SETUP_UNIX_H
#define ARCH_SETUP_UNIX_H

#if !defined(MISSING_STDINT_H) /* need to define int64_t if so */
#include <stdint.h>
#endif

#if defined(CPU_X86)

inline uint32_t ArchSwap32( uint32_t n )
{
        return ( n >> 24 ) | ( n << 24 ) | ( ( n >> 8 ) & 0x00ff00 ) | ( ( n << 8 ) & 0xff0000 );
}

inline uint32_t ArchSwap24( uint32_t n )
{
        return ArchSwap32( n ) >> 8;
}

inline uint16_t ArchSwap16( uint16_t n )
{
        return ( n << 8 ) | ( n >> 8 );
}

#define HAVE_BYTE_SWAPS
#endif

#endif

/*
 * (c) 2004 Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

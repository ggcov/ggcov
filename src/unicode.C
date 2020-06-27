/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2020 Greg Banks <gnb@fastmail.fm>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "common.h"
#include "unicode.H"

using namespace std;

static const char replacement[] = "\\uFFFD";

static void
escape_utf16_codepoint(uint32_t c, ostream &o)
{
    if (c <= 0xffff)
    {
	/* codepoint in the BMP, emit a single escape sequence */
	static const char hexchars[] = "0123456789ABCDEF";
	o << "\\u";
	o << hexchars[(c >> 12) & 0xf];
	o << hexchars[(c >> 8) & 0xf];
	o << hexchars[(c >> 4) & 0xf];
	o << hexchars[c & 0xf];
    }
    else if (c <= 0x10ffff)
    {
	/* codepoint in supplementary planes; emit a UTF-16 surrogate pair */
	c -= 0x10000;
	escape_utf16_codepoint(0xd800 + (c >> 10), o);
	escape_utf16_codepoint(0xdc00 + (c & 0x3ff), o);
    }
    else
    {
	/* invalid character, emit the replacement char */
	o << replacement;
    }
}

void escape_utf8_string(const char *s, ostream &o)
{
    const uint8_t *p = (const uint8_t *)s;
    uint32_t code = 0;		/* the codepoint */
    unsigned int nbits =  0;	/* how many bits of codepoint we have */
    unsigned int maxbits =  0;	/* how many bits of codepoint we expect */

    o << '"';

    while (uint8_t c = *p++)
    {
	if ((c & 0x80))
	{
	    /* this byte is part of a UTF-8 encoding sequence */
	    if ((c & 0x40))
	    {
		/* leading byte for a new sequence */
		if (nbits)
		{
		    /* Was partway through a previous sequence and
		     * did not expect the start of a new sequence here.
		     * This is broken UTF-8, so throw away the partly
		     * built codepoint and emit a replacement char. */
		    o << replacement;
		}
		/* begin the new sequence */
		nbits = 5;
		maxbits = 11;
		uint8_t mask;
		for (mask = 0x20 ; mask >= 0x2 && (c & mask); mask >>= 1)
		{
		    maxbits += 5;
		    nbits--;
		}
		code = c & (mask-1);
	    }
	    else
	    {
		/* following byte in a sequence */
		if (nbits == 0)
		{
		    /* Was not in a sequence. This is broken UTF-8, so throw
		     * away the byte and emit a replacement char. */
		    o << replacement;
		}
		else
		{
		    code <<= 6;
		    code |= (c & 0x3f);
		    nbits += 6;
		    if (nbits == maxbits)
		    {
			/* sequence finished successfully */
			escape_utf16_codepoint(code, o);
			nbits = 0;
			maxbits = 0;
		    }
		}
	    }
	    continue;
	}

	/* normal 7-bit ASCII character */

	if (nbits)
	{
	    /* Was partway through a previous sequence and
	     * did not expect the start of a new sequence here.
	     * This is broken UTF-8, so throw away the partly
	     * built codepoint and emit a replacement char. */
	    nbits = 0;
	    maxbits = 0;
	    o << replacement;
	}

	if (c == '\\')
	{
	    o << "\\\\";
	}
	else if (c == '"')
	{
	    o << "\\\"";
	}
	else if (c == '\n')
	{
	    o << "\\n";
	}
	else if (c == '\r')
	{
	    o << "\\r";
	}
	else if (c == '\t')
	{
	    o << "\\t";
	}
	else if (c < 0x20)
	{
	    /* all other control characters */
	    escape_utf16_codepoint((uint32_t)c, o);
	}
	else
	{
	    o << (char)c;
	}
    }

    o << '"';
}



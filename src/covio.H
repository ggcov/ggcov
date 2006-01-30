/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2004 Greg Banks <gnb@alphalink.com.au>
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

#ifndef _ggcov_covio_h_
#define _ggcov_covio_h_ 1

#include "common.h"
#include "estring.H"

/*
 * There are twice as many functions as necessary because the
 * lowest level formatting changed at around gcc 3.3.  Integral
 * data are now saved bigendian, and string data grew a preceeding
 * length field.  The format now looks a *lot* like XDR!
 *
 * Then they changed once again at gcc 3.4 with three differences
 * a) now can be either bigendian or littleendian, b) string
 * lengths are recorded in 4-byte units not bytes and now include
 * the pad charrs and c) 64b ints are stored as lo32 then hi32
 * but each of those is host endian.
 *
 * Given all that, plus the necessity for runtime switching
 * between gcc33/gcc34l/gcc34b formats, we now have a class
 * to abstract these differences.
 */
class covio_t
{
public:
    covio_t(const char *fn)
     :  fn_(fn),
        fp_(0),
	ownfp_(TRUE)
    {
    }
    covio_t(const char *fn, FILE *fp)
     :  fn_(fn),
     	fp_(fp),
	ownfp_(FALSE)
    {
    }
    virtual ~covio_t();
    
    gboolean open_read();

    const char *filename() const { return fn_; }
    
    gboolean skip(unsigned int len);
    int read(char *buf, unsigned int len);
    int read(estring &e, unsigned int len);
    
    off_t tell() const { return ftell(fp_); }
    int seek(off_t off) { return fseek(fp_, off, SEEK_SET); }
    gboolean eof() const { return feof(fp_); }

protected:
    const char *fn_;
    FILE *fp_;
    gboolean ownfp_;

    /* These functions return TRUE unless EOF */
    gboolean read_lu32(gnb_u32_t&); /* little-endian 32b */
    gboolean read_bu32(gnb_u32_t&); /* big-endian 32b */
    gboolean read_lu64(gnb_u64_t&); /* little-endian 64b */
    gboolean read_bu64(gnb_u64_t&); /* big-endian 64b */
    
    /* the given byte length into a new string */
    gboolean read_string_len(estring &e, gnb_u32_t len);


public:
    /* These functions return TRUE unless EOF */
    virtual gboolean read_u32(gnb_u32_t &) = 0;
    virtual gboolean read_u64(gnb_u64_t &) = 0;
    virtual gboolean read_string(estring &) = 0;
};

class covio_old_t : public covio_t
{
public:
    covio_old_t(const char *fn, FILE *fp) : covio_t(fn, fp) { }
    covio_old_t(const char *fn) : covio_t(fn) { }
    virtual ~covio_old_t() { }

    /* Reads an old format .bb string up to the given tag & returns a new string */
    gboolean read_bbstring(estring &e, gnb_u32_t endtag);

public:
    gboolean read_u32(gnb_u32_t &);
    gboolean read_u64(gnb_u64_t &);
    gboolean read_string(estring &);
};

class covio_gcc33_t : public covio_t
{
public:
    covio_gcc33_t(const char *fn, FILE *fp) : covio_t(fn, fp) { }
    covio_gcc33_t(const char *fn) : covio_t(fn) { }
    virtual ~covio_gcc33_t() { }

public:
    gboolean read_u32(gnb_u32_t &);
    gboolean read_u64(gnb_u64_t &);
    gboolean read_string(estring &);
};

class covio_gcc34l_t : public covio_t
{
public:
    covio_gcc34l_t(const char *fn, FILE *fp) : covio_t(fn, fp) { }
    covio_gcc34l_t(const char *fn) : covio_t(fn) { }
    virtual ~covio_gcc34l_t() { }

public:
    gboolean read_u32(gnb_u32_t &);
    gboolean read_u64(gnb_u64_t &);
    gboolean read_string(estring &);
};

class covio_gcc34b_t : public covio_t
{
public:
    covio_gcc34b_t(const char *fn, FILE *fp) : covio_t(fn, fp) { }
    covio_gcc34b_t(const char *fn) : covio_t(fn) { }
    virtual ~covio_gcc34b_t() { }

public:
    gboolean read_u32(gnb_u32_t &);
    gboolean read_u64(gnb_u64_t &);
    gboolean read_string(estring &);
};

#endif /* _ggcov_covio_h_ */

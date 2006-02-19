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
#include "string_var.H"

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

class covio_t;

class covio_fmt_t
{
public:
    covio_fmt_t()
    {
    }
    virtual ~covio_fmt_t()
    {
    }

    /* These functions return TRUE unless EOF */
    virtual gboolean read_u32(covio_t &, gnb_u32_t &) = 0;
    virtual gboolean read_u64(covio_t &, gnb_u64_t &) = 0;
    virtual gboolean read_string(covio_t &, estring &) = 0;
};

class covio_t
{
public:
    covio_t(const char *fn)
     :  fn_(fn),
        fp_(0),
	ownfp_(TRUE),
	format_(0)
    {
    }
    covio_t(const char *fn, FILE *fp)
     :  fn_(fn),
     	fp_(fp),
	ownfp_(FALSE),
	format_(0)
    {
    }
    ~covio_t();
    
    gboolean open_read();

    const char *filename() const { return fn_; }
    FILE *take()
    {
    	FILE *fp = fp_;
	fp_ = 0;
	ownfp_ = FALSE;
    	return fp;
    }
    
    gboolean skip(unsigned int len);
    int read(char *buf, unsigned int len);
    int read(estring &e, unsigned int len);
    gboolean gets(estring &e, unsigned int maxlen);
    
    off_t tell() const { return ftell(fp_); }
    int seek(off_t off) { return fseek(fp_, off, SEEK_SET); }
    gboolean eof() const { return feof(fp_); }

    /* Reads an old format .bb string up to the given tag & returns a new string */
    gboolean read_bbstring(estring &, gnb_u32_t endtag);
    gboolean read_u32(gnb_u32_t &v)
    {
    	return format_->read_u32(*this, v);
    }
    gboolean read_u64(gnb_u64_t &v)
    {
    	return format_->read_u64(*this, v);
    }
    gboolean read_string(estring &v)
    {
    	return format_->read_string(*this, v);
    }

    enum format_t
    {
    	FORMAT_OLD, 	/* traditional gcc 2.x */
	FORMAT_GCC33,	/* gcc 3.3 */
	FORMAT_GCC34L,	/* gcc 3.4, little endian */
	FORMAT_GCC34B,	/* gcc 3.4, big endian */

	FORMAT_NUM
    };
    void set_format(format_t);

private:
    string_var fn_;
    FILE *fp_;
    gboolean ownfp_;
    covio_fmt_t *format_;

    /* These functions return TRUE unless EOF */
    gboolean read_lu32(gnb_u32_t&); /* little-endian 32b */
    gboolean read_bu32(gnb_u32_t&); /* big-endian 32b */
    gboolean read_lu64(gnb_u64_t&); /* little-endian 64b */
    gboolean read_bu64(gnb_u64_t&); /* big-endian 64b */
    
    /* the given byte length into a new string */
    gboolean read_string_len(estring &e, gnb_u32_t len);

    friend class covio_fmt_old_t;
    friend class covio_fmt_gcc33_t;
    friend class covio_fmt_gcc34l_t;
    friend class covio_fmt_gcc34b_t;
};

// Smart pointer class for covio_t
class covio_var
{
private:
    covio_t *io_;
    
public:
    // default ctor
    covio_var()
     :  io_(0)
    {
    }
    // const initialisation ctor
    covio_var(covio_t *io)
     :  io_(io)
    {
    }
    // copy c'tor: probably won't work without refcounting
//    covio_var(const covio_var &o)
//    {
//    	io_ = o.io_;
//    }
    // dtor    
    ~covio_var()
    {
    	delete io_;
    }

    operator covio_t *() const
    {
    	return io_;
    }
    // assignment operators
    covio_t *operator=(covio_t *io)
    {
    	delete io_;
	io_ = io;
	return io;
    }
};

#endif /* _ggcov_covio_h_ */

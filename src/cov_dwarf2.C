/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2003 Greg Banks <gnb@alphalink.com.au>
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

#include "cov_specific.H"
#include "ptrarray.H"

#ifdef HAVE_LIBBFD

CVSID("$Id: cov_dwarf2.C,v 1.1 2004-03-16 20:27:24 gnb Exp $");

/*
 * Machine-specific code to read DWARF v2 debug entries from an
 * object file or executable and parse them for source filenames.
 * Large chunks of code ripped from binutils/readelf.c and dwarf2.h
 */
 
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Streaming class for picking apart the binary encoding of DWARF structures.
 */

class dwarf_stream_t
{
private:
    const unsigned char *base_, *ptr_;
    bfd_size_type remain_;

public:
    dwarf_stream_t()
    {
    }
    
    ~dwarf_stream_t()
    {
    }
    
    void
    init(const unsigned char *data, unsigned int len)
    {
    	base_ = ptr_ = data;
	remain_ = len;
    }

    gboolean
    init_from_section(cov_bfd_t *cbfd, const char *name)
    {
	cov_bfd_section_t *sec;

    	if ((sec = cbfd->find_section(name)) == 0 ||
    	    (base_ = sec->get_contents(&remain_)) == 0)
	    return FALSE;
	ptr_ = base_;
	return TRUE;
    }
    
    unsigned long tell() const;
    gboolean seek(unsigned long off);
    gboolean skip(unsigned long delta);
    const unsigned char *data() const { return base_; }

    gboolean get_string(const char **);
    gboolean get_uint8(uint8_t *);
    gboolean get_uint16(uint16_t *);
    gboolean get_uint32(uint32_t *);
    gboolean get_uint64(uint64_t *);
    gboolean get_address(void **);	    // TODO: use the BFD typedef?
    gboolean get_varint(unsigned long *);
};

unsigned long
dwarf_stream_t::tell() const
{
    return (ptr_ - base_);
}

gboolean
dwarf_stream_t::seek(unsigned long off)
{
    unsigned int len = remain_ + (ptr_ - base_);
    if (off > len)
    	return FALSE;
    ptr_ = base_ + off;
    remain_ = len - off;
    return TRUE;
}

gboolean
dwarf_stream_t::skip(unsigned long delta)
{
    if (delta > remain_)
    	return FALSE;
    ptr_ += delta;
    remain_ -= delta;
    return TRUE;
}

gboolean
dwarf_stream_t::get_string(const char **valp)
{
    unsigned int i;
    
    for (i = 0 ; i <= remain_ && ptr_[i] ; i++)
    	;
    if (ptr_[i])
    	return FALSE;
    if (valp)
    	*valp = (const char *)ptr_;
    i++;
    ptr_ += i;
    remain_ -= i;
    return TRUE;
}

gboolean
dwarf_stream_t::get_uint8(uint8_t *valp)
{
    if (remain_ < 1)
    	return FALSE;
    if (valp)
    	*valp = *ptr_;
    ptr_ += 1;
    remain_ -= 1;
    return TRUE;
}

/*
 *  Note: we skirt the issue of endianness by assuming that ggcov
 *  	  runs on the same architecture as the generated files, i.e.
 *  	  cross-coverage is not supported.  So this operation is
 *        a simple memcpy.
 */

gboolean
dwarf_stream_t::get_uint16(uint16_t *valp)
{
    if (remain_ < sizeof(*valp))
    	return FALSE;
    if (valp)
    	memcpy((void *)valp, ptr_, sizeof(*valp));
    ptr_ += sizeof(*valp);
    remain_ -= sizeof(*valp);
    return TRUE;
}

gboolean
dwarf_stream_t::get_uint32(uint32_t *valp)
{
    if (remain_ < sizeof(*valp))
    	return FALSE;
    if (valp)
    	memcpy((void *)valp, ptr_, sizeof(*valp));
    ptr_ += sizeof(*valp);
    remain_ -= sizeof(*valp);
    return TRUE;
}

gboolean
dwarf_stream_t::get_uint64(uint64_t *valp)
{
    if (remain_ < sizeof(*valp))
    	return FALSE;
    if (valp)
    	memcpy((void *)valp, ptr_, sizeof(*valp));
    ptr_ += sizeof(*valp);
    remain_ -= sizeof(*valp);
    return TRUE;
}

gboolean
dwarf_stream_t::get_address(void **valp)
{
    if (remain_ < sizeof(*valp))
    	return FALSE;
    if (valp)
    	memcpy((void *)valp, ptr_, sizeof(*valp));
    ptr_ += sizeof(*valp);
    remain_ -= sizeof(*valp);
    return TRUE;
}

gboolean
dwarf_stream_t::get_varint(unsigned long *valp)
{
    unsigned int shift = 0;
    unsigned long val = 0;
    unsigned int i;
    
    for (i = 0 ; i < remain_ && shift < 25 ; i++, shift += 7)
    {
	val |= ((ptr_[i] & 0x7f) << shift);
	if (!(ptr_[i] & 0x80))
	{
	    if (valp)
    		*valp = val;
    	    i++;
	    ptr_ += i;
	    remain_ -= i;
	    return TRUE;
	}
    }

    return FALSE;
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct dwarf_attr_t
{
    unsigned int tag_;
    unsigned int form_;
};

struct dwarf_abbrev_t
{
    unsigned int entry_;
    unsigned int tag_;
    list_t<dwarf_attr_t> attrs_;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define DW_TAG_compile_unit 0x11


class cov_dwarf2_filename_scanner_t : public cov_filename_scanner_t
{
public:
    ~cov_dwarf2_filename_scanner_t();
    gboolean attach(cov_bfd_t *b);
    char *next();

private:
    void clear_dirs();
    gboolean get_lineinfo();
    char *get_lineinfo_filename();
    gboolean get_abbrevs();
    dwarf_abbrev_t *find_abbrev(unsigned long num) const;
    char *get_compunit_filename();

    /* contents of .debug_line section */
    dwarf_stream_t line_sec_;
    unsigned long lineinfo_off_;
    ptrarray_t<char> *dir_table_;
    
    /* contents of .debug_abbrev section */
    dwarf_stream_t abbrev_sec_;
    list_t<dwarf_abbrev_t> abbrevs_;

    /* contents of .debug_info section */
    dwarf_stream_t info_sec_;
    unsigned long compunit_off_;

    /* contents of .debug_str section */
    char *strings_;
    bfd_size_type string_size_;

    /* iteration variables */
    unsigned int state_;
};

COV_FACTORY_STATIC_REGISTER(cov_filename_scanner_t,
    	    	    	    cov_dwarf2_filename_scanner_t);

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_dwarf2_filename_scanner_t::~cov_dwarf2_filename_scanner_t()
{
    if (line_sec_.data() != 0)
    	g_free((void *)line_sec_.data());
    if (abbrev_sec_.data() != 0)
    	g_free((void *)abbrev_sec_.data());
    if (info_sec_.data() != 0)
    	g_free((void *)info_sec_.data());
    if (strings_ != 0)
    	g_free(strings_);
    if (dir_table_ != 0)
    {
    	clear_dirs();
    	delete dir_table_;
    }
}

void
cov_dwarf2_filename_scanner_t::clear_dirs()
{
    unsigned int i;

    for (i = 0 ; i < dir_table_->length() ; i++)
    {
	g_free(dir_table_->nth(i));
	dir_table_->set(i, 0);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_dwarf2_filename_scanner_t::attach(cov_bfd_t *b)
{
    cov_bfd_section_t *sec;

    if (!cov_filename_scanner_t::attach(b))
    	return FALSE;

    if (!line_sec_.init_from_section(cbfd_, ".debug_line"))
    	return FALSE;

    if (!abbrev_sec_.init_from_section(cbfd_, ".debug_abbrev"))
    	return FALSE;

    if (!info_sec_.init_from_section(cbfd_, ".debug_info"))
    	return FALSE;

    if ((sec = cbfd_->find_section(".debug_str")) == 0 ||
    	(strings_ = (char *)sec->get_contents(&string_size_)) == 0)
    	return FALSE;
    
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* read an external lineinfo struct, opcode lengths & dir table */
gboolean
cov_dwarf2_filename_scanner_t::get_lineinfo()
{
    uint32_t length;
    uint16_t version;
    uint8_t opcode_base;
    const char *dir;
    
    /* read the EXTERNAL_LineInfo struct */

    if (lineinfo_off_)
    	line_sec_.seek(lineinfo_off_);
    if (!line_sec_.get_uint32(&length))
    	return FALSE;
    /* TODO: bounds check length */
    lineinfo_off_ = line_sec_.tell() + length;

    if (!line_sec_.get_uint16(&version) || version != 2)
    	return FALSE;
    if (!line_sec_.get_uint32(/*prologue_length*/0) ||
        !line_sec_.get_uint8(/*min_insn_length*/0) ||
        !line_sec_.get_uint8(/*default_is_stmt*/0) ||
        !line_sec_.get_uint8(/*line_base*/0) ||
        !line_sec_.get_uint8(/*line_range*/0) ||
        !line_sec_.get_uint8(&opcode_base))
    	return FALSE;

    /* read the opcode lengths */
    if (!line_sec_.skip(opcode_base-1))
    	return FALSE;

    /* read the directory table */
    if (dir_table_ == 0)
    {
    	/* allocate a new table */
	dir_table_ = new ptrarray_t<char>;
    }
    else
    {
    	/* clear out the old table */
	clear_dirs();
	dir_table_->resize(0);
    }
    
    for (;;)
    {
    	if (!line_sec_.get_string(&dir))
	    return FALSE;
	if (*dir == '\0')
	    break;
	dir_table_->append(g_strdup(dir));

	dprintf2(D_DWARF, "dir_table[%d] = \"%s\"\n",
	    	dir_table_->length(),
		dir_table_->nth(dir_table_->length()-1));
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_dwarf2_filename_scanner_t::get_lineinfo_filename()
{
    const char *file;
    unsigned long diridx;
    char *path;
    
    if (!line_sec_.get_string(&file) || *file == '\0')
    	return 0;
    if (!line_sec_.get_varint(&diridx) ||
    	!line_sec_.get_varint(/*time*/0) ||
    	!line_sec_.get_varint(/*size*/0))
    	return 0;
    if (diridx > dir_table_->length())
    	return 0;
    path = (diridx ? 
    	    g_strconcat(dir_table_->nth(diridx-1), "/", file, 0) :
	    g_strdup(file));
    dprintf1(D_DWARF, "get_lineinfo_filename() = \"%s\"\n", path);
    return path;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_dwarf2_filename_scanner_t::get_abbrevs()
{
    unsigned long a, b;
    dwarf_abbrev_t *abbrev;
    dwarf_attr_t *attr;
    
    dprintf0(D_DWARF, "get_abbrevs:\n");

    for (;;)
    {
    	if (!abbrev_sec_.get_varint(&a))
	    return FALSE;
	    
	if (a == 0)
	    break;    /* end of section */
	    
    	if (!abbrev_sec_.get_varint(&b))
	    return FALSE;
    	if (!abbrev_sec_.get_uint8(/*children*/0))
	    return FALSE;
    
    	abbrev = new dwarf_abbrev_t;
	abbrev->entry_ = a;
	abbrev->tag_ = b;
	abbrevs_.append(abbrev);
	
	dprintf2(D_DWARF, "abbrev entry=%d tag=%d {\n",
	    	    abbrev->entry_, abbrev->tag_);
	
	for (;;)
	{
    	    if (!abbrev_sec_.get_varint(&a))
		return FALSE;
    	    if (!abbrev_sec_.get_varint(&b))
		return FALSE;
	    if (!a)
	    	break;
	    attr = new dwarf_attr_t;
	    attr->tag_ = a;
	    attr->form_ = b;
	    abbrev->attrs_.append(attr);

	    dprintf2(D_DWARF, "    attr=%d form=%d\n",
	    		attr->tag_, attr->form_);
	}
	dprintf0(D_DWARF, "}\n");
    }
    
    dprintf0(D_DWARF, "get_abbrevs: end of section\n");
    return TRUE;
}

dwarf_abbrev_t *
cov_dwarf2_filename_scanner_t::find_abbrev(unsigned long num) const
{
    list_iterator_t<dwarf_abbrev_t> iter;
    
    for (iter = abbrevs_.first() ; iter != (dwarf_abbrev_t *)0 ; ++iter)
    {
    	dwarf_abbrev_t *abbrev = (*iter);
	if (abbrev->entry_ == num)
	    return abbrev;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_dwarf2_filename_scanner_t::get_compunit_filename()
{
    uint32_t length;
    uint16_t version;
    unsigned long abbrevno;
    dwarf_abbrev_t *abbrev;
    uint32_t name_off, comp_dir_off, producer_off;
    char *path;

    /* read an external CompUnit structure */

    if (compunit_off_)
    	info_sec_.seek(compunit_off_);

    if (!info_sec_.get_uint32(&length))
    	return 0;
    /* TODO: bounds check length */
    compunit_off_ = info_sec_.tell() + length;

    if (!info_sec_.get_uint16(&version) || version != 2)
    	return 0;
    if (!info_sec_.get_uint32(/*abbrev_offset*/0) ||
        !info_sec_.get_uint8(/*pointer_size*/0))
    	return 0;
	
    /* hardcoded to expect a DW_TAG_compile_unit abbrev first */
    if (!info_sec_.get_varint(&abbrevno) ||
    	abbrevno == 0 ||
    	(abbrev = find_abbrev(abbrevno)) == 0 ||
	abbrev->tag_ != DW_TAG_compile_unit)
    	return 0;

    /* read hardcoded DW_TAG_compile_unit */
    if (!info_sec_.get_uint32(/*stmt_list*/0) ||
    	!info_sec_.get_address(/*high_pc*/0) ||
    	!info_sec_.get_address(/*low_pc*/0) ||
    	!info_sec_.get_uint32(&name_off) ||
    	!info_sec_.get_uint32(&comp_dir_off) ||
    	!info_sec_.get_uint32(&producer_off))
    	return 0;

    if (name_off > string_size_ ||
    	comp_dir_off > string_size_ ||
    	producer_off > string_size_)
    	return 0;
	
    path = g_strconcat(
    		strings_+comp_dir_off,
		"/",
		strings_+name_off,
		0);
    dprintf1(D_DWARF, "get_compunit_filename() = \"%s\"\n", path);
    return path;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_dwarf2_filename_scanner_t::next()
{
    char *s;

    for (;;)
    {
	switch (state_)
	{
	case 0: /* scanning LineInfo in .debug_line */
    	    state_ = (get_lineinfo() ? 1 : 2);
	    break;
	case 1: /* scanning filename table in .debug_line */
    	    if ((s = get_lineinfo_filename()) != 0)
	    	return s;
	    state_ = 0;	    	/* try for another LineInfo */
    	    break;
	case 2: /* scanning .debug_abbrev etc */
	    if (!get_abbrevs())
		return 0;	/* end of iteration */
	    state_ = 3;     	/* start scanning compunits */
	    break;
	case 3: /* scanning CompUnits in .debug_info */
	    return get_compunit_filename();
	default:
    	    return 0;	/* end of iteration */
	}
    }

//	dprintf0(D_DWARF|D_VERBOSE, "index|type|other|desc|value   |string\n");

    return 0;	/* end of iteration */
}

#endif /*HAVE_LIBBFD*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/

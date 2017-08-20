/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2004 Greg Banks <gnb@users.sourceforge.net>
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
#include "logging.H"
#include <vector>
#include <map>

#ifdef HAVE_LIBBFD

/*
 * Machine-specific code to read DWARF v2 debug entries from an
 * object file or executable and parse them for source filenames.
 * Large chunks of code ripped from binutils/readelf.c and dwarf2.h
 */

static logging::logger_t &_log = logging::find_logger("dwarf");

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

    bool
    init_from_section(cov_bfd_t *cbfd, const char *name)
    {
	cov_bfd_section_t *sec;

	if ((sec = cbfd->find_section(name)) == 0 ||
	    (base_ = sec->get_contents(&remain_)) == 0)
	    return false;
	ptr_ = base_;
	return true;
    }

    unsigned long tell() const;
    bool seek(unsigned long off);
    bool skip(unsigned long delta);
    const unsigned char *data() const { return base_; }

    bool get_string(const char **);
    bool get_uint8(uint8_t *);
    bool get_uint16(uint16_t *);
    bool get_uint32(uint32_t *);
    bool get_uint64(uint64_t *);
    bool get_unit_header_length(uint64_t *, bool *is64bp);
    bool get_offset(uint64_t *, bool is64b);
    bool get_address(void **);          // TODO: use the BFD typedef?
    bool get_varint(unsigned long *);
    const unsigned char *get_bytes(const uint8_t **valp, size_t len);
};

unsigned long
dwarf_stream_t::tell() const
{
    return (ptr_ - base_);
}

bool
dwarf_stream_t::seek(unsigned long off)
{
    unsigned int len = remain_ + (ptr_ - base_);
    if (off > len)
	return false;
    ptr_ = base_ + off;
    remain_ = len - off;
    return true;
}

bool
dwarf_stream_t::skip(unsigned long delta)
{
    if (delta > remain_)
	return false;
    ptr_ += delta;
    remain_ -= delta;
    return true;
}

bool
dwarf_stream_t::get_string(const char **valp)
{
    unsigned int i;

    for (i = 0 ; i <= remain_ && ptr_[i] ; i++)
	;
    if (ptr_[i])
	return false;
    if (valp)
	*valp = (const char *)ptr_;
    i++;
    ptr_ += i;
    remain_ -= i;
    return true;
}

bool
dwarf_stream_t::get_uint8(uint8_t *valp)
{
    if (remain_ < 1)
	return false;
    if (valp)
	*valp = *ptr_;
    ptr_ += 1;
    remain_ -= 1;
    return true;
}

/*
 *  Note: we skirt the issue of endianness by assuming that ggcov
 *        runs on the same architecture as the generated files, i.e.
 *        cross-coverage is not supported.  So this operation is
 *        a simple memcpy.
 */

bool
dwarf_stream_t::get_uint16(uint16_t *valp)
{
    if (remain_ < sizeof(*valp))
	return false;
    if (valp)
	memcpy((void *)valp, ptr_, sizeof(*valp));
    ptr_ += sizeof(*valp);
    remain_ -= sizeof(*valp);
    return true;
}

bool
dwarf_stream_t::get_uint32(uint32_t *valp)
{
    if (remain_ < sizeof(*valp))
	return false;
    if (valp)
	memcpy((void *)valp, ptr_, sizeof(*valp));
    ptr_ += sizeof(*valp);
    remain_ -= sizeof(*valp);
    return true;
}

bool
dwarf_stream_t::get_uint64(uint64_t *valp)
{
    if (remain_ < sizeof(*valp))
	return false;
    if (valp)
	memcpy((void *)valp, ptr_, sizeof(*valp));
    ptr_ += sizeof(*valp);
    remain_ -= sizeof(*valp);
    return true;
}

bool
dwarf_stream_t::get_unit_header_length(uint64_t *valp, bool *is64bp)
{
    uint32_t word;
    if (!get_uint32(&word))
	return false;
    if (word == 0xffffffff)
    {
	*is64bp = true;
	return get_uint64(valp);
    }
    else if (word > 0xfffffff0)
    {
	return false;
    }
    else
    {
	*is64bp = false;
	if (valp)
	    *valp = word;
	return true;
    }
}

bool
dwarf_stream_t::get_offset(uint64_t *valp, bool is64b)
{
    if (!is64b)
    {
	uint32_t word;
	if (!get_uint32(&word))
	    return false;
	if (valp)
	    *valp = word;
	return true;
    }
    return get_uint64(valp);
}

bool
dwarf_stream_t::get_address(void **valp)
{
    if (remain_ < sizeof(*valp))
	return false;
    if (valp)
	memcpy((void *)valp, ptr_, sizeof(*valp));
    ptr_ += sizeof(*valp);
    remain_ -= sizeof(*valp);
    return true;
}

bool
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
	    return true;
	}
    }

    return false;
}

bool
dwarf_stream_t::get_bytes(const uint8_t **valp, size_t len)
{
    if (remain_ < len)
	return false;
    if (valp)
	*valp = ptr_;
    ptr_ += len;
    remain_ -= len;
    return true;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

struct dwarf_attr_t
{
    dwarf_attr_t(unsigned int tag, unsigned int form)
     :  tag_(tag), form_(form) {}

    unsigned int tag_;
    unsigned int form_;
};

struct dwarf_abbrev_t
{
    dwarf_abbrev_t(unsigned int entry, unsigned int tag)
     :  entry_(entry), tag_(tag) {}

    unsigned int entry_;
    unsigned int tag_;
    vector<dwarf_attr_t> attrs_;
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#define DW_TAG_compile_unit 0x11


class cov_dwarf2_filename_scanner_t : public cov_filename_scanner_t
{
public:
    ~cov_dwarf2_filename_scanner_t();
    bool attach(cov_bfd_t *b);
    char *next();

private:
    void clear_dirs();
    bool get_lineinfo();
    char *get_lineinfo_filename();
    bool get_abbrevs();
    const dwarf_abbrev_t *find_abbrev(unsigned long num) const;
    char *get_compunit_filename();

    /* contents of .debug_line section */
    dwarf_stream_t line_sec_;
    unsigned long lineinfo_off_;
    ptrarray_t<char> *dir_table_;

    /* contents of .debug_abbrev section */
    dwarf_stream_t abbrev_sec_;
    vector<dwarf_abbrev_t> abbrevs_;

    /* contents of .debug_info section */
    dwarf_stream_t info_sec_;
    uint64_t compunit_off_;

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

bool
cov_dwarf2_filename_scanner_t::attach(cov_bfd_t *b)
{
    cov_bfd_section_t *sec;

    if (!cov_filename_scanner_t::attach(b))
	return false;

    if (!line_sec_.init_from_section(cbfd_, ".debug_line"))
	return false;

    if (!abbrev_sec_.init_from_section(cbfd_, ".debug_abbrev"))
	return false;

    if (!info_sec_.init_from_section(cbfd_, ".debug_info"))
	return false;

    if ((sec = cbfd_->find_section(".debug_str")) == 0 ||
	(strings_ = (char *)sec->get_contents(&string_size_)) == 0)
	return false;

    return true;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* read an external lineinfo struct, opcode lengths & dir table */
bool
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
	return false;
    /* TODO: bounds check length */
    lineinfo_off_ = line_sec_.tell() + length;

    if (!line_sec_.get_uint16(&version))
	return false;
    switch (version)
    {
    case 2:
	break;
    default:
	_log.warning("Cannot handle DWARF version %u\n", version);
	return false;
    }
    if (!line_sec_.get_uint32(/*prologue_length*/0) ||
	!line_sec_.get_uint8(/*min_insn_length*/0) ||
	!line_sec_.get_uint8(/*default_is_stmt*/0) ||
	!line_sec_.get_uint8(/*line_base*/0) ||
	!line_sec_.get_uint8(/*line_range*/0) ||
	!line_sec_.get_uint8(&opcode_base))
	return false;

    /* read the opcode lengths */
    if (!line_sec_.skip(opcode_base-1))
	return false;

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
	    return false;
	if (*dir == '\0')
	    break;
	dir_table_->append(g_strdup(dir));

	_log.debug("dir_table[%d] = \"%s\"\n",
		   dir_table_->length(),
		   dir_table_->nth(dir_table_->length()-1));
    }

    return true;
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
	    g_strconcat(dir_table_->nth(diridx-1), "/", file, (char *)0) :
	    g_strdup(file));
    _log.debug("get_lineinfo_filename() = \"%s\"\n", path);
    return path;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

bool
cov_dwarf2_filename_scanner_t::get_abbrevs()
{
    unsigned long code, tag, name, form;
    dwarf_abbrev_t *abbrev;
    dwarf_attr_t *attr;

    _log.debug("get_abbrevs:\n");

    for (;;)
    {
	if (!abbrev_sec_.get_varint(&code))
	    return false;

	if (code == 0)
	    break;    /* end of section */

	if (!abbrev_sec_.get_varint(&tag))
	    return false;
	if (!abbrev_sec_.get_uint8(/*children*/0))
	    return false;

	abbrev = new dwarf_abbrev_t(code, tag);
	abbrevs_.push_back(abbrev);

	_log.debug("abbrev entry=%d tag=%d {\n", abbrev->entry_, abbrev->tag_);

	for (;;)
	{
	    if (!abbrev_sec_.get_varint(&name))
		return false;
	    if (!abbrev_sec_.get_varint(&form))
		return false;
	    if (!name)
		break;
	    attr = new dwarf_attr_t(name, form);
	    abbrev->attrs_.append(attr);

	    _log.debug("    attr=%d form=%d\n", attr->tag_, attr->form_);
	}
	_log.debug("}\n");
    }

    _log.debug("get_abbrevs: end of section\n");
    return true;
}

const dwarf_abbrev_t *
cov_dwarf2_filename_scanner_t::find_abbrev(unsigned long num) const
{
    for (vector<dwarf_abbrev_t>::const_iterator iter = abbrevs_.begin() ; iter != abbrevs_.end() ; ++iter)
    {
	const dwarf_abbrev_t *abbrev = *iter;
	if (abbrev->entry_ == num)
	    return abbrev;
    }
    return 0;
}

struct dwarf_value_t
{
    dwarf_value_t()
    {
	memset(this, 0, sizeof(*this));
    }
    dwarf_value_t()
    {
	reset();
    }

    enum type_t
    {
	T_unknown,  // = 0
	T_uint,
	T_string,
	T_bytes
    };
    void set_uint(uint64_t v)
    {
	reset();
	type_ = T_uint;
	uint_ = v;
	ptr_ = 0;
    }
    void set_string(const char *s)
    {
	reset();
	type_ = T_string;
	uint_ = 0;
	ptr_ = (s == 0 ? 0 : strdup(s));
    }
    void set_string(const char *s, size_t len)
    {
	reset();
	type_ = T_string;
	uint_ = 0;
	if (!len)
	{
	    ptr_ = 0;
	}
	else
	{
	    ptr_ = gnb_xmalloc(len+1);
	    memcpy(ptr_, s, len);
	    ptr_[len] = '\0';
	}
    }
    void set_bytes(uint8_t *val, size_t len)
    {
	reset();
	type_ = T_bytes;
	uint_ = len;
	if (!len)
	{
	    ptr_ = 0;
	}
	else
	{
	    ptr_ = gnb_xmalloc(len);
	    memcpy(ptr_, s, len);
	}
    }

private:
    type_t type_;
    uint64_t uint_;
    char *ptr_;

    void reset()
    {
	if (type_ != T_unknown)
	{
	    free(ptr_);
	    memset(this, 0, sizeof(*this));
	}
    }
}

struct dwarf_die_t
{
    unsigned int tag;
    const dwarf_abbrev_t *abbrev;
    vector<dwarf_value_t> values;
};


bool
cov_dwarf2_filename_scanner_t::read_die(dwarf_stream_t &sec, dwarf_die_t *diep)
{
    unsigned long abbrevno;
    const dwarf_abbrev_t *abbrev;

    if (!sec.get_varint(&abbrevno) ||
	abbrevno == 0 ||
	(abbrev = find_abbrev(abbrevno)) == 0)
	return false;

    die->abbrev = abbrev;
    die->tag = abbrev->tag_;
    die->values.resize(abbrev->attrs_.size());

    int i;
    for (vector<dwarf_attr_t>::const_iterator iter = abbrev->attrs_.begin() ; iter != abbrev->attrs_.end() ; ++iter, ++i)
    {
	const dwarf_attr_t *attr = *iter;
	dwarf_value_t *value = &die->values[i];
	switch (attr->form_)
	{
	case DW_FORM_address:
	    {
		void *addr;
		if (!sec.get_address(&addr))
		    return false;
		value->set_uint((uint64_t)addr);
	    }
	    break;
	}
	case DW_FORM_block1:
	    {
		uint8_t len;
		if (!sec.get_uint8(&len))
		    return false;
		if (!sec.get_bytes(len))
		    return false;
	    }
	    break;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_dwarf2_filename_scanner_t::get_compunit_filename()
{
    uint64_t length;
    bool is64b = false;
    uint16_t version;
    uint32_t name_off, comp_dir_off, producer_off;
    char *path;

    /* read an external CompUnit structure */
    _log.debug("get_compunit_filename()\n");

    if (compunit_off_)
	info_sec_.seek(compunit_off_);
_log.debug("XXX %u\n", __LINE__);

    /* Read the compilation unit header */
    if (!info_sec_.get_unit_header_length(&length, &is64b))
	return 0;
_log.debug("XXX %u\n", __LINE__);
    /* TODO: bounds check length */
    compunit_off_ = info_sec_.tell() + length;

_log.debug("XXX %u\n", __LINE__);
    if (!info_sec_.get_uint16(&version))
	return 0;
    switch (version)
    {
    case 2:
	if (is64b)
	{
	    _log.error("Found 64b length in DWARF2 unit header\n");
	    return 0;
	}
	break;
    case 4:
	break;
    default:
	_log.warning("Cannot handle DWARF version %u\n", version);
	return 0;
    }
_log.debug("XXX %u\n", __LINE__);
    if (!info_sec_.get_offset(/*abbrev_offset*/0) ||
	!info_sec_.get_uint8(/*pointer_size*/0))
	return 0;
_log.debug("XXX %u\n", __LINE__);

    /* The first debug entry is a DW_TAG_compile_unit */

    if (!read_die(info_sec_))
	return 0;
    if (die.tag != DW_TAG_compile_unit)
	return 0;

_log.debug("XXX %u\n", __LINE__);

    /* read hardcoded DW_TAG_compile_unit */
    if (!info_sec_.get_uint32(/*stmt_list*/0) ||
	!info_sec_.get_address(/*high_pc*/0) ||
	!info_sec_.get_address(/*low_pc*/0) ||
	!info_sec_.get_uint32(&name_off) ||
	!info_sec_.get_uint32(&comp_dir_off) ||
	!info_sec_.get_uint32(&producer_off))
	return 0;
_log.debug("XXX %u\n", __LINE__);

    if (name_off > string_size_ ||
	comp_dir_off > string_size_ ||
	producer_off > string_size_)
	return 0;
_log.debug("XXX %u\n", __LINE__);

    path = g_strconcat(
		strings_+comp_dir_off,
		"/",
		strings_+name_off,
		(char *)0);
    _log.debug("get_compunit_filename() = \"%s\"\n", path);
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
#if 0
	case 0: /* scanning LineInfo in .debug_line */
	    state_ = (get_lineinfo() ? 1 : 2);
	    break;
	case 1: /* scanning filename table in .debug_line */
	    if ((s = get_lineinfo_filename()) != 0)
		return s;
	    state_ = 0;         /* try for another LineInfo */
	    break;
#else
	case 0:
	    state_ = 2;
	    break;
#endif
	case 2: /* scanning .debug_abbrev etc */
	    if (!get_abbrevs())
		return 0;       /* end of iteration */
	    state_ = 3;         /* start scanning compunits */
	    break;
	case 3: /* scanning CompUnits in .debug_info */
	    return get_compunit_filename();
	default:
	    return 0;   /* end of iteration */
	}
    }

    return 0;   /* end of iteration */
}

#endif /*HAVE_LIBBFD*/
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/

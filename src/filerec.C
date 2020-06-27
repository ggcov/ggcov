/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2003-2020 Greg Banks <gnb@fastmail.fm>
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

#include "filerec.H"
#include "tok.H"

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

file_rec_t::file_rec_t()
{}

file_rec_t::file_rec_t(const char *nm, cov_file_t *f)
 :  name_(nm), file_(f)
{
    if (file_ != 0)
	scope_ = new cov_file_scope_t(file_);
    else
	scope_ = new cov_compound_scope_t();
}

file_rec_t::~file_rec_t()
{
    delete scope_;
    children_.delete_all();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

file_rec_t *file_rec_t::find_child(const char *name) const
{
    for (list_iterator_t<file_rec_t> friter = children_.first() ; *friter ; ++friter)
    {
	if (!strcmp((*friter)->name_, name))
	    return *friter;
    }
    return 0;
}

void file_rec_t::add_child(file_rec_t *child)
{
    assert(is_directory());
    children_.append(child);
    ((cov_compound_scope_t *)scope_)->add_child(child->scope_);
}

file_rec_t *file_rec_t::add_descendent(cov_file_t *f)
{
    cov::status_t st = f->status();

    if (st == cov::SUPPRESSED || st == cov::UNINSTRUMENTED)
	return 0;

    file_rec_t *parent, *fr = 0;
    char *buf = g_strdup(f->minimal_name());
    char *end = buf + strlen(buf);
    tok_t tok(buf, "/");
    const char *part;

    parent = this;

    while ((part = tok.next()) != 0)
    {
	if (part + strlen(part) == end)
	{
	    parent->add_child(fr = new file_rec_t(part, f));
	}
	else
	{
	    fr = parent->find_child(part);
	    if (!fr)
		parent->add_child(fr = new file_rec_t(part, 0));
	}
	parent = fr;
    }
    return fr;
}

void file_rec_t::add_descendents(list_iterator_t<cov_file_t> iter)
{
    for ( ; *iter ; ++iter)
	add_descendent(*iter);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void file_rec_t::dump(int indent, logging::logger_t &_log)
{
    int i;
    estring indentbuf;

    for (i = 0 ; i < indent ; i++)
	indentbuf.append_char(' ');
    _log.debug("%s%s\n", indentbuf.data(), name_.data());

    for (list_iterator_t<file_rec_t> friter = children_.first() ; *friter ; ++friter)
	(*friter)->dump(indent+4, _log);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/

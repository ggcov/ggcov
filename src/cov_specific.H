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

#ifndef _ggcov_specific_H_
#define _ggcov_specific_H_ 1

/*
 * This header file contains the base classes for all the
 * machine-specific parts of the coverage data reading code.
 */

#include "common.h"

#ifdef HAVE_LIBBFD
#include <bfd.h>
#endif

#ifdef HAVE_ELF_H
#include <elf.h>
#endif

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*
 * Abstract factory for building concrete instances of various
 * abstract machine-specific classes.  The leaf classes declare
 * themselves statically by invoking the COV_FACTORY_STATIC_REGISTER()
 * macro in global scope.
 * TODO: provide a way to register dynamically.
 */

class cov_factory_item_t
{
private:
    static cov_factory_item_t *all_;

    cov_factory_item_t *next_;
    int category_;
    const char *name_;
    void *(*creator_)();
    
    friend class cov_factory_item_iterator_t;

public:
    cov_factory_item_t(int cat, const char *name, void *(*creator)())
     :  category_(cat),
     	name_(name),
	creator_(creator)
    {
    	next_ = all_;
	all_ = this;
    }
    
    int category() const
    {
    	return category_;
    }
    const char *name() const
    {
    	return name_;
    }
    void *create() const
    {
    	return (*creator_)();
    }
};

class cov_factory_item_iterator_t
{
private:
    int category_;
    cov_factory_item_t *elem_;
    
public:
    cov_factory_item_iterator_t(int category)
     :  category_(category)
    {
    	for (elem_ = cov_factory_item_t::all_ ;
	     elem_ != 0 && elem_->category() != category_ ;
	     elem_ = elem_->next_)
	    ;
    }
    ~cov_factory_item_iterator_t()
    {
    }
    
    cov_factory_item_t *current() const
    {
    	return elem_;
    }

    cov_factory_item_t *next()
    {
    	do
	{
	     elem_ = (elem_ == 0 ? 0 : elem_->next_);
	}
	while (elem_ != 0 && elem_->category() != category_);
	return elem_;
    }
};

template<class T> class cov_factory_t
{
private:
    cov_factory_item_iterator_t iter_;
    
public:
    cov_factory_t()
     :  iter_(T::factory_category())
    {
    }
    
    ~cov_factory_t()
    {
    }

    T *create() const
    {
    	return (iter_.current() == 0 ? 0 : (T *)iter_.current()->create());
    }

    gboolean next()
    {
    	return (iter_.next() != 0);
    }
    
    const char *name() const
    {
    	return (iter_.current() == 0 ? 0 : iter_.current()->name());
    }
};


#define PASTE(x,y)  	x##y
#define STRINGIFY(x)	#x

#define COV_FACTORY_STATIC_REGISTER(superklass, klass) \
    static void * PASTE(klass,_creator)() { return new klass (); } \
    cov_factory_item_t PASTE(klass,_static_reg)( \
    	superklass::factory_category(), \
	STRINGIFY(klass), PASTE(klass,_creator));

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/*
 * Base class for classes which scan an object or executable
 * looking for recorded source filenames.  Used to implement
 * handling an executable or object file on the commandline.
 */
class cov_filename_scanner_t
{
public:
    cov_filename_scanner_t();
    virtual ~cov_filename_scanner_t();
    
    /* tries to open the object filename */
    virtual gboolean open(const char *filename);

    /* Iteration interface: returns new strings until it returns NULL */
    virtual char *next() = 0;
    
    static int factory_category();

protected:
#ifdef HAVE_LIBBFD
    /* utility functions */
    void *get_section(const char *name, bfd_size_type *lenp);

    bfd *abfd_;
#endif
};

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

#endif /* _ggcov_specific_H_ */

/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2010 Greg Banks <gnb@users.sourceforge.net>
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

#include "cov.H"
#include "cov_specific.H"
#include "estring.H"
#include "string_var.H"
#include "filename.h"
#include "mvc.h"
#include "tok.H"
#include <dirent.h>

list_t<cov_project_t> cov_project_t::all_;
cov_project_t *cov_project_t::current_;

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_project_t::cov_project_t(const char *name, const char *basedir)
 :  name_(name),
    basedir_(basedir),
    files_(new hashtable_t<const char, cov_file_t>),
    common_path_(0),
    common_len_(0)
{
    all_.append(this);
    if (current_ == 0)
	current_ = this;
}

cov_project_t::~cov_project_t()
{
    all_.remove(this);
    if (current_ == this)
	current_ = 0;
    // TODO: destroy files_ hashtable etc
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_project_t *
cov_project_t::current()
{
    return current_;
}

void
cov_project_t::make_current()
{
    current_ = this;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

char *
cov_project_t::get_pathname(const char *filename) const
{
    // TODO: ensure @filename is relative and does not escape from basedir
    /* Note: deliberately relative to "." if basedir_ == NULL */
    return file_make_absolute_to_dir(filename, basedir_);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_project_t::pre_read(void)
{
}

void
cov_project_t::post_read_1(
    const char *name,
    cov_file_t *f,
    gpointer userdata)
{
    cov_project_t *proj = (cov_project_t *)userdata;

    proj->files_list_.prepend(f);
    f->finalise();
}

static int
compare_files(const cov_file_t *fa, const cov_file_t *fb)
{
    return strcmp(fa->minimal_name(), fb->minimal_name());
}

void
cov_project_t::post_read(void)
{
    list_iterator_t<cov_file_t> iter;

    /* construct the list of filenames */
    files_list_.remove_all();
    files_->foreach(post_read_1, this);
    files_list_.sort(compare_files);

    /* Build the callgraph */
    /* TODO: only do this to newly read files */
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	cov_add_callnodes(*iter);
    for (iter = cov_file_t::first() ; iter != (cov_file_t *)0 ; ++iter)
    	cov_add_callarcs(*iter);

    /* emit an MVC notification */
    mvc_changed(this, 1);
}

cov_file_t *
cov_project_t::find_file(const char *name)
{
    assert(files_ != 0);
    string_var fullname = unminimise_name(name);
    return files_->lookup(fullname);
}


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_project_t::read_source_file_2(const char *fname, gboolean quiet)
{
    cov_file_t *f;
    string_var filename = get_pathname(fname);

    dprintf1(D_FILES, "Handling source file %s\n", filename.data());

    if ((f = cov_file_t::find(filename)) != 0)
    {
    	if (!quiet)
    	    fprintf(stderr, "Internal error: handling %s twice\n", filename.data());
    	return FALSE;
    }

    f = new cov_file_t(filename, fname);

    if (!f->read(quiet))
    {
	delete f;
	return FALSE;
    }

    return TRUE;
}

gboolean
cov_project_t::read_source_file(const char *filename)
{
    return read_source_file_2(filename, /*quiet*/FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_project_t::read_shlibs(cov_bfd_t *b, int depth)
{
    cov_factory_t<cov_shlib_scanner_t> factory;
    cov_shlib_scanner_t *ss;
    string_var file;
    int successes = 0;

    dprintf1(D_FILES, "Scanning \"%s\" for shared libraries\n",
    	     b->filename());

    do
    {
    	dprintf1(D_FILES, "Trying scanner %s\n", factory.name());
    	if ((ss = factory.create()) != 0 && ss->attach(b))
	    break;
	delete ss;
	ss = 0;
    }
    while (factory.next());

    if (ss == 0)
    	return FALSE;	/* no scanner can open this file */

    /*
     * TODO: instead of using the first scanner that succeeds open()
     *       use the first one that returns any results.
     */
    while ((file = ss->next()) != 0)
    {
    	dprintf1(D_FILES, "Trying filename %s\n", file.data());
	if (read_one_object_file(file, depth))
	    successes++;
    }

    delete ss;
    return (successes > 0);
}

gboolean
cov_project_t::read_one_object_file(const char *exefilename, int depth)
{
    cov_bfd_t *b;
    cov_filename_scanner_t *fs;
    string_var dir;
    string_var file;
    int successes = 0;

    dprintf1(D_FILES, "Scanning object or exe file \"%s\"\n", exefilename);

    if ((b = new cov_bfd_t()) == 0)
    	return FALSE;
    if (!b->open(exefilename))
    {
    	delete b;
	return FALSE;
    }

    cov_factory_t<cov_filename_scanner_t> factory;
    do
    {
    	dprintf1(D_FILES, "Trying scanner %s\n", factory.name());
    	if ((fs = factory.create()) != 0 && fs->attach(b))
	    break;
	delete fs;
	fs = 0;
    }
    while (factory.next());

    if (fs == 0)
    {
    	delete b;
    	return FALSE;	/* no scanner can open this file */
    }

    dir = file_dirname(exefilename);
    cov_add_search_directory(dir);

    /*
     * TODO: instead of using the first scanner that succeeds open()
     *       use the first one that returns any results.
     */
    while ((file = fs->next()) != 0)
    {
    	dprintf1(D_FILES, "Trying filename %s\n", file.data());
	if (cov_is_source_filename(file) &&
	    file_is_regular(file) == 0 &&
	    read_source_file_2(file, /*quiet*/TRUE))
	    successes++;
    }

    delete fs;

    successes += read_shlibs(b, depth+1);

    if (depth == 0 && successes == 0)
    	fprintf(stderr, "found no coveraged source files in executable \"%s\"\n",
	    	exefilename);
    delete b;
    return (successes > 0);
}

gboolean
cov_project_t::read_object_file(const char *exefilename)
{
    return read_one_object_file(exefilename, 0);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

unsigned int
cov_project_t::read_directory_2(
    const char *dirname,
    gboolean recursive,
    gboolean quiet)
{
    DIR *dir;
    struct dirent *de;
    int dirlen;
    unsigned int successes = 0;

    estring child = dirname;
    dprintf1(D_FILES, "Scanning directory \"%s\"\n", child.data());

    if ((dir = opendir(dirname)) == 0)
    {
    	perror(child.data());
    	return 0;
    }

    while (child.last() == '/')
	child.truncate_to(child.length()-1);
    if (!strcmp(child, "."))
	child.truncate();
    else
	child.append_char('/');
    dirlen = child.length();

    while ((de = readdir(dir)) != 0)
    {
    	if (!strcmp(de->d_name, ".") ||
	    !strcmp(de->d_name, ".."))
	    continue;

	child.truncate_to(dirlen);
	child.append_string(de->d_name);

    	if (file_is_regular(child) == 0 &&
	    cov_is_source_filename(child))
	    successes += read_source_file_2(child, /*quiet*/TRUE);
	else if (recursive && file_is_directory(child) == 0)
	    successes += read_directory_2(child, recursive, /*quiet*/TRUE);
    }

    closedir(dir);
    if (successes == 0 && !quiet)
    {
	if (recursive)
    	    fprintf(stderr, "found no coveraged source files in or under directory \"%s\"\n",
	    	    dirname);
	else
    	    fprintf(stderr, "found no coveraged source files in directory \"%s\"\n",
	    	    dirname);
    }
    return successes;
}

unsigned int
cov_project_t::read_directory(const char *dirname, gboolean recursive)
{
    return read_directory_2(dirname, recursive, /*quiet*/FALSE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_project_t::read_file(const char *filename, gboolean recursive)
{
    string_var path = get_pathname(filename);

    if (file_is_directory(path) == 0)
    {
	if (!read_directory(path, recursive))
	    return FALSE;
    }
    else if (file_is_regular(path) == 0)
    {
	if (cov_is_source_filename(path))
	{
	    if (!read_source_file(path))
		return FALSE;
	}
	else
	{
	    if (!read_object_file(path))
		return FALSE;
	}
    }
    else
    {
    	/* TODO: gui alert to user */
	fprintf(stderr, "%s: don't know how to handle this filename\n",
		path.data());
	return FALSE;
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_project_t::read_files(GList *files, gboolean recursive)
{
    GList *iter;

    if (files == 0)
    {
	const char *dir = basedir_;
	if (!dir)
	    dir = ".";
	else
	    cov_add_search_directory(dir);
    	if (!read_directory(dir, recursive))
	    return FALSE;
    }
    else
    {
	for (iter = files ; iter != 0 ; iter = iter->next)
	{
	    const char *filename = (const char *)iter->data;

	    if (file_is_directory(filename) == 0)
	    	cov_add_search_directory(filename);
	    /* TODO: search dir is now wrong scope, should be project not global */
    	}

	for (iter = files ; iter != 0 ; iter = iter->next)
	{
	    const char *filename = (const char *)iter->data;

	    if (!read_file(filename, recursive))
		return FALSE;
	}
    }

    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cov_project_t::read_all_files()
{
    return read_files(/*files*/0, /*recursive*/TRUE);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cov_project_t *
cov_project_t::get(const char *name)
{
    if (!name || !*name)
	return 0;

    list_iterator_t<cov_project_t> itr;
    for (itr = all_.first() ; itr != (cov_project_t *)0 ; ++itr)
    {
	if (!strcmp((*itr)->name_, name))
	    return *itr;
    }
    return 0;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

static gboolean
filename_matches_directory_prefix(const char *filename, const char *dir)
{
    int dirlen = strlen(dir);

    return (!strncmp(dir, filename, dirlen) &&
	    (filename[dirlen] == '\0' || filename[dirlen] == '/'));
}

static gboolean
filename_is_common(const char *filename)
{
    static const char * const uncommon_dirs[] = 
    {
	"/usr/include",
	"/usr/lib",
	0
    };
    const char * const * dirp;

    for (dirp = uncommon_dirs ; *dirp != 0 ; dirp++)
    {
    	if (filename_matches_directory_prefix(filename, *dirp))
	    return FALSE;
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_project_t::attach_file(cov_file_t *f)
{
    files_->insert(f->name_, f);
    f->common_ = filename_is_common(f->name_);
    if (f->common_)
	add_name(f->name_);
}

void
cov_project_t::detach_file(cov_file_t *f)
{
    files_list_.remove(f);
    files_->remove(f->name_);
    if (f->common_)
	dirty_common_path();
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cov_project_t::add_name(const char *name)
{
    assert(name[0] == '/');
    if (common_path_ == 0)
    {
    	/* first filename: initialise the common path to the directory */
	char *p;
    	common_path_ = g_strdup(name);
	if ((p = strrchr(common_path_, '/')) != 0)
	    p[1] = '\0';
    }
    else
    {
    	/* subsequent filenames: shrink common path as necessary */
	char *cs, *ce, *ns, *ne;
	cs = common_path_+1;
	ns = (char *)name+1;
	for (;;)
	{
	    if ((ne = strchr(ns, '/')) == 0)
	    	break;
	    if ((ce = strchr(cs, '/')) == 0)
	    	break;
	    if ((ce - cs) != (ne - ns))
	    	break;
	    if (memcmp(cs, ns, (ne - ns)))
	    	break;
	    cs = ce+1;
	    ns = ne+1;
	}
	*cs = '\0';
    }
    common_len_ = strlen(common_path_);
    dprintf2(D_FILES, "cov_project_t::add_name: name=\"%s\" => common=\"%s\"\n",
    	    	name, common_path_);
}

void
cov_project_t::dirty_common_path()
{
    if (common_path_ != 0)
    {
	g_free(common_path_);
	common_path_ = 0;
	common_len_ = -1;   /* indicates dirty */
    }
}

void
cov_project_t::add_name_tramp(
    const char *name,
    cov_file_t *f,
    gpointer userdata)
{
    cov_project_t *proj = (cov_project_t *)userdata;

    if (f->common_)
	proj->add_name(name);
}

void
cov_project_t::check_common_path()
{
    if (common_len_ < 0)
    {
    	dprintf0(D_FILES, "cov_project_t::check_common_path: recalculating common path\n");
    	common_len_ = 0;
	files_->foreach(add_name_tramp, 0);
    }
}

const char *
cov_project_t::minimal_name(const char *name)
{
    check_common_path();
    if (!strncmp(name, common_path_, common_len_))
	return name + common_len_;
    else
	return name;
}

char *
cov_project_t::minimise_name(const char *name)
{
    return g_strdup(minimal_name(name));
}

char *
cov_project_t::unminimise_name(const char *name)
{
    if (name[0] == '/')
    {
    	/* absolute name */
    	return g_strdup(name);
    }
    else
    {
    	/* partial, presumably minimal, name */
    	check_common_path();
	return g_strconcat(common_path_, name, (char *)0);
    }
}

const char *
cov_project_t::common_path()
{
    check_common_path();
    return common_path_;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/

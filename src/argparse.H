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

#ifndef __ggcov_argparse_H__
#define __ggcov_argparse_H__ 1

#include "ptrarray.H"
#include "hashtable.H"
#include "ptrarray.H"
#include "string_var.H"
#include "tok.H"
#include "fakepopt.h"
#if HAVE_GTK_INIT_WITH_ARGS
#include <glib.h>
#endif

//
// argparse is an argument parser based loosely on the
// Java and Python argument parsing libraries.
//
// To use it, create a argparse::parser_t with your params object,
// add options using add_option(), call parse() with an argv[]
// vector, and then examine your params object.
//
// Params objects hold the accumulated options and non-option
// arguments.  You can either use the provided simple_params_t
// which stores key/value pairs in a hashtable and allows you
// to check whether options were set and what their values were
// by the option's long name, or alternatively provide your own
// by subclassing from params_t.  Providing your own allows
// finer control over the options, in particular you can
// implement your own get/set methods with whatever logic you
// want.
//
// To add an option, you create an option_t object using
// parser.add_option() and configure it by calling
// name(), description() etc.  These methods are setup so
// you can chain the calls, e.g.
//
//     argparse::simple_params_t params;
//     argparse::parser_t parser(params);
//     parser.add_option('f').description("specify the format").with_value();
//     parser.add_option('o').description("specify the output file").with_value();
//     parser.add_option('v').description("be more verbose");
//     parser.parse(argc, argv);
//     if (params.has('v')) { ... }
//     if (params.has('f')) { ... format = params.value('f') ...  }
//

namespace argparse
{

class parser_t;

class params_t
{
public:
    params_t();
    virtual ~params_t();

    virtual bool set(const char *key, const char *value);

    virtual void add_file(const char *file);
    virtual unsigned int num_files() const;
    virtual const char *nth_file(unsigned i) const;
    typedef ptrarray_iterator_t<const char> file_iterator_t;
    virtual file_iterator_t file_iter() const;

    // You can override this method to do the adding of options to
    // the parser for you; handy for inheriting from other params
    // objects.  Called from the parser_t c'tor.
    virtual void setup_parser(parser_t &);

    // You can override this method to do any kind of logical
    // processing needed after the commandline has been parsed.
    virtual void post_args();

private:
    ptrarray_t<const char> *files_;
};

typedef void (params_t::*noarg_setter_t)();
typedef void (params_t::*arg_setter_t)(const char *);

class simple_params_t : public params_t
{
public:
    simple_params_t();
    virtual ~simple_params_t();

    virtual bool set(const char *key, const char *value);
    bool has(const char *key) const;
    const char *value(const char *key) const;

    virtual void post_args();

private:
    static gboolean delete_one(const char *name, char*, gpointer userdata);
    hashtable_t<const char, char> *values_;
};

class option_t
{
private:
    option_t(char short_option, const char *long_option);
    ~option_t();

    bool set(params_t &params, const char *arg) const;
    char *describe() const;

    // defaults to long_option_ or short_option_ if long_option_ is not set
    string_var name_;
    char short_option_;
    string_var long_option_;
    string_var description_;
    string_var metavar_;
    enum
    {
	F_SETTER=(1<<0),	// 1:call the setter_ function 0:call params_t::set() with option's name
	F_ARG=(1<<1)	// 1:setter has argument, 0:no argument
    };
    unsigned int flags_;
    arg_setter_t setter_;

    friend class parser_t;

public:

    // Specify a setter function which will be called with no
    // arguments when this option is parsed from the commandline.
    // Implies without_arg().  If no setter function is specified,
    // the generic params_t::set() method will be called with this
    // option's name as the first argument when this option is
    // parsed from the commandline.
    option_t &setter(noarg_setter_t v)
    {
	setter_ = (arg_setter_t)v;
	flags_ |= F_SETTER;	    /* has setter function */
	flags_ &= ~F_ARG;	    /* setter function has no argument */
	return *this;
    }

    // Specify a setter function which will be called with a single
    // string argument when this option is parsed from the
    // commandline.  Implies with_arg().  If no setter function is
    // specified, the generic params_t::set() method will be called
    // with this option's name as the first argument when this
    // option is parsed from the commandline.
    option_t &setter(arg_setter_t v)
    {
	setter_ = v;
	flags_ |= F_SETTER;	    /* has setter function */
	flags_ |= F_ARG;	    /* setter function has 1 argument */
	return *this;
    }

    // Specify the option's name.  The name defaults to the options
    // long_option, or to a string form of it's short_option if the
    // long_option is not specified.  The name is only used in the
    // generic params_t::set() method, which is only called if no
    // setter function is specified using either of the setter()
    // methods.
    option_t &name(const char *v)
    {
	name_ = v;
	return *this;
    }

    // Specify the option's description.  This is text that will be
    // used to format the message which is automatically printed if
    // the --help option is parsed on the commandline.
    option_t &description(const char *v)
    {
	description_ = v;
	return *this;
    }

    // Specify that the option does not take an argument.  This
    // is the default.
    option_t &without_arg()
    {
	flags_ &= ~F_ARG;	    /* setter/generic has no argument */
	return *this;
    }

    // Specify that the option takes an argument.
    option_t &with_arg()
    {
	flags_ |= F_ARG;	    /* setter/generic has 1 argument */
	return *this;
    }

    // Specify the string that will be used to stand in for the
    // argument in automatically generated help messages.  Default
    // is "ARG".  Only used if the option takes an argument.
    option_t &metavar(const char *v)
    {
        metavar_ = v;
        return *this;
    }

    // accessors
    char short_option() const { return short_option_; }
    const char *long_option() const { return long_option_.data(); }
    gboolean has_argument() const { return !!(flags_ & F_ARG); }
    const char *description() const { return description_.data(); }
    const char *metavar() const
    {
        return (!has_argument() ? NULL :
                (metavar_.data() ? metavar_.data() : "ARG"));
    }
};

/*
 * Abstract base class for argument parsers.
 */
class parser_t
{
public:
    parser_t(params_t &params);
    ~parser_t();

    // Add an option with GNU-style short and long names.
    // Either, but not both, of the long or short names may
    // be passed as 0 to leave them unspecified.
    option_t &add_option(char short_name, const char *long_name=0);

    void set_other_option_help(const char *v) { other_option_help_ = v; }

    virtual int parse(int argc, char **argv) = 0;

    option_t *get_nth_option(unsigned int i) const { return options_->nth(i); }
    /* Given a string like either "-f" for short option or "--foo" for
     * long option, return an option_t or NULL if it doesn't exist */
    option_t *get_option_by_name(const char *name) const { return options_by_name_->lookup((char *)name); }

    bool set(const option_t *o, const char *arg) { return o->set(params_, arg); }

protected:
    params_t &params_;
    ptrarray_t<option_t> *options_;
    hashtable_t<char, option_t> *options_by_name_;    /* indexed by -SHORT and --LONG */
    string_var other_option_help_;
};

/*
 * This derived parser class is for using the POPT library API to parse
 * arguments, either the real one or our faked local reimplementation.
 */
class popt_parser_t : public parser_t
{
public:
    popt_parser_t(params_t &params);
    ~popt_parser_t();

    int parse(int argc, char **argv);

    // This is public so we can integrate with GNOME 1.x
    struct poptOption *get_popt_table();
    void handle_popt_tail(poptContext popt_context);

private:
    static void popt_callback(poptContext con,
			      enum poptCallbackReason reason,
			      const struct poptOption *opt,
			      const char *arg,
			      const void *data);
    struct poptOption *popt_options_;
};

/*
 * This typedef is used by CLIs which don't need to care about details of
 * parser implementations or integrating with GTK or GNOME, they just setup
 * some options and call parse().  We have our own implementation of the popt
 * API so we can just always assume that works and use it.
 */
typedef popt_parser_t default_parser_t;

#if HAVE_GTK_INIT_WITH_ARGS

/*
 * This derived parser class is for using the GOption feature of modern
 * versions of the GLib library to parse arguments.
 */
class goption_parser_t : public parser_t
{
public:
    goption_parser_t(params_t &params);
    ~goption_parser_t();

    virtual int parse(int argc, char **argv);
    GOptionEntry *get_goption_table();

    void post_args() { params_.post_args(); }

private:
    static gboolean goption_callback(const char *option_name,
                                     const char *value,
                                     gpointer data,   /* if only we could control this */
                                     GError **error);

    /* This class needs to be a singleton because gtk_init_with_args()
     * does not provide a way to set the GOption callback closure.  */
    static goption_parser_t *instance_;
    GOptionEntry *goptions_;
};

#endif

#ifndef _PASTE_
#define _PASTE_(x,y) x##y
#endif

// Convenience macro to be used in the declaration of a params class
// which derives from params_t.  It declares and  defines a string
// variable and getter and setter methods for it.  The setter method
// will be called set_foo() where foo is the argument to this macro, and
// is suitable for passing to option_t::setter().
#define ARGPARSE_STRING_PROPERTY(nm) \
    public: \
    void _PASTE_(set_,nm)(const char *v) { _PASTE_(nm,_) = v; } \
    const char *_PASTE_(get_,nm)() const { return _PASTE_(nm,_); } \
    private: \
    string_var _PASTE_(nm,_)

// Convenience macro to be used in the declaration of a params class
// which derives from params_t.  It declares and defines a variable
// which is a list of strings and getter and setter methods for it.
// The setter method will be called set_foo() where foo is the argument
// to this macro, and is suitable for passing to option_t::setter().
// The setter actually *appends* rather than setting.  Values are
// are split by commas and whitespace.
#define ARGPARSE_STRINGLIST_PROPERTY(nm) \
    public: \
    void _PASTE_(set_,nm)(const char *v) { \
        tok_t t(v, ", \t"); \
        const char *p; \
        while ((p = t.next())) { \
            _PASTE_(nm,_).append(g_strdup(p)); \
        } \
    } \
    const list_t<char> &_PASTE_(get_,nm)() const { return _PASTE_(nm,_); } \
    private: \
    list_t<char> _PASTE_(nm,_)
    /* TODO: this will be leaked because it needs an explicitly coded dtor */

// Convenience macro to be used in the declaration of a params class
// which derives from params_t.  It declares and defines a boolean
// variable and getter and setter methods for it.  The setter method
// will be called set_foo() where foo is the argument to this macro, and
// is suitable for passing to option_t::setter().  It also declares a
// clear_foo() method which is also suitable for passing to
// option_t::setter().
#define ARGPARSE_BOOL_PROPERTY(nm) \
    public: \
    void _PASTE_(set_,nm)() { _PASTE_(nm,_) = true; } \
    void _PASTE_(clear_,nm)() { _PASTE_(nm,_) = false; } \
    bool _PASTE_(get_,nm)() const { return _PASTE_(nm,_); } \
    private: \
    bool _PASTE_(nm,_)

// close the namespace
}

#endif /* __ggcov_argparse_H__ */

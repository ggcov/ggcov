/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2004-2005 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_cpp_parser_H_
#define _ggcov_cpp_parser_H_ 1

#include "estring.H"
#include "string_var.H"
#include "list.H"
#include <tr1/unordered_map>
#include <string>

class cpp_parser_t
{
public:
    cpp_parser_t(const char *filename);
    virtual ~cpp_parser_t();

    bool parse();

protected:
    virtual void depends_changed() = 0;
    virtual void handle_comment(const char *text) = 0;
    virtual void post_line() = 0;

    unsigned long lineno() const { return lineno_; }
    bool depends(const char *var) const;

    void dump() const;

private:
    struct depend_t
    {
	unsigned long lineno_;
	std::tr1::unordered_map<std::string, int> deltas_;
    };

    enum token_t
    {
	T_LEFT_PAREN = '(',
	T_RIGHT_PAREN = ')',
	T_BANG = '!',

	T_INCLUDE=256,
	T_IF,
	T_IFDEF,
	T_IFNDEF,
	T_ELSE,
	T_ELIF,
	T_ENDIF,
	T_DEFINE,
	T_UNDEF,
	T_DEFINED,
	T_LOG_AND,
	T_LOG_OR,

	T_NUMBER,
	T_IDENTIFIER,

	T_BOOL_EXPR,
	T_LOG_OP,

	T_MAX_TOKEN
    };

    int xgetc();
    void xungetc(int c);
    int getc_commentless();
    void got_comment(estring &e, unsigned int off);
    int get_token();
    const char *token_as_string(int tok) const;
    void parse_cpp_line(unsigned long lineno);
    void parse_c_line(unsigned long lineno);
    void stack_dump();
    void stack_push(int token);
    int stack_pop();
    int stack_peek(int off);
    void stack_replace(int ntoks, int newtoken);
    bool parse_boolean_expr(bool inverted);
    void parse_if();
    void parse_ifdef();
    void parse_ifndef();
    void parse_else();
    void parse_endif();
    void set_delta(depend_t *dep, const char *var, int delta);

    string_var filename_;
    unsigned long lineno_;
    estring line_;
    unsigned int index_;    /* index into line_ which xgetc() will next return */
    bool in_comment_;
    estring comment_;       /* text of all comments in the line, ws-separated */
    char tokenbuf_[256];
    list_t<depend_t> depend_stack_;
#define MAX_STACK   32
    int depth_;
    int stack_[MAX_STACK];
};


#endif /* _ggcov_cpp_parser_H_ */

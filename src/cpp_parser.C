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

#include "cpp_parser.H"

#define ISBLANK(c)  	((c) == ' ' || (c) == '\t')

CVSID("$Id: cpp_parser.C,v 1.1 2004-02-08 11:08:17 gnb Exp $");

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cpp_parser_t::cpp_parser_t(const char *filename)
 :  filename_(filename)
{
}

cpp_parser_t::~cpp_parser_t()
{
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
cpp_parser_t::getc()
{
    return data_[index_++];
}

void
cpp_parser_t::ungetc(int c)
{
    assert(index_ > 0);
    assert(data_[index_-1] == c);
    index_--;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

int
cpp_parser_t::getc_commentless()
{
    while (index_ < length_)
    {
    	int c = getc();

    	if (!in_comment_)
    	{
	    /* scanning for start of comment */
	    if (c == '/')
	    {
	    	c = getc();
		if (c == '*')
	    	    in_comment_ = TRUE;
		else if (c == '/')  /* // C++ style comment */
		    return EOF;
		else
		{
		    ungetc(c);
	    	    return '/';
		}
	    }
	    else
	    	return c;
	}
	else
	{
	    /* scanning for '*' at end of C comment */
	    if (c == '*')
	    {
	    	c = getc();
		if (c == '/')
	    	    in_comment_ = FALSE;
	    }
	}
    }
    return EOF;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

/* TODO: handle token pasting ??? */
int
cpp_parser_t::get_token()
{
    int c;
    unsigned int len = 0;

    while ((c = getc_commentless()) != EOF && ISBLANK(c))
	;
    if (c == EOF)
    	return EOF;

    tokenbuf_[0] = '\0';
	
    if (isdigit(c))
    {
    	do
	{
	    tokenbuf_[len++] = c;
	}
	while ((c = getc_commentless()) != EOF &&
	       isdigit(c) && 
	       len < sizeof(tokenbuf_));
	tokenbuf_[len] = '\0';
	if (c != EOF)
	    ungetc(c);
    	return T_NUMBER;
    }
    else if (isalpha(c) || (c) == '_')
    {
    	do
	{
	    tokenbuf_[len++] = c;
	}
	while ((c = getc_commentless()) != EOF &&
	       (isalnum(c) || (c) == '_') && 
	       len < sizeof(tokenbuf_));
	tokenbuf_[len] = '\0';
	if (c != EOF)
	    ungetc(c);

	if (!strcmp(tokenbuf_, "include"))
	    return T_INCLUDE;
	if (!strcmp(tokenbuf_, "if"))
	    return T_IF;
	if (!strcmp(tokenbuf_, "ifdef"))
	    return T_IFDEF;
	if (!strcmp(tokenbuf_, "ifndef"))
	    return T_IFNDEF;
	if (!strcmp(tokenbuf_, "else"))
	    return T_ELSE;
	if (!strcmp(tokenbuf_, "elif"))
	    return T_ELIF;
	if (!strcmp(tokenbuf_, "endif"))
	    return T_ENDIF;
	if (!strcmp(tokenbuf_, "define"))
	    return T_DEFINE;
	if (!strcmp(tokenbuf_, "undef"))
	    return T_UNDEF;
	if (!strcmp(tokenbuf_, "defined"))
	    return T_DEFINED;
	return T_IDENTIFIER;
    }
    else if (c == '&')
    {
    	if ((c = getc()) == '&')
	    return T_LOG_AND;
	ungetc(c);
	return '&';
    }
    else if (c == '|')
    {
    	if ((c = getc()) == '|')
	    return T_LOG_OR;
	ungetc(c);
	return '|';
    }
    return c;
}

const char *
cpp_parser_t::token_as_string(int tok) const
{
    static const char *token_strs[] = 
    {
    	"INCLUDE",
	"IF",
	"IFDEF",
	"IFNDEF",
	"ELSE",
	"ELIF",
	"ENDIF",
	"DEFINE",
	"UNDEF",
	"DEFINED",
	"&&",
	"||",
	"NUMBER",
	"IDENTIFIER",
	"BOOL_EXPR",
	"LOG_OP"
    };

    if (tok < 256)
    {
	static char cbuf[8];
    	if (isprint(tok))
	{
	    cbuf[0] = tok;
	    cbuf[1] = '\0';
	}
	else
	{
	    snprintf(cbuf, sizeof(cbuf), "\\%03o", tok);
	}
    	return cbuf;
    }
    return (tok >= T_MAX_TOKEN ? "unknown" : token_strs[tok-256]);
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

cpp_parser_t::depend_t::depend_t()
{
    deltas_ = new hashtable_t<const char *, int>;
}

static gboolean
delete_one_delta(const char *var, int *dd, void *closure)
{
    delete dd;
    return TRUE;
}

cpp_parser_t::depend_t::~depend_t()
{
    deltas_->foreach_remove(delete_one_delta, 0);
    delete deltas_;
}


void
cpp_parser_t::set_delta(depend_t *dep, const char *var, int delta)
{
    int *dd;
    
    if ((dd = dep->deltas_->lookup(var)) == 0)
    {
    	dd = new int;
	dep->deltas_->insert(g_strdup(var), dd);
    }
    else if (*dd != delta)
    {
    	fprintf(stderr,
	    	"%s:%ld: Warning: complex expression involves %s"
	    	" more than once, code suppression may not work.\n",
	    	filename_.data(), lineno_, var);
    }
    *dd = delta;
}

gboolean
cpp_parser_t::depends(const char *var) const
{
    unsigned int count[3];
    list_iterator_t<depend_t> iter;
    gboolean ret;
    
    memset(count, 0, sizeof(count));
    for (iter = depend_stack_.first() ; *iter ; ++iter)
    {
    	depend_t *dep = *iter;
	int *dd = dep->deltas_->lookup(var);
	if (dd != 0)
	    count[(*dd)+1]++;
    }
    ret = (count[2] && !count[0]); // some positive and no negative
    dprintf2(D_CPP|D_VERBOSE, "depends: %s=%d\n", var, ret);
    return ret;
}

static void
dump_one_delta(const char *var, int *dd, void *closure)
{
    fprintf(stderr, " %s=%d", var, *dd);
}

void
cpp_parser_t::dump() const
{
    list_iterator_t<depend_t> iter;
    
    fprintf(stderr, "Depend stack dump:\n");
    for (iter = depend_stack_.last() ; *iter ; --iter)
    {
    	fprintf(stderr, "    [%ld]", (*iter)->lineno_);
    	(*iter)->deltas_->foreach(dump_one_delta, 0);
	fputc('\n', stderr);
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cpp_parser_t::stack_dump()
{
    int i;
    
    fprintf(stderr, "Parse stack:");
    for (i = 0 ; i < depth_ ; i++)
    	fprintf(stderr, " %s", token_as_string(stack_[i]));
    fputc('\n', stderr);
}

void
cpp_parser_t::stack_push(int token)
{
    assert(depth_ < MAX_STACK);
    stack_[depth_++] = token;

    if (debug_enabled(D_CPP|D_VERBOSE))
	stack_dump();
}

int
cpp_parser_t::stack_pop()
{
    assert(depth_ > 0);
    return stack_[--depth_];

    if (debug_enabled(D_CPP|D_VERBOSE))
	stack_dump();
}

int
cpp_parser_t::stack_peek(int off)
{
    assert(depth_ > off);
    return stack_[depth_-1-off];
}

void
cpp_parser_t::stack_replace(int ntoks, int newtoken)
{
    assert(depth_ >= ntoks);
    depth_ -= ntoks;
    stack_[depth_++] = newtoken;

    if (debug_enabled(D_CPP|D_VERBOSE))
	stack_dump();
}

gboolean
cpp_parser_t::parse_boolean_expr(gboolean inverted)
{
    int tok;
    string_var tmp;

    while ((tok = get_token()) != EOF)
    {
    	dprintf2(D_CPP|D_VERBOSE, "Read token %s %s\n",
	    	token_as_string(tok), tokenbuf_);

    	/* shift */
	switch (tok)
	{
	case T_LEFT_PAREN:
	case T_DEFINED:
	    stack_push(tok);
	    break;
	case T_LOG_AND:
	case T_LOG_OR:
	    stack_push(T_LOG_OP);
	    break;
	case T_RIGHT_PAREN:
	    if (depth_ >= 3 &&
		stack_peek(0) == T_IDENTIFIER &&
		stack_peek(1) == T_LEFT_PAREN &&
		stack_peek(2) == T_DEFINED)
	    	stack_replace(3, T_BOOL_EXPR);
	    else if (depth_ >= 2 &&
		stack_peek(0) == T_BOOL_EXPR &&
		stack_peek(1) == T_LEFT_PAREN)
	    	stack_replace(2, stack_peek(0));
	    break;
	case T_IDENTIFIER:
	    stack_push(T_IDENTIFIER);
    	    set_delta(depend_stack_.head(), tokenbuf_, (inverted ? -1 : 1));
	    break;
	case T_BANG:
	    stack_push(T_BANG);
    	    inverted = !inverted;
	    break;
	default:
    	    return FALSE;
	}

    	/* reduce */
    	for (;;)
	{
	    if (depth_ >= 2 &&
	    	stack_peek(0) == T_BOOL_EXPR &&
		stack_peek(1) == T_BANG)
	    {
		stack_replace(2, stack_peek(0));
		inverted = !inverted;
	    }
	    else if (depth_ >= 3 &&
	    	stack_peek(0) == T_BOOL_EXPR &&
	    	stack_peek(1) == T_LOG_OP &&
		stack_peek(2) == T_BOOL_EXPR)
		stack_replace(3, T_BOOL_EXPR);
	    else
	    	break;
	}
    }
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cpp_parser_t::parse_if()
{
    dprintf1(D_CPP, "[%ld] if\n", lineno_);

    // notify subclasses
    depends_changed();

    depend_t *dep = new depend_t;
    dep->lineno_ = lineno_;
    depend_stack_.prepend(dep);

    depth_ = 0;    
    parse_boolean_expr(FALSE);
}

void
cpp_parser_t::parse_ifdef()
{
    dprintf1(D_CPP, "[%ld] ifdef\n", lineno_);

    // notify subclasses
    depends_changed();

    if (get_token() != T_IDENTIFIER)
    	return; /* syntax error */
    if (get_token() != EOF)
    	return; /* syntax error */

    depend_t *dep = new depend_t;
    dep->lineno_ = lineno_;
    depend_stack_.prepend(dep);
    set_delta(dep, tokenbuf_, 1);
}

void
cpp_parser_t::parse_ifndef()
{
    dprintf1(D_CPP, "[%ld] ifndef\n", lineno_);

    // notify subclasses
    depends_changed();

    if (get_token() != T_IDENTIFIER)
    	return; /* syntax error */
    if (get_token() != EOF)
    	return; /* syntax error */

    depend_t *dep = new depend_t;
    dep->lineno_ = lineno_;
    depend_stack_.prepend(dep);
    set_delta(dep, tokenbuf_, -1);
}

static void
invert_one_delta(const char *var, int *dd, void *closure)
{
    *dd = -*dd;
}

void
cpp_parser_t::parse_else()
{
    dprintf1(D_CPP, "[%ld] else\n", lineno_);

    // notify subclasses
    depends_changed();
    
    depend_t *dep;
    if ((dep = depend_stack_.head()) == 0)
    {
    	fprintf(stderr,
	    	"%s:%ld: Warning: unmatched else\n",
	    	filename_.data(), lineno_);
	return;
    }
    dep->lineno_ = lineno_;
    dep->deltas_->foreach(invert_one_delta, 0);
}

void
cpp_parser_t::parse_endif()
{
    dprintf1(D_CPP, "[%ld] endif\n", lineno_);

    // notify subclasses
    depends_changed();

    depend_t *dep;

    if ((dep = depend_stack_.remove_head()) == 0)
    {
    	fprintf(stderr,
	    	"%s:%ld: Warning: unmatched endif\n",
	    	filename_.data(), lineno_);
	return;
    }
    
    delete dep;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

void
cpp_parser_t::parse_line(unsigned long lineno, const estring *line)
{
    data_ = line->data();
    length_ = line->length();
    lineno_ = lineno;
    index_ = 0;
    in_comment_ = FALSE;
    
#if 0
    int c;
    fprintf(stderr, "cpp_parser_t::parse: line[%ld]=\"%s\"\n\t\t\"",
	    lineno, data_);
    while ((c = getc_commentless()) != EOF)
    {
	fputc(c, stderr);
    }
    fprintf(stderr, "\"\n");
#endif

#if 0
    int tok;
    fprintf(stderr, "cpp_parser_t::parse: line[%ld]=\"%s\"\n ->",
	    lineno, data_);
    while ((tok = get_token()) != EOF)
    {
    	fprintf(stderr, " %d(%s)", tok, token_as_string(tok));
    }
    fprintf(stderr, "\n");
#endif

    int tok = get_token();
    switch (tok)
    {
    case T_IF: parse_if(); break;
    case T_IFDEF: parse_ifdef(); break;
    case T_IFNDEF: parse_ifndef(); break;
    case T_ELSE: parse_else(); break;
    case T_ENDIF: parse_endif(); break;
    default: return;
    }
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/

gboolean
cpp_parser_t::parse()
{
    FILE *fp;
    char *start, *end;
    unsigned long lineno = 0, extended = 0, lineno_adj = 0;
    estring line;
    char buf[1024];
    
    if ((fp = fopen(filename_, "r")) == 0)
    {
    	perror(filename_);
	return FALSE;
    }
    
    while (fgets(buf, sizeof(buf), fp) != 0)
    {
    	++lineno;
    	/* drop trailing newline */
	for (end = buf+strlen(buf)-1 ;
	     end >= buf && (*end == '\n' || *end == '\r') ;
	     )
	    *end-- = '\0';

	/* skip initial ws */
	for (start = buf ; ISBLANK(*start) ; start++)
	    ;

    	if (extended) /* previous line was extended */
	{
	    line.append_char(' ');
	}
	else
	{
	    if (*start++ != '#')
		continue;	    /* not a pre-processor line */
	    line.truncate();
	}

	if (*end == '\\')
	{
	    *end-- = '\0';
	    extended++;
	}
	else
	{
	    lineno_adj = extended;
	    extended = 0;
	}

	line.append_string(start);

	if (extended)		/* this line is extended */
	    continue;
	
	/* Have a complete pre-processor line in `line' */
	parse_line(lineno - lineno_adj, &line);
    }
    
    fclose(fp);
    return TRUE;
}

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*/
/*END*/

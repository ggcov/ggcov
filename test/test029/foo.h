#ifndef _FOO_H_
#define _FOO_H_ 1

struct foo
{
    int (*munge)(int);
};

struct foo *bar_new(void);
struct foo *baz_new(void);
struct foo *quux_new(void);

#endif /* _FOO_H_ */

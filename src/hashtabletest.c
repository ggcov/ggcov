#include "filename.h"
#include "string_var.H"
#include "hashtable.H"
#include "testfw.H"

struct pairx
{
    string_var name_;
    int x_;

    pairx(const char *name, int x) :  name_(name), x_(x) { }
};

TEST(ctors)
{
    hashtable_t<const char, pairx> *ht =
	    new hashtable_t<const char, pairx>;

    check(ht->size() == 0);
    check(ht->lookup("foo") == 0);
    delete ht;
}

TEST(insert_remove)
{
    hashtable_t<const char, pairx> *ht =
	    new hashtable_t<const char, pairx>;
    pairx *foo, *bar, *baz;

    check(ht->size() == 0);
    check(ht->lookup("foo") == 0);
    check(ht->lookup("bar") == 0);
    check(ht->lookup("baz") == 0);

    foo = new pairx("foo", 1001);
    ht->insert(foo->name_, foo);
    check(ht->size() == 1);
    check(ht->lookup("foo") == foo);
    check(ht->lookup("bar") == 0);
    check(ht->lookup("baz") == 0);

    bar = new pairx("bar", 1002);
    ht->insert(bar->name_, bar);
    check(ht->size() == 2);
    check(ht->lookup("foo") == foo);
    check(ht->lookup("bar") == bar);
    check(ht->lookup("baz") == 0);

    baz = new pairx("baz", 1003);
    ht->insert(baz->name_, baz);
    check(ht->size() == 3);
    check(ht->lookup("foo") == foo);
    check(ht->lookup("bar") == bar);
    check(ht->lookup("baz") == baz);

    ht->remove("bar");
    check(ht->size() == 2);
    check(ht->lookup("foo") == foo);
    check(ht->lookup("bar") == 0);
    check(ht->lookup("baz") == baz);

    ht->remove("baz");
    check(ht->size() == 1);
    check(ht->lookup("foo") == foo);
    check(ht->lookup("bar") == 0);
    check(ht->lookup("baz") == 0);

    ht->remove("foo");
    check(ht->size() == 0);
    check(ht->lookup("foo") == 0);
    check(ht->lookup("bar") == 0);
    check(ht->lookup("baz") == 0);

    delete foo;
    delete bar;
    delete baz;
    delete ht;
}

TEST(keys)
{
    hashtable_t<const char, pairx> *ht =
	    new hashtable_t<const char, pairx>;
    pairx *foo, *bar, *baz;

    list_t<const char> keys;
    ht->keys(&keys);
    check(keys.length() == 0);

    foo = new pairx("foo", 1001);
    ht->insert(foo->name_, foo);
    bar = new pairx("bar", 1002);
    ht->insert(bar->name_, bar);
    baz = new pairx("baz", 1003);
    ht->insert(baz->name_, baz);

    check(ht->size() == 3);
    check(ht->lookup("foo") == foo);
    check(ht->lookup("bar") == bar);
    check(ht->lookup("baz") == baz);

    ht->keys(&keys);
    check(keys.length() == 3);
    unsigned int found = 0x0;
    int i;
    for (i = 0 ; i < 3 ; i++)
    {
	const char *k = keys.remove_head();
	check(k != 0);
	if (!strcmp(k, "foo"))
	    found |= 0x1;
	else if (!strcmp(k, "bar"))
	    found |= 0x2;
	else if (!strcmp(k, "baz"))
	    found |= 0x4;
    }
    check(keys.length() == 0);
    check(found == 0x7);

    delete foo;
    delete bar;
    delete baz;
    delete ht;
}

TEST(iterator)
{
    hashtable_t<const char, pairx> *ht =
	    new hashtable_t<const char, pairx>;
    pairx *foo, *bar, *baz;

    foo = new pairx("foo", 1001);
    ht->insert(foo->name_, foo);
    bar = new pairx("bar", 1002);
    ht->insert(bar->name_, bar);
    baz = new pairx("baz", 1003);
    ht->insert(baz->name_, baz);

    check(ht->size() == 3);
    check(ht->lookup("foo") == foo);
    check(ht->lookup("bar") == bar);
    check(ht->lookup("baz") == baz);

    hashtable_iter_t<const char, pairx> itr;
    check(*itr == 0);
    check(itr.key() == 0);
    check(itr.value() == 0);
    check(++itr == FALSE);

    unsigned int found = 0x0;
    int i;
    itr = ht->first();

    for (i = 0 ; i < 3 ; i++)
    {
	check(itr.key() != 0);
	check(itr.value() != 0);
	if (!strcmp(itr.key(), "foo"))
	{
	    check(itr.value() == foo);
	    found |= 0x1;
	}
	else if (!strcmp(itr.key(), "bar"))
	{
	    check(itr.value() == bar);
	    found |= 0x2;
	}
	else if (!strcmp(itr.key(), "baz"))
	{
	    check(itr.value() == baz);
	    found |= 0x4;
	}
	if (i < 2)
	    check(++itr != 0);
	else
	    check(++itr == 0);
    }
    check(itr.key() == 0);
    check(itr.value() == 0);
    check(++itr == 0);

    delete foo;
    delete bar;
    delete baz;
    delete ht;
}


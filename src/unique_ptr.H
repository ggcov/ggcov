/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Copyright (c) 2001-2015 Greg Banks <gnb@users.sourceforge.net>
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

#ifndef _ggcov_unique_ptr_H_
#define _ggcov_unique_ptr_H_ 1

// Limited version of std::unique_ptr<> from C++11
// One day soon we'll be able to rely on having C++11 features
template<class T> class unique_ptr
{
public:
    unique_ptr() : ptr_(0) {}
    unique_ptr(T *p) : ptr_(p) {}
    ~unique_ptr() { delete ptr_; ptr_ = 0; }

    operator bool() const { return !!ptr_; }

    T *operator=(T *p) { delete ptr_; ptr_ = p; return ptr_; }

    T *get() const { return ptr_; }
    T *operator*() const { return ptr_; }
    T *operator->() const { return ptr_; }

    void reset() { delete ptr_; ptr_ = 0; }
    T *release() { T *p = ptr_; ptr_ = 0; return p; }

private:
    T *ptr_;
};

#endif /* _ggcov_unique_ptr_H_ */

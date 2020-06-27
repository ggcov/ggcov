/*
 * ggcov - A GTK frontend for exploring gcov coverage data
 * Derived from ggui code
 * Copyright (c) 2000-2020 Greg Banks <gnb@fastmail.fm>
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

#ifndef _ggui_mvc_h_
#define _ggui_mvc_h_ 1

#include "common.h"

typedef void (*mvc_callback_t)(void *obj, unsigned int features, void *closure);

void mvc_listen(void *obj, unsigned int features, mvc_callback_t,
		void *closure);
void mvc_unlisten(void *obj, unsigned int features, mvc_callback_t,
		  void *closure);
void mvc_changed(void *obj, unsigned int features);
void mvc_deleted(void *obj);

/*
 * Between batch() and unbatch() all changed notifications are
 * queued internally in the mvc module, then emitted in unbatch().
 */
void mvc_batch(void);
void mvc_unbatch(void);

#define MVC_FEATURE_BASE    2

#endif /* _ggui_mvc_h_ */

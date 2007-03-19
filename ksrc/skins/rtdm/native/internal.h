/*
 * Real-Time Driver Model for native Linux-rt, private header
 *
 * Copyright (C) 2007 Wolfgang Grandegger <wg@grandegger.com>
 * Copyright (C) 2005-2007 Jan Kiszka <jan.kiszka@web.de>.
 * Copyright (C) 2005 Joerg Langenberg <joerg.langenberg@gmx.net>.
 *
 * Xenomai is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Xenomai is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xenomai; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _RTDM_INTERNAL_H
#define _RTDM_INTERNAL_H

#include <linux/fs.h>
#include <rtdm/rtdm_driver.h>

int _rtdm_sock_create(struct socket *sock, int protocol);

extern struct file_operations rtdm_chrdev_ops;
extern struct proto_ops rtdm_proto_ops;

static inline void rtdm_reference_device(struct rtdm_device *device)
{
	atomic_inc(&device->reserved.refcount);
}

static inline void rtdm_dereference_device(struct rtdm_device *device)
{
	atomic_dec(&device->reserved.refcount);
}

#define RTDM_ASSERT(cond,action)  do { \
    if (unlikely(CONFIG_XENO_OPT_DEBUG_RTDM > 0 && !(cond))) { \
        printk("assertion failed at %s:%d (%s)\n", __FILE__, __LINE__, (#cond)); \
        action; \
    } \
} while(0)

#endif /* _RTDM_INTERNAL_H */

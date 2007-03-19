/*
 * Copyright (C) 2005, 2006 Jan Kiszka <jan.kiszka@web.de>.
 * Copyright (C) 2005 Joerg Langenberg <joerg.langenberg@gmx.net>.
 *
 * RTDM is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _RTDM_CORE_H
#define _RTDM_CORE_H

#include <linux/fs.h>

int _rtdm_sock_create(struct socket *sock, int protocol);

extern struct file_operations rtdm_chrdev_ops;
extern struct proto_ops rtdm_proto_ops;

#endif				/* _RTDM_CORE_H */

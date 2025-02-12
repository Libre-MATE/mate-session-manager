/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2012-2021 MATE Developers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Authors: William Jon McCann <mccann@jhu.edu>
 *
 */

#ifndef __MDM_LOG_H
#define __MDM_LOG_H

#include <glib.h>
#include <stdarg.h>

G_BEGIN_DECLS

void mdm_log_default_handler(const gchar *log_domain, GLogLevelFlags log_level,
                             const gchar *message, gpointer unused_data);
void mdm_log_set_debug(gboolean debug);
void mdm_log_toggle_debug(void);
void mdm_log_init(void);
void mdm_log_shutdown(void);

/* compatibility */
#define mdm_fail g_critical
#define mdm_error g_warning
#define mdm_info g_message
#define mdm_debug g_debug

#define mdm_assert g_assert
#define mdm_assert_not_reached g_assert_not_reached

G_END_DECLS

#endif /* __MDM_LOG_H */

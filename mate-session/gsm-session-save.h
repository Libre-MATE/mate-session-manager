/* gsm-session-save.h
 * Copyright (C) 2008 Lucas Rocha.
 * Copyright (C) 2012-2021 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef __GSM_SESSION_SAVE_H__
#define __GSM_SESSION_SAVE_H__

#include <glib.h>

#include "gsm-store.h"

G_BEGIN_DECLS

void gsm_session_save(GsmStore *client_store, GError **error);

G_END_DECLS

#endif /* __GSM_SESSION_SAVE_H__ */

/*
 * Copyright (C) 2013 Stefano Karapetsas
 * Copyright (C) 2013-2021 MATE Developers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Authors:
 *  Stefano Karapetsas <stefano@karapetsas.com>
 */

#ifndef __GSM_SYSTEMD_H__
#define __GSM_SYSTEMD_H__

#include <glib-object.h>
#include <glib.h>
#include <unistd.h>

G_BEGIN_DECLS

#define GSM_TYPE_SYSTEMD (gsm_systemd_get_type())
#define GSM_SYSTEMD_ERROR (gsm_systemd_error_quark())
G_DECLARE_DERIVABLE_TYPE(GsmSystemd, gsm_systemd, GSM, SYSTEMD, GObject)

#define LOGIND_RUNNING() (access("/run/systemd/seats/", F_OK) >= 0)
typedef enum _GsmSystemdError GsmSystemdError;

struct _GsmSystemdClass {
  GObjectClass parent_class;

  void (*request_completed)(GsmSystemd *manager, GError *error);

  void (*privileges_completed)(GsmSystemd *manager, gboolean success,
                               gboolean ask_later, GError *error);
};

enum _GsmSystemdError {
  GSM_SYSTEMD_ERROR_RESTARTING = 0,
  GSM_SYSTEMD_ERROR_STOPPING
};

#define GSM_SYSTEMD_SESSION_TYPE_LOGIN_WINDOW "greeter"

GQuark gsm_systemd_error_quark(void);

GsmSystemd *gsm_systemd_new(void) G_GNUC_MALLOC;

gboolean gsm_systemd_can_switch_user(GsmSystemd *manager);

gboolean gsm_systemd_get_restart_privileges(GsmSystemd *manager);

gboolean gsm_systemd_get_stop_privileges(GsmSystemd *manager);

gboolean gsm_systemd_can_stop(GsmSystemd *manager);

gboolean gsm_systemd_can_restart(GsmSystemd *manager);

gboolean gsm_systemd_can_hibernate(GsmSystemd *manager);

gboolean gsm_systemd_can_suspend(GsmSystemd *manager);

gboolean gsm_systemd_is_last_session_for_user(GsmSystemd *manager);

void gsm_systemd_attempt_stop(GsmSystemd *manager);

void gsm_systemd_attempt_restart(GsmSystemd *manager);

void gsm_systemd_attempt_hibernate(GsmSystemd *manager);

void gsm_systemd_attempt_suspend(GsmSystemd *manager);

void gsm_systemd_set_session_idle(GsmSystemd *manager, gboolean is_idle);

gchar *gsm_systemd_get_current_session_type(GsmSystemd *manager);

GsmSystemd *gsm_get_systemd(void);

G_END_DECLS

#endif /* __GSM_SYSTEMD_H__ */

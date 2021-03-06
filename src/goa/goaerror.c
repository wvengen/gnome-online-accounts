/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2011 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "goaerror.h"

/**
 * SECTION:goaerror
 * @title: GoaError
 * @short_description: Error codes
 *
 * Error codes and D-Bus errors.
 */

static const GDBusErrorEntry dbus_error_entries[] =
{
  {GOA_ERROR_FAILED,                       "org.freedesktop.Goa.Error.Failed"},
  {GOA_ERROR_NOT_SUPPORTED,                "org.freedesktop.Goa.Error.NotSupported"},
  {GOA_ERROR_DIALOG_DISMISSED,             "org.gnome.OnlineAccounts.Error.DialogDismissed"},
  {GOA_ERROR_ACCOUNT_EXISTS,               "org.gnome.OnlineAccounts.Error.AccountExists"},
  {GOA_ERROR_NOT_AUTHORIZED,               "org.gnome.OnlineAccounts.Error.NotAuthorized"}
};

GQuark
goa_error_quark (void)
{
  G_STATIC_ASSERT (G_N_ELEMENTS (dbus_error_entries) == GOA_ERROR_NUM_ENTRIES);
  static volatile gsize quark_volatile = 0;
  g_dbus_error_register_error_domain ("goa-error-quark",
                                      &quark_volatile,
                                      dbus_error_entries,
                                      G_N_ELEMENTS (dbus_error_entries));
  return (GQuark) quark_volatile;
}

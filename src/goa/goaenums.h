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

#if !defined (__GOA_INSIDE_GOA_H__) && !defined (GOA_COMPILATION)
#error "Only <goa/goa.h> can be included directly."
#endif

#ifndef __GOA_ENUMS_H__
#define __GOA_ENUMS_H__

#include <gio/gio.h>

G_BEGIN_DECLS

/**
 * GoaError:
 * @GOA_ERROR_FAILED: The operation failed.
 * @GOA_ERROR_NOT_SUPPORTED: The operation is not supported.
 * @GOA_ERROR_DIALOG_DISMISSED: The dialog was dismissed.
 * @GOA_ERROR_ACCOUNT_EXISTS: Account already exists.
 * @GOA_ERROR_NOT_AUTHORIZED: Not authorized to perform operation.
 *
 * Error codes for the #GOA_ERROR error domain and the
 * corresponding D-Bus error names.
 */
typedef enum
{
  GOA_ERROR_FAILED,           /* org.gnome.OnlineAccounts.Error.Failed */
  GOA_ERROR_NOT_SUPPORTED,    /* org.gnome.OnlineAccounts.Error.NotSupported */
  GOA_ERROR_DIALOG_DISMISSED, /* org.gnome.OnlineAccounts.Error.DialogDismissed */
  GOA_ERROR_ACCOUNT_EXISTS,   /* org.gnome.OnlineAccounts.Error.AccountExists */
  GOA_ERROR_NOT_AUTHORIZED    /* org.gnome.OnlineAccounts.Error.NotAuthorized */
} GoaError;

#define GOA_ERROR_NUM_ENTRIES  (GOA_ERROR_NOT_AUTHORIZED + 1)

G_END_DECLS

#endif /* __GOA_ENUMS_H__ */

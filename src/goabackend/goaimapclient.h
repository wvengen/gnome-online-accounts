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

#if !defined (__GOA_BACKEND_INSIDE_GOA_BACKEND_H__) && !defined (GOA_BACKEND_COMPILATION)
#error "Only <goabackend/goabackend.h> can be included directly."
#endif

#ifndef __GOA_IMAP_CLIENT_H__
#define __GOA_IMAP_CLIENT_H__

#include <goabackend/goabackendtypes.h>

G_BEGIN_DECLS

#define GOA_TYPE_IMAP_CLIENT         (goa_imap_client_get_type ())
#define GOA_IMAP_CLIENT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GOA_TYPE_IMAP_CLIENT, GoaImapClient))
#define GOA_IS_IMAP_CLIENT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOA_TYPE_IMAP_CLIENT))

GType           goa_imap_client_get_type          (void) G_GNUC_CONST;
GoaImapClient  *goa_imap_client_new               (void);
gboolean        goa_imap_client_connect_sync      (GoaImapClient  *client,
                                                   const gchar    *host_and_port,
                                                   gboolean        use_tls,
                                                   GoaImapAuth    *auth,
                                                   GCancellable   *cancellable,
                                                   GError        **error);
gchar          *goa_imap_client_run_command_sync  (GoaImapClient  *client,
                                                   const gchar    *command,
                                                   GCancellable   *cancellable,
                                                   GError        **error);
gboolean        goa_imap_client_idle_sync         (GoaImapClient  *client,
                                                   guint           max_idle_seconds,
                                                   GCancellable   *cancellable,
                                                   GError         **error);
gboolean        goa_imap_client_disconnect_sync   (GoaImapClient  *client,
                                                   GCancellable   *cancellable,
                                                   GError        **error);

// TODO: gint                   goa_imap_client_get_socket_fd    (GoaImapClient  *client);


G_END_DECLS

#endif /* __GOA_IMAP_CLIENT_H__ */
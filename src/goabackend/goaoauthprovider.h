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

#ifndef __GOA_OAUTH_PROVIDER_H__
#define __GOA_OAUTH_PROVIDER_H__

#include <goabackend/goabackendtypes.h>
#include <goabackend/goaprovider.h>

G_BEGIN_DECLS

#define GOA_TYPE_OAUTH_PROVIDER         (goa_oauth_provider_get_type ())
#define GOA_OAUTH_PROVIDER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GOA_TYPE_OAUTH_PROVIDER, GoaOAuthProvider))
#define GOA_OAUTH_PROVIDER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), GOA_TYPE_OAUTH_PROVIDER, GoaOAuthProviderClass))
#define GOA_OAUTH_PROVIDER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GOA_TYPE_OAUTH_PROVIDER, GoaOAuthProviderClass))
#define GOA_IS_OAUTH_PROVIDER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOA_TYPE_OAUTH_PROVIDER))

#define GOA_IS_OAUTH_PROVIDER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GOA_TYPE_OAUTH_PROVIDER))

typedef struct _GoaOAuthProviderClass GoaOAuthProviderClass;
typedef struct _GoaOAuthProviderPrivate GoaOAuthProviderPrivate;

/**
 * GoaOAuthProvider:
 *
 * The #GoaOAuthProvider structure contains only private data and should
 * only be accessed using the provided API.
 */
struct _GoaOAuthProvider
{
  /*< private >*/
  GoaProvider parent_instance;
  GoaOAuthProviderPrivate *priv;
};

/**
 * GoaOAuthProviderClass:
 * @parent_class: The parent class.
 * @get_consumer_key: Virtual function for goa_oauth_provider_get_consumer_key().
 * @get_consumer_secret: Virtual function for goa_oauth_provider_get_consumer_secret().
 * @get_request_uri: Virtual function for goa_oauth_provider_get_request_uri().
 * @get_authorization_uri: Virtual function for goa_oauth_provider_get_authorization_uri().
 * @get_token_uri: Virtual function for goa_oauth_provider_get_token_uri().
 * @get_callback_uri: Virtual function for goa_oauth_provider_get_callback_uri().
 * @get_identity_sync: Virtual function for goa_oauth_provider_get_identity_sync().
 * @build_authorization_uri: Virtual function for goa_oauth_provider_build_authorization_uri().
 * @get_use_external_browser: Virtual function for goa_oauth_provider_get_use_external_browser().
 * @get_request_uri_params: Virtual function for goa_oauth_provider_get_request_uri_params().
 * @add_account_key_values: Virtual function for goa_oauth_provider_add_account_key_values().
 *
 * Class structure for #GoaOAuthProvider.
 */
struct _GoaOAuthProviderClass
{
  GoaProviderClass parent_class;

  /* pure virtual */
  const gchar *(*get_consumer_key)         (GoaOAuthProvider  *provider);
  const gchar *(*get_consumer_secret)      (GoaOAuthProvider  *provider);
  const gchar *(*get_request_uri)          (GoaOAuthProvider  *provider);
  const gchar *(*get_authorization_uri)    (GoaOAuthProvider  *provider);
  const gchar *(*get_token_uri)            (GoaOAuthProvider  *provider);
  const gchar *(*get_callback_uri)         (GoaOAuthProvider  *provider);

  gchar       *(*get_identity_sync)        (GoaOAuthProvider  *provider,
                                            const gchar       *access_token,
                                            const gchar       *access_token_secret,
                                            gchar            **out_presentation_identity,
                                            GCancellable      *cancellable,
                                            GError           **error);

  /* virtual but with default implementation */
  gchar       *(*build_authorization_uri)  (GoaOAuthProvider  *provider,
                                            const gchar       *authorization_uri,
                                            const gchar       *escaped_oauth_token);
  gboolean     (*get_use_external_browser) (GoaOAuthProvider  *provider);
  gchar      **(*get_request_uri_params)   (GoaOAuthProvider  *provider);
  void         (*add_account_key_values)   (GoaOAuthProvider  *provider,
                                            GVariantBuilder   *builder);

  /*< private >*/
  /* Padding for future expansion */
  gpointer goa_reserved[32];
};

GType        goa_oauth_provider_get_type                 (void) G_GNUC_CONST;
const gchar *goa_oauth_provider_get_consumer_key         (GoaOAuthProvider  *provider);
const gchar *goa_oauth_provider_get_consumer_secret      (GoaOAuthProvider  *provider);
const gchar *goa_oauth_provider_get_request_uri          (GoaOAuthProvider  *provider);
gchar      **goa_oauth_provider_get_request_uri_params   (GoaOAuthProvider  *provider);
const gchar *goa_oauth_provider_get_authorization_uri    (GoaOAuthProvider  *provider);
const gchar *goa_oauth_provider_get_token_uri            (GoaOAuthProvider  *provider);
const gchar *goa_oauth_provider_get_callback_uri         (GoaOAuthProvider  *provider);
gchar       *goa_oauth_provider_get_identity_sync        (GoaOAuthProvider  *provider,
                                                          const gchar       *access_token,
                                                          const gchar       *access_token_secret,
                                                          gchar            **out_presentation_identity,
                                                          GCancellable      *cancellable,
                                                          GError           **error);
gchar       *goa_oauth_provider_get_access_token_sync    (GoaOAuthProvider  *provider,
                                                          GoaObject         *object,
                                                          gboolean           force_refresh,
                                                          gchar            **out_access_token_secret,
                                                          gint              *out_access_token_expires_in,
                                                          GCancellable      *cancellable,
                                                          GError           **error);
gchar       *goa_oauth_provider_build_authorization_uri  (GoaOAuthProvider  *provider,
                                                          const gchar       *authorization_uri,
                                                          const gchar       *escaped_oauth_token);
gboolean     goa_oauth_provider_get_use_external_browser (GoaOAuthProvider  *provider);
void         goa_oauth_provider_add_account_key_values   (GoaOAuthProvider  *provider,
                                                          GVariantBuilder   *builder);

G_END_DECLS

#endif /* __GOA_OAUTH_PROVIDER_H__ */

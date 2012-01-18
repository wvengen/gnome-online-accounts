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

#include <rest/oauth-proxy.h>
#include <json-glib/json-glib.h>

#include "goaprovider.h"
#include "goaoauthprovider.h"
#include "goaflickrprovider.h"

/**
 * GoaFlickrProvider:
 *
 * The #GoaFlickrProvider structure contains only private data and should
 * only be accessed using the provided API.
 */
struct _GoaFlickrProvider
{
  /*< private >*/
  GoaOAuthProvider parent_instance;
};

typedef struct _GoaFlickrProviderClass GoaFlickrProviderClass;

struct _GoaFlickrProviderClass
{
  GoaOAuthProviderClass parent_class;
};

/**
 * SECTION:goaflickrprovider
 * @title: GoaFlickrProvider
 * @short_description: A provider for Flickr
 *
 * #GoaFlickrProvider is used for handling Flickr accounts.
 */

G_DEFINE_TYPE_WITH_CODE (GoaFlickrProvider, goa_flickr_provider, GOA_TYPE_OAUTH_PROVIDER,
                         g_io_extension_point_implement (GOA_PROVIDER_EXTENSION_POINT_NAME,
							 g_define_type_id,
							 "flickr",
							 0));

/* ---------------------------------------------------------------------------------------------------- */

static const gchar *
get_provider_type (GoaProvider *_provider)
{
  return "flickr";
}

static gchar *
get_provider_name (GoaProvider *_provider,
                   GoaObject   *object)
{
  return g_strdup (_("Flickr"));
}

static const gchar *
get_consumer_key (GoaOAuthProvider *provider)
{
  return GOA_FLICKR_CONSUMER_KEY;
}

static const gchar *
get_consumer_secret (GoaOAuthProvider *provider)
{
  return GOA_FLICKR_CONSUMER_SECRET;
}

static const gchar *
get_request_uri (GoaOAuthProvider *provider)
{
  return "http://www.flickr.com/services/oauth/request_token";
}

static const gchar *
get_authorization_uri (GoaOAuthProvider *provider)
{
  return "http://www.flickr.com/services/oauth/authorize";
}

static const gchar *
get_token_uri (GoaOAuthProvider *provider)
{
  return "http://www.flickr.com/services/oauth/access_token";
}

static const gchar *
get_callback_uri (GoaOAuthProvider *provider)
{
  return "https://www.gnome.org/goa-1.0/oauth";
}

/* ---------------------------------------------------------------------------------------------------- */

static gchar *
get_identity_sync (GoaOAuthProvider  *provider,
                   const gchar       *access_token,
                   const gchar       *access_token_secret,
                   gchar            **out_presentation_identity,
                   GCancellable      *cancellable,
                   GError           **error)
{
  RestProxy *proxy;
  RestProxyCall *call;
  JsonParser *parser;
  JsonObject *json_object;
  gchar *ret;
  gchar *id;
  gchar *presentation_identity;

  ret = NULL;
  proxy = NULL;
  call = NULL;
  parser = NULL;
  id = NULL;
  presentation_identity = NULL;

  /* TODO: cancellable */

  proxy = oauth_proxy_new_with_token (goa_oauth_provider_get_consumer_key (provider),
                                      goa_oauth_provider_get_consumer_secret (provider),
                                      access_token,
                                      access_token_secret,
                                      "http://api.flickr.com/services/rest",
                                      FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_add_param (call, "method", "flickr.test.login");
  rest_proxy_call_add_param (call, "format", "json");
  rest_proxy_call_add_param (call, "nojsoncallback", "1");
  rest_proxy_call_set_method (call, "GET");

  if (!rest_proxy_call_sync (call, error))
    goto out;
  if (rest_proxy_call_get_status_code (call) != 200)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   _("Expected status 200 when requesting user id, instead got status %d (%s)"),
                   rest_proxy_call_get_status_code (call),
                   rest_proxy_call_get_status_message (call));
      goto out;
    }

  parser = json_parser_new ();
  if (!json_parser_load_from_data (parser,
                                   rest_proxy_call_get_payload (call),
                                   rest_proxy_call_get_payload_length (call),
                                   error))
    {
      g_prefix_error (error, _("Error parsing response as JSON: "));
      goto out;
    }

  json_object = json_node_get_object (json_parser_get_root (parser));
  json_object = json_object_get_object_member (json_object, "user");
  if (json_object == NULL)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   _("Didn't find user member in JSON data"));
      goto out;
    }
  id = g_strdup (json_object_get_string_member (json_object, "id"));
  if (id == NULL)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   _("Didn't find user.id member in JSON data"));
      goto out;
    }
  json_object = json_object_get_object_member (json_object, "username");
  if (json_object == NULL)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   _("Didn't find user.username member in JSON data"));
      goto out;
    }
  presentation_identity = g_strdup (json_object_get_string_member (json_object, "_content"));
  if (presentation_identity == NULL)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   _("Didn't find user.username._content member in JSON data"));
      goto out;
    }

  ret = id;
  id = NULL;
  if (out_presentation_identity != NULL)
    {
      *out_presentation_identity = presentation_identity;
      presentation_identity = NULL;
    }

 out:
  g_free (id);
  g_free (presentation_identity);
  if (call != NULL)
    g_object_unref (call);
  if (proxy != NULL)
    g_object_unref (proxy);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
build_object (GoaProvider         *provider,
              GoaObjectSkeleton   *object,
              GKeyFile            *key_file,
              const gchar         *group,
              GError             **error)
{
  gboolean ret;
  ret = FALSE;

  /* Chain up */
  if (!GOA_PROVIDER_CLASS (goa_flickr_provider_parent_class)->build_object (provider,
                                                                             object,
                                                                             key_file,
                                                                             group,
                                                                             error))
    goto out;

  ret = TRUE;

 out:
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
get_use_external_browser (GoaOAuthProvider *provider)
{
  /* For some reason this only works in a browser - bad callback URL? TODO: investigate */
  return TRUE;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
show_account (GoaProvider         *provider,
              GoaClient           *client,
              GoaObject           *object,
              GtkBox              *vbox,
              GtkTable            *table)
{
  /* Chain up */
  GOA_PROVIDER_CLASS (goa_flickr_provider_parent_class)->show_account (provider, client, object, vbox, table);
  goa_util_add_row_editable_label_from_keyfile (table, object, _("User Name"), "PresentationIdentity", FALSE);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
goa_flickr_provider_init (GoaFlickrProvider *client)
{
}

static void
goa_flickr_provider_class_init (GoaFlickrProviderClass *klass)
{
  GoaProviderClass *provider_class;
  GoaOAuthProviderClass *oauth_class;

  provider_class = GOA_PROVIDER_CLASS (klass);
  provider_class->get_provider_type     = get_provider_type;
  provider_class->get_provider_name     = get_provider_name;
  provider_class->build_object          = build_object;
  provider_class->show_account          = show_account;

  oauth_class = GOA_OAUTH_PROVIDER_CLASS (klass);
  oauth_class->get_identity_sync        = get_identity_sync;
  oauth_class->get_consumer_key         = get_consumer_key;
  oauth_class->get_consumer_secret      = get_consumer_secret;
  oauth_class->get_request_uri          = get_request_uri;
  oauth_class->get_authorization_uri    = get_authorization_uri;
  oauth_class->get_token_uri            = get_token_uri;
  oauth_class->get_callback_uri         = get_callback_uri;
  oauth_class->get_use_external_browser = get_use_external_browser;
}

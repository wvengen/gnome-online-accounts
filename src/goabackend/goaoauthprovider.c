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
#include <stdlib.h>

#include <rest/oauth-proxy.h>
#include <libsoup/soup-gnome.h>
#include <webkit/webkit.h>
#include <json-glib/json-glib.h>

#include "goalogging.h"
#include "goaprovider.h"
#include "goaoauthprovider.h"

/**
 * SECTION:goaoauthprovider
 * @title: GoaOAuthProvider
 * @short_description: Abstract base class for OAuth 1.0a providers
 *
 * #GoaOAuthProvider is an abstract base class for OAuth 1.0a
 * compliant implementations as defined by <ulink
 * url="http://tools.ietf.org/html/rfc5849">RFC
 * 5849</ulink>. Additionally, the code works with providers
 * implementing <ulink
 * url="http://oauth.googlecode.com/svn/spec/ext/session/1.0/drafts/1/spec.html">OAuth
 * Session 1.0 Draft 1</ulink> for refreshing access tokens.
 *
 * Subclasses must implement
 * #GoaOAuthProviderClass.get_consumer_key,
 * #GoaOAuthProviderClass.get_consumer_secret,
 * #GoaOAuthProviderClass.get_request_uri,
 * #GoaOAuthProviderClass.get_authorization_uri,
 * #GoaOAuthProviderClass.get_token_uri,
 * #GoaOAuthProviderClass.get_callback_uri and
 * #GoaOAuthProviderClass.get_identity_sync methods.
 *
 * Additionally, the
 * #GoaProviderClass.get_provider_type,
 * #GoaProviderClass.get_provider_name,
 * #GoaProviderClass.build_object (this should chain up to its
 * parent class) methods must be implemented.
 *
 * Note that the #GoaProviderClass.add_account,
 * #GoaProviderClass.refresh_account and
 * #GoaProviderClass.ensure_credentials_sync methods do not
 * need to be implemented - this type implements these methods.
 */

G_LOCK_DEFINE_STATIC (provider_lock);

G_DEFINE_ABSTRACT_TYPE (GoaOAuthProvider, goa_oauth_provider, GOA_TYPE_PROVIDER);

static gboolean
is_authorization_error (GError *error)
{
  gboolean ret;

  g_return_val_if_fail (error != NULL, FALSE);

  ret = FALSE;
  if (error->domain == REST_PROXY_ERROR || error->domain == SOUP_HTTP_ERROR)
    {
      if (SOUP_STATUS_IS_CLIENT_ERROR (error->code))
        ret = TRUE;
    }
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
goa_oauth_provider_get_use_external_browser_default (GoaOAuthProvider  *provider)
{
  return FALSE;
}

/**
 * goa_oauth_provider_get_use_external_browser:
 * @provider: A #GoaOAuthProvider.
 *
 * Returns whether an external browser (the users default browser)
 * should be used for user interaction.
 *
 * If an external browser is used, then the dialogs presented in
 * goa_provider_add_account() and
 * goa_provider_refresh_account() will show a simple "Paste
 * authorization code here" instructions along with an entry and
 * button.
 *
 * This is a virtual method where the default implementation returns
 * %FALSE.
 *
 * Returns: %TRUE if the user interaction should happen in an external
 * browser, %FALSE to use an embedded browser widget.
 */
gboolean
goa_oauth_provider_get_use_external_browser (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), FALSE);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_use_external_browser (provider);
}

/* ---------------------------------------------------------------------------------------------------- */

static void
goa_oauth_provider_add_account_key_values_default (GoaOAuthProvider  *provider,
                                                   GVariantBuilder   *builder)
{
  /* do nothing */
}

/**
 * goa_oauth_provider_add_account_key_values:
 * @provider: A #GoaProvider.
 * @builder: A #GVariantBuilder for a <literal>a{ss}</literal> variant.
 *
 * Hook for implementations to add key/value pairs to the key-file
 * when creating an account.
 *
 * This is a virtual method where the default implementation does nothing.
 */
void
goa_oauth_provider_add_account_key_values (GoaOAuthProvider  *provider,
                                           GVariantBuilder   *builder)
{
  g_return_if_fail (GOA_IS_OAUTH_PROVIDER (provider));
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->add_account_key_values (provider, builder);
}

/* ---------------------------------------------------------------------------------------------------- */

static gchar *
goa_oauth_provider_build_authorization_uri_default (GoaOAuthProvider  *provider,
                                                    const gchar       *authorization_uri,
                                                    const gchar       *escaped_oauth_token)
{
  return g_strdup_printf ("%s"
                          "?oauth_token=%s",
                          authorization_uri,
                          escaped_oauth_token);
}

/**
 * goa_oauth_provider_build_authorization_uri:
 * @provider: A #GoaOAuthProvider.
 * @authorization_uri: An authorization URI.
 * @escaped_oauth_token: An escaped oauth token.
 *
 * Builds the URI that can be opened in a web browser (or embedded web
 * browser widget) to start authenticating an user.
 *
 * The default implementation just returns the expected URI
 * (e.g. <literal>http://example.com/dialog/oauth?auth_token=1234567890</literal>)
 * - override (and chain up) if you e.g. need to to pass additional
 * parameters.
 *
 * The @authorization_uri parameter originate from the result of the
 * the goa_oauth_provider_get_authorization_uri() method. The
 * @escaped_oauth_token parameter is the temporary credentials identifier
 * escaped using g_uri_escape_string().
 *
 * Returns: (transfer full): An authorization URI that must be freed with g_free().
 */
gchar *
goa_oauth_provider_build_authorization_uri (GoaOAuthProvider  *provider,
                                            const gchar       *authorization_uri,
                                            const gchar       *escaped_oauth_token)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  g_return_val_if_fail (authorization_uri != NULL, NULL);
  g_return_val_if_fail (escaped_oauth_token != NULL, NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->build_authorization_uri (provider,
                                                                                   authorization_uri,
                                                                                   escaped_oauth_token);
}

/**
 * goa_oauth_provider_get_consumer_key:
 * @provider: A #GoaOAuthProvider.
 *
 * Gets the consumer key identifying the client.
 *
 * This is a pure virtual method - a subclass must provide an
 * implementation.
 *
 * Returns: (transfer none): A string owned by @provider - do not free.
 */
const gchar *
goa_oauth_provider_get_consumer_key (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_consumer_key (provider);
}

/**
 * goa_oauth_provider_get_consumer_secret:
 * @provider: A #GoaOAuthProvider.
 *
 * Gets the consumer secret identifying the client.
 *
 * This is a pure virtual method - a subclass must provide an
 * implementation.
 *
 * Returns: (transfer none): A string owned by @provider - do not free.
 */
const gchar *
goa_oauth_provider_get_consumer_secret (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_consumer_secret (provider);
}

/**
 * goa_oauth_provider_get_request_uri:
 * @provider: A #GoaOAuthProvider.
 *
 * Gets the request uri.
 *
 * http://tools.ietf.org/html/rfc5849#section-2.1
 *
 * This is a pure virtual method - a subclass must provide an
 * implementation.
 *
 * Returns: (transfer none): A string owned by @provider - do not free.
 */
const gchar *
goa_oauth_provider_get_request_uri (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_request_uri (provider);
}

/**
 * goa_oauth_provider_get_request_uri_params:
 * @provider: A #GoaOAuthProvider.
 *
 * Gets additional parameters for the request URI.
 *
 * http://tools.ietf.org/html/rfc5849#section-2.1
 *
 * This is a virtual method where the default implementation returns
 * %NULL.
 *
 * Returns: (transfer full): %NULL (for no parameters) or a
 * %NULL-terminated array of (key, value) pairs that will be added to
 * the URI. The caller will free the returned value with g_strfreev().
 */
gchar **
goa_oauth_provider_get_request_uri_params (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_request_uri_params (provider);
}

static gchar **
goa_oauth_provider_get_request_uri_params_default (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  return NULL;
}

/**
 * goa_oauth_provider_get_authorization_uri:
 * @provider: A #GoaOAuthProvider.
 *
 * Gets the authorization uri.
 *
 * http://tools.ietf.org/html/rfc5849#section-2.2
 *
 * This is a pure virtual method - a subclass must provide an
 * implementation.
 *
 * Returns: (transfer none): A string owned by @provider - do not free.
 */
const gchar *
goa_oauth_provider_get_authorization_uri (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_authorization_uri (provider);
}

/**
 * goa_oauth_provider_get_token_uri:
 * @provider: A #GoaOAuthProvider.
 *
 * Gets the token uri.
 *
 * http://tools.ietf.org/html/rfc5849#section-2.3
 *
 * This is a pure virtual method - a subclass must provide an
 * implementation.
 *
 * Returns: (transfer none): A string owned by @provider - do not free.
 */
const gchar *
goa_oauth_provider_get_token_uri (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_token_uri (provider);
}

/**
 * goa_oauth_provider_get_callback_uri:
 * @provider: A #GoaOAuthProvider.
 *
 * Gets the callback uri.
 *
 * http://tools.ietf.org/html/rfc5849#section-2.1
 *
 * This is a pure virtual method - a subclass must provide an
 * implementation.
 *
 * Returns: (transfer none): A string owned by @provider - do not free.
 */
const gchar *
goa_oauth_provider_get_callback_uri (GoaOAuthProvider *provider)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_callback_uri (provider);
}

/**
 * goa_oauth_provider_get_identity_sync:
 * @provider: A #GoaOAuthProvider.
 * @access_token: A valid OAuth 1.0 access token.
 * @access_token_secret: The valid secret for @access_token.
 * @out_presentation_identity: (out): Return location for presentation identity or %NULL.
 * @cancellable: (allow-none): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Method that returns the identity corresponding to @access_token and
 * @access_token_secret.
 *
 * The identity is needed because all authentication happens out of
 * band. In addition to the identity, an implementation also returns a
 * <emphasis>presentation identity</emphasis> that is more suitable
 * for presentation (the identity could be a GUID for example).
 *
 * The calling thread is blocked while the identity is obtained.
 *
 * This is a pure virtual method - a subclass must provide an
 * implementation.
 *
 * Returns: The identity or %NULL if error is set. The returned string
 * must be freed with g_free().
 */
gchar *
goa_oauth_provider_get_identity_sync (GoaOAuthProvider *provider,
                                      const gchar      *access_token,
                                      const gchar      *access_token_secret,
                                      gchar           **out_presentation_identity,
                                      GCancellable     *cancellable,
                                      GError          **error)
{
  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  g_return_val_if_fail (access_token != NULL, NULL);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  return GOA_OAUTH_PROVIDER_GET_CLASS (provider)->get_identity_sync (provider, access_token, access_token_secret, out_presentation_identity, cancellable, error);
}

/* ---------------------------------------------------------------------------------------------------- */

static gchar *
get_tokens_sync (GoaOAuthProvider  *provider,
                 const gchar       *token,
                 const gchar       *token_secret,
                 const gchar       *session_handle, /* may be NULL */
                 const gchar       *verifier,       /* may be NULL */
                 gchar            **out_access_token_secret,
                 gint              *out_access_token_expires_in,
                 gchar            **out_session_handle,
                 gint              *out_session_handle_expires_in,
                 GCancellable      *cancellable,
                 GError           **error)
{
  RestProxy *proxy;
  RestProxyCall *call;
  gchar *ret;
  guint status_code;
  GHashTable *f;
  const gchar *expires_in_str;
  gchar *ret_access_token;
  gchar *ret_access_token_secret;
  gint ret_access_token_expires_in;
  gchar *ret_session_handle;
  gint ret_session_handle_expires_in;

  ret = NULL;
  ret_access_token = NULL;
  ret_access_token_secret = NULL;
  ret_access_token_expires_in = 0;
  ret_session_handle = NULL;
  ret_session_handle_expires_in = 0;

  proxy = oauth_proxy_new (goa_oauth_provider_get_consumer_key (provider),
                           goa_oauth_provider_get_consumer_secret (provider),
                           goa_oauth_provider_get_token_uri (provider),
                           FALSE);
  oauth_proxy_set_token (OAUTH_PROXY (proxy), token);
  oauth_proxy_set_token_secret (OAUTH_PROXY (proxy), token_secret);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_method (call, "POST");
  if (verifier != NULL)
    rest_proxy_call_add_param (call, "oauth_verifier", verifier);
  if (session_handle != NULL)
    rest_proxy_call_add_param (call, "oauth_session_handle", session_handle);
  /* TODO: cancellable support? */
  if (!rest_proxy_call_sync (call, error))
    goto out;

  status_code = rest_proxy_call_get_status_code (call);
  if (status_code != 200)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   /* Translators: the %d is a HTTP status code and the %s is a textual description of it */
                   _("Expected status 200 when requesting access token, instead got status %d (%s)"),
                   status_code,
                   rest_proxy_call_get_status_message (call));
      goto out;
    }

  f = soup_form_decode (rest_proxy_call_get_payload (call));
  ret_access_token = g_strdup (g_hash_table_lookup (f, "oauth_token"));
  ret_access_token_secret = g_strdup (g_hash_table_lookup (f, "oauth_token_secret"));
  ret_session_handle = g_strdup (g_hash_table_lookup (f, "oauth_session_handle"));
  expires_in_str = g_hash_table_lookup (f, "oauth_expires_in");
  if (expires_in_str != NULL)
    ret_access_token_expires_in = atoi (expires_in_str);
  expires_in_str = g_hash_table_lookup (f, "oauth_authorization_expires_in");
  if (expires_in_str != NULL)
    ret_session_handle_expires_in = atoi (expires_in_str);
  g_hash_table_unref (f);

  if (ret_access_token == NULL || ret_access_token_secret == NULL)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   _("Missing access_token or access_token_secret headers in response"));
      goto out;
    }

  ret = ret_access_token; ret_access_token = NULL;
  if (out_access_token_secret != NULL)
    {
      *out_access_token_secret = ret_access_token_secret;
      ret_access_token_secret = NULL;
    }
  if (out_access_token_expires_in != NULL)
    *out_access_token_expires_in = ret_access_token_expires_in;
  if (out_session_handle != NULL)
    {
      *out_session_handle = ret_session_handle;
      ret_session_handle = NULL;
    }
  if (out_session_handle_expires_in != NULL)
    *out_session_handle_expires_in = ret_session_handle_expires_in;

 out:
  g_free (ret_access_token);
  g_free (ret_access_token_secret);
  g_free (ret_session_handle);
  if (call != NULL)
    g_object_unref (call);
  if (proxy != NULL)
    g_object_unref (proxy);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  GoaOAuthProvider *provider;
  GtkDialog *dialog;
  GError *error;
  GMainLoop *loop;

  gchar *oauth_verifier;

  gchar *identity;
  gchar *presentation_identity;

  gchar *request_token;
  gchar *request_token_secret;
  gchar *access_token;
  gchar *access_token_secret;
  gint access_token_expires_in;
  gchar *session_handle;
  gint session_handle_expires_in;
} IdentifyData;

static gboolean
on_web_view_navigation_policy_decision_requested (WebKitWebView             *webView,
                                                  WebKitWebFrame            *frame,
                                                  WebKitNetworkRequest      *request,
                                                  WebKitWebNavigationAction *navigation_action,
                                                  WebKitWebPolicyDecision   *policy_decision,
                                                  gpointer                   user_data)
{
  IdentifyData *data = user_data;
  const gchar *redirect_uri;
  const gchar *requested_uri;

  /* TODO: use oauth_proxy_extract_access_token() */

  requested_uri = webkit_network_request_get_uri (request);
  redirect_uri = goa_oauth_provider_get_callback_uri (data->provider);
  if (g_str_has_prefix (requested_uri, redirect_uri))
    {
      SoupMessage *message;
      SoupURI *uri;
      GHashTable *key_value_pairs;

      message = webkit_network_request_get_message (request);
      uri = soup_message_get_uri (message);
      key_value_pairs = soup_form_decode (uri->query);

      /* TODO: error handling? */
      data->oauth_verifier = g_strdup (g_hash_table_lookup (key_value_pairs, "oauth_verifier"));
      if (data->oauth_verifier != NULL)
        {
          gtk_dialog_response (data->dialog, GTK_RESPONSE_OK);
        }
      g_hash_table_unref (key_value_pairs);
      webkit_web_policy_decision_ignore (policy_decision);
      return TRUE; /* ignore the request */
    }
  else
    {
      return FALSE; /* make default behavior apply */
    }
}

static void
on_entry_changed (GtkEditable *editable,
                  gpointer     user_data)
{
  IdentifyData *data = user_data;
  gboolean sensitive;

  g_free (data->oauth_verifier);
  data->oauth_verifier = g_strdup (gtk_entry_get_text (GTK_ENTRY (editable)));
  sensitive = data->oauth_verifier != NULL && (strlen (data->oauth_verifier) > 0);
  gtk_dialog_set_response_sensitive (data->dialog, GTK_RESPONSE_OK, sensitive);
}

static gboolean
get_tokens_and_identity (GoaOAuthProvider *provider,
                         GtkDialog        *dialog,
                         GtkBox           *vbox,
                         gchar           **out_access_token,
                         gchar           **out_access_token_secret,
                         gint             *out_access_token_expires_in,
                         gchar           **out_session_handle,
                         gint             *out_session_handle_expires_in,
                         gchar           **out_identity,
                         gchar           **out_presentation_identity,
                         GError          **error)
{
  gboolean ret;
  gchar *url;
  IdentifyData data;
  gchar *escaped_request_token;
  RestProxy *proxy;
  RestProxyCall *call;
  GHashTable *f;
  gboolean use_external_browser;
  gchar **request_params;
  guint n;

  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), FALSE);
  g_return_val_if_fail (GTK_IS_DIALOG (dialog), FALSE);
  g_return_val_if_fail (GTK_IS_BOX (vbox), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  ret = FALSE;
  escaped_request_token = NULL;
  proxy = NULL;
  call = NULL;
  url = NULL;
  request_params = NULL;

  use_external_browser = goa_oauth_provider_get_use_external_browser (provider);

  /* TODO: check with NM whether we're online, if not - return error */

  memset (&data, '\0', sizeof (IdentifyData));
  data.provider = provider;
  data.loop = g_main_loop_new (NULL, FALSE);

  /* TODO: run in worker thread */
  proxy = oauth_proxy_new (goa_oauth_provider_get_consumer_key (provider),
                           goa_oauth_provider_get_consumer_secret (provider),
                           goa_oauth_provider_get_request_uri (provider), FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_method (call, "POST");
  if (use_external_browser)
    rest_proxy_call_add_param (call, "oauth_callback", "oob");
  else
    rest_proxy_call_add_param (call, "oauth_callback", goa_oauth_provider_get_callback_uri (provider));

  request_params = goa_oauth_provider_get_request_uri_params (provider);
  if (request_params != NULL)
    {
      g_assert (g_strv_length (request_params) % 2 == 0);
      for (n = 0; request_params[n] != NULL; n += 2)
        rest_proxy_call_add_param (call, request_params[n], request_params[n+1]);
    }
  if (!rest_proxy_call_sync (call, &data.error))
    {
      g_prefix_error (&data.error, _("Error getting a Request Token: "));
      goto out;
    }
  if (rest_proxy_call_get_status_code (call) != 200)
    {
      g_set_error (&data.error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   /* Translators: the %d is a HTTP status code and the %s is a textual description of it */
                   _("Expected status 200 for getting a Request Token, instead got status %d (%s)"),
                   rest_proxy_call_get_status_code (call),
                   rest_proxy_call_get_status_message (call));
      goto out;
    }
  f = soup_form_decode (rest_proxy_call_get_payload (call));
  data.request_token = g_strdup (g_hash_table_lookup (f, "oauth_token"));
  data.request_token_secret = g_strdup (g_hash_table_lookup (f, "oauth_token_secret"));
  g_hash_table_unref (f);
  if (data.request_token == NULL || data.request_token_secret == NULL)
    {
      g_set_error (&data.error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   _("Missing request_token or request_token_secret headers in response"));
      goto out;
    }

  escaped_request_token = g_uri_escape_string (data.request_token, NULL, TRUE);
  url = goa_oauth_provider_build_authorization_uri (provider,
                                                            goa_oauth_provider_get_authorization_uri (provider),
                                                            escaped_request_token);
  if (use_external_browser)
    {
      GtkWidget *label;
      GtkWidget *entry;
      gchar *escaped_url;
      gchar *markup;

      escaped_url = g_markup_escape_text (url, -1);
      /* Translators: The verb "Paste" is used when asking the user to paste a string from a web browser window */
      markup = g_strdup_printf (_("Paste token obtained from the <a href=\"%s\">authorization page</a>:"),
                                escaped_url);
      g_free (escaped_url);

      label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (label), markup);
      g_free (markup);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);
      entry = gtk_entry_new ();
      gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
      gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, TRUE, 0);
      gtk_widget_grab_focus (entry);
      gtk_widget_show_all (GTK_WIDGET (vbox));

      gtk_dialog_add_button (dialog, GTK_STOCK_OK, GTK_RESPONSE_OK);
      gtk_dialog_set_default_response (dialog, GTK_RESPONSE_OK);
      gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK, FALSE);
      g_signal_connect (entry, "changed", G_CALLBACK (on_entry_changed), &data);

      if (!gtk_show_uri (NULL,
                         url,
                         GDK_CURRENT_TIME,
                         &data.error))
        {
          goto out;
        }
    }
  else
    {
      GtkWidget *scrolled_window;
      GtkWidget *web_view;
      SoupSession *webkit_soup_session;
      SoupCookieJar *cookie_jar;

      webkit_soup_session = webkit_get_default_session ();
      /* Get the proxy configuration from the GNOME settings */
      soup_session_add_feature_by_type (webkit_soup_session, SOUP_TYPE_PROXY_RESOLVER_GNOME);

      /* Ensure we use an empty non-persistent cookie to avoid login
       * credentials being reused...
       */
      soup_session_remove_feature_by_type (webkit_soup_session, SOUP_TYPE_COOKIE_JAR);
      cookie_jar = soup_cookie_jar_new ();
      soup_session_add_feature (webkit_soup_session, SOUP_SESSION_FEATURE (cookie_jar));
      g_object_unref (cookie_jar);

      /* TODO: we might need to add some more web browser UI to make this
       * work...
       */
      web_view = webkit_web_view_new ();
      webkit_web_view_load_uri (WEBKIT_WEB_VIEW (web_view), url);
      g_signal_connect (web_view,
                        "navigation-policy-decision-requested",
                        G_CALLBACK (on_web_view_navigation_policy_decision_requested),
                        &data);

      scrolled_window = gtk_scrolled_window_new (NULL, NULL);
      gtk_widget_set_size_request (scrolled_window, 500, 400);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                      GTK_POLICY_AUTOMATIC,
                                      GTK_POLICY_AUTOMATIC);
      gtk_container_add (GTK_CONTAINER (scrolled_window), web_view);
      gtk_container_add (GTK_CONTAINER (vbox), scrolled_window);

      gtk_widget_show_all (scrolled_window);
    }
  data.dialog = dialog;
  gtk_dialog_run (GTK_DIALOG (dialog));
  if (data.oauth_verifier == NULL)
    {
      if (data.error == NULL)
        {
          g_set_error (&data.error,
                       GOA_ERROR,
                       GOA_ERROR_DIALOG_DISMISSED,
                       _("Dialog was dismissed"));
        }
      goto out;
    }
  g_assert (data.error == NULL);

  /* OK, we are done interacting with the user ... but before we can
   * make up our mind, there are two more RPC calls to make and these
   * call may take some time. So hide the dialog immediately.
   */
  gtk_widget_hide (GTK_WIDGET (dialog));

  /* OK, we now have the request token... we can exchange that for a
   * (short-lived) access token and session_handle (used to refresh the
   * access token)..
   */

  /* TODO: run in worker thread */
  data.access_token = get_tokens_sync (provider,
                                       data.request_token,
                                       data.request_token_secret,
                                       NULL, /* session_handle */
                                       data.oauth_verifier,
                                       &data.access_token_secret,
                                       &data.access_token_expires_in,
                                       &data.session_handle,
                                       &data.session_handle_expires_in,
                                       NULL, /* GCancellable */
                                       &data.error);
  if (data.access_token == NULL)
    {
      g_prefix_error (&data.error, _("Error getting an Access Token: "));
      goto out;
    }

  /* TODO: run in worker thread */
  data.identity = goa_oauth_provider_get_identity_sync (provider,
                                                                data.access_token,
                                                                data.access_token_secret,
                                                                &data.presentation_identity,
                                                                NULL, /* TODO: GCancellable */
                                                                &data.error);
  if (data.identity == NULL)
    {
      g_prefix_error (&data.error, _("Error getting identity: "));
      goto out;
    }

  ret = TRUE;

 out:
  if (call != NULL)
    g_object_unref (call);

  if (ret)
    {
      g_warn_if_fail (data.error == NULL);
      if (out_access_token != NULL)
        *out_access_token = g_strdup (data.access_token);
      if (out_access_token_secret != NULL)
        *out_access_token_secret = g_strdup (data.access_token_secret);
      if (out_access_token_expires_in != NULL)
        *out_access_token_expires_in = data.access_token_expires_in;
      if (out_session_handle != NULL)
        *out_session_handle = g_strdup (data.session_handle);
      if (out_session_handle_expires_in != NULL)
        *out_session_handle_expires_in = data.session_handle_expires_in;
      if (out_identity != NULL)
        *out_identity = g_strdup (data.identity);
      if (out_presentation_identity != NULL)
        *out_presentation_identity = g_strdup (data.presentation_identity);
    }
  else
    {
      g_warn_if_fail (data.error != NULL);
      g_propagate_error (error, data.error);
    }

  g_free (data.presentation_identity);
  g_free (data.identity);
  g_free (url);

  g_free (data.oauth_verifier);
  if (data.loop != NULL)
    g_main_loop_unref (data.loop);
  g_free (data.access_token);
  g_free (data.access_token_secret);
  g_free (escaped_request_token);

  g_free (data.request_token);
  g_free (data.request_token_secret);

  g_strfreev (request_params);
  if (proxy != NULL)
    g_object_unref (proxy);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  GError *error;
  GMainLoop *loop;
  gchar *account_object_path;
} AddData;

static void
add_account_cb (GoaManager   *manager,
                GAsyncResult *res,
                gpointer      user_data)
{
  AddData *data = user_data;
  goa_manager_call_add_account_finish (manager,
                                       &data->account_object_path,
                                       res,
                                       &data->error);
  g_main_loop_quit (data->loop);
}

static gint64
duration_to_abs_usec (gint duration_sec)
{
  gint64 ret;
  GTimeVal now;

  g_get_current_time (&now);
  ret = ((gint64) now.tv_sec) * 1000L * 1000L + ((gint64) now.tv_usec);
  ret += ((gint64) duration_sec) * 1000L * 1000L;
  return ret;
}

static gint
abs_usec_to_duration (gint64 abs_usec)
{
  gint64 ret;
  GTimeVal now;

  g_get_current_time (&now);
  ret = abs_usec - (((gint64) now.tv_sec) * 1000L * 1000L + ((gint64) now.tv_usec));
  ret /= 1000L * 1000L;
  return ret;
}

static GoaObject *
goa_oauth_provider_add_account (GoaProvider *_provider,
                                GoaClient   *client,
                                GtkDialog   *dialog,
                                GtkBox      *vbox,
                                GError     **error)
{
  GoaOAuthProvider *provider = GOA_OAUTH_PROVIDER (_provider);
  GoaObject *ret;
  gchar *access_token;
  gchar *access_token_secret;
  gint access_token_expires_in;
  gchar *session_handle;
  gint session_handle_expires_in;
  gchar *identity;
  gchar *presentation_identity;
  GList *accounts;
  GList *l;
  AddData data;
  GVariantBuilder builder;

  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  g_return_val_if_fail (GOA_IS_CLIENT (client), NULL);
  g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
  g_return_val_if_fail (GTK_IS_BOX (vbox), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  ret = NULL;
  access_token = NULL;
  access_token_secret = NULL;
  session_handle = NULL;
  identity = NULL;
  presentation_identity = NULL;
  accounts = NULL;

  memset (&data, '\0', sizeof (AddData));
  data.loop = g_main_loop_new (NULL, FALSE);

  if (!get_tokens_and_identity (provider,
                                dialog,
                                vbox,
                                &access_token,
                                &access_token_secret,
                                &access_token_expires_in,
                                &session_handle,
                                &session_handle_expires_in,
                                &identity,
                                &presentation_identity,
                                &data.error))
    goto out;

  /* OK, got the identity... see if there's already an account
   * of this type with the given identity
   */
  accounts = goa_client_get_accounts (client);
  for (l = accounts; l != NULL; l = l->next)
    {
      GoaObject *object = GOA_OBJECT (l->data);
      GoaAccount *account;
      GoaOAuthBased *oauth_based;
      const gchar *identity_from_object;

      account = goa_object_peek_account (object);
      oauth_based = goa_object_peek_oauth_based (object);
      if (oauth_based == NULL)
        continue;

      if (g_strcmp0 (goa_account_get_provider_type (account),
                     goa_provider_get_provider_type (GOA_PROVIDER (provider))) != 0)
        continue;

      identity_from_object = goa_account_get_identity (account);
      if (g_strcmp0 (identity_from_object, identity) == 0)
        {
          g_set_error (&data.error,
                       GOA_ERROR,
                       GOA_ERROR_ACCOUNT_EXISTS,
                       _("There is already an account for the identity %s"),
                       identity);
          goto out;
        }
    }

  /* we want the GoaClient to update before this method returns (so it
   * can create a proxy for the new object) so run the mainloop while
   * waiting for this to complete
   */
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{ss}"));
  goa_oauth_provider_add_account_key_values (provider, &builder);
  goa_manager_call_add_account (goa_client_get_manager (client),
                                goa_provider_get_provider_type (GOA_PROVIDER (provider)),
                                identity,
                                presentation_identity,
                                g_variant_builder_end (&builder),
                                NULL, /* GCancellable* */
                                (GAsyncReadyCallback) add_account_cb,
                                &data);
  g_main_loop_run (data.loop);
  if (data.error != NULL)
    goto out;

  ret = GOA_OBJECT (g_dbus_object_manager_get_object (goa_client_get_object_manager (client),
                                                      data.account_object_path));

  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&builder, "{sv}", "access_token", g_variant_new_string (access_token));
  g_variant_builder_add (&builder, "{sv}", "access_token_secret", g_variant_new_string (access_token_secret));
  if (access_token_expires_in > 0)
    g_variant_builder_add (&builder, "{sv}", "access_token_expires_at",
                           g_variant_new_int64 (duration_to_abs_usec (access_token_expires_in)));
  if (session_handle != NULL)
    g_variant_builder_add (&builder, "{sv}", "session_handle", g_variant_new_string (session_handle));
  if (session_handle_expires_in > 0)
    g_variant_builder_add (&builder, "{sv}", "session_handle_expires_at",
                           g_variant_new_int64 (duration_to_abs_usec (session_handle_expires_in)));
  /* TODO: run in worker thread */
  if (!goa_provider_store_credentials_sync (GOA_PROVIDER (provider),
                                            ret,
                                            g_variant_builder_end (&builder),
                                            NULL, /* GCancellable */
                                            &data.error))
    goto out;

 out:
  if (data.error != NULL)
    {
      g_propagate_error (error, data.error);
      g_assert (ret == NULL);
    }
  else
    {
      g_assert (ret != NULL);
    }

  g_list_foreach (accounts, (GFunc) g_object_unref, NULL);
  g_list_free (accounts);
  g_free (identity);
  g_free (presentation_identity);
  g_free (access_token);
  g_free (access_token_secret);
  g_free (session_handle);
  g_free (data.account_object_path);
  if (data.loop != NULL)
    g_main_loop_unref (data.loop);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
goa_oauth_provider_refresh_account (GoaProvider  *_provider,
                                    GoaClient    *client,
                                    GoaObject    *object,
                                    GtkWindow    *parent,
                                    GError      **error)
{
  GoaOAuthProvider *provider = GOA_OAUTH_PROVIDER (_provider);
  GtkWidget *dialog;
  gchar *access_token;
  gchar *access_token_secret;
  gint access_token_expires_in;
  gchar *session_handle;
  gint session_handle_expires_in;
  gchar *identity;
  const gchar *existing_identity;
  GVariantBuilder builder;
  gboolean ret;

  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), FALSE);
  g_return_val_if_fail (GOA_IS_CLIENT (client), FALSE);
  g_return_val_if_fail (GOA_IS_OBJECT (object), FALSE);
  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  access_token = NULL;
  access_token_secret = NULL;
  session_handle = NULL;
  identity = NULL;

  dialog = gtk_dialog_new_with_buttons (NULL,
                                        parent,
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                        NULL);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 12);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_widget_show_all (dialog);

  if (!get_tokens_and_identity (provider,
                                GTK_DIALOG (dialog),
                                GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                                &access_token,
                                &access_token_secret,
                                &access_token_expires_in,
                                &session_handle,
                                &session_handle_expires_in,
                                &identity,
                                NULL, /* out_presentation_identity */
                                error))
    goto out;

  existing_identity = goa_account_get_identity (goa_object_peek_account (object));
  if (g_strcmp0 (identity, existing_identity) != 0)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_FAILED,
                   _("Was asked to login as %s, but logged in as %s"),
                   existing_identity,
                   identity);
      goto out;
    }

  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&builder, "{sv}", "access_token", g_variant_new_string (access_token));
  g_variant_builder_add (&builder, "{sv}", "access_token_secret", g_variant_new_string (access_token_secret));
  if (access_token_expires_in > 0)
    g_variant_builder_add (&builder, "{sv}", "access_token_expires_at",
                           g_variant_new_int64 (duration_to_abs_usec (access_token_expires_in)));
  if (session_handle != NULL)
    g_variant_builder_add (&builder, "{sv}", "session_handle", g_variant_new_string (session_handle));
  if (session_handle_expires_in > 0)
    g_variant_builder_add (&builder, "{sv}", "session_handle_expires_at",
                           g_variant_new_int64 (duration_to_abs_usec (session_handle_expires_in)));
  /* TODO: run in worker thread */
  if (!goa_provider_store_credentials_sync (GOA_PROVIDER (provider),
                                            object,
                                            g_variant_builder_end (&builder),
                                            NULL, /* GCancellable */
                                            error))
    goto out;

  goa_account_call_ensure_credentials (goa_object_peek_account (object),
                                       NULL, /* GCancellable */
                                       NULL, NULL); /* callback, user_data */

  ret = TRUE;

 out:
  gtk_widget_destroy (dialog);

  g_free (identity);
  g_free (access_token);
  g_free (access_token_secret);
  g_free (session_handle);
  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
free_mutex (GMutex *mutex)
{
  g_mutex_free (mutex);
}

/**
 * goa_oauth_provider_get_access_token_sync:
 * @provider: A #GoaOAuthProvider.
 * @object: A #GoaObject.
 * @force_refresh: If set to %TRUE, forces a refresh of the access token, if possible.
 * @out_access_token_secret: (out): The secret for the return access token.
 * @out_access_token_expires_in: (out): Return location for how many seconds the returned token is valid for (0 if unknown) or %NULL.
 * @cancellable: (allow-none): A #GCancellable or %NULL.
 * @error: Return location for error or %NULL.
 *
 * Synchronously gets an access token for @object. The calling thread
 * is blocked while the operation is pending.
 *
 * The resulting token is typically read from the local cache so most
 * of the time only a local roundtrip to the storage for the token
 * cache (e.g. <command>gnome-keyring-daemon</command>) is
 * needed. However, the operation may involve refreshing the token
 * with the service provider so a full network round-trip may be
 * needed.
 *
 * Note that multiple calls are serialized to avoid multiple
 * outstanding requests to the service provider.
 *
 * This operation may fail if e.g. unable to refresh the credentials
 * or if network connectivity is not available. Note that even if a
 * token is returned, the returned token isn't guaranteed to work -
 * use goa_provider_ensure_credentials_sync() if you need
 * stronger guarantees.
 *
 * Returns: The access token or %NULL if error is set. The returned
 * string must be freed with g_free().
 */
gchar *
goa_oauth_provider_get_access_token_sync (GoaOAuthProvider   *provider,
                                          GoaObject          *object,
                                          gboolean            force_refresh,
                                          gchar             **out_access_token_secret,
                                          gint               *out_access_token_expires_in,
                                          GCancellable       *cancellable,
                                          GError            **error)
{
  GVariant *credentials;
  GVariantIter iter;
  const gchar *key;
  GVariant *value;
  gchar *access_token;
  gchar *access_token_secret;
  gchar *session_handle;
  gchar *access_token_for_refresh;
  gchar *access_token_secret_for_refresh;
  gchar *session_handle_for_refresh;
  gint access_token_expires_in;
  gint session_handle_expires_in;
  gboolean success;
  GVariantBuilder builder;
  gchar *ret;
  GMutex *lock;

  g_return_val_if_fail (GOA_IS_OAUTH_PROVIDER (provider), NULL);
  g_return_val_if_fail (GOA_IS_OBJECT (object), NULL);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  ret = NULL;
  credentials = NULL;
  access_token = NULL;
  access_token_secret = NULL;
  access_token_expires_in = 0;
  session_handle = NULL;
  session_handle_expires_in = 0;
  access_token_for_refresh = NULL;
  access_token_secret_for_refresh = NULL;
  session_handle_for_refresh = NULL;
  success = FALSE;

  /* provider_lock is too coarse, use a per-object lock instead */
  G_LOCK (provider_lock);
  lock = g_object_get_data (G_OBJECT (object), "-goa-oauth-provider-get-access-token-lock");
  if (lock == NULL)
    {
      lock = g_mutex_new ();
      g_object_set_data_full (G_OBJECT (object),
                              "-goa-oauth-provider-get-access-token-lock",
                              lock,
                              (GDestroyNotify) free_mutex);
    }
  G_UNLOCK (provider_lock);

  g_mutex_lock (lock);

  /* First, get the credentials from the keyring */
  credentials = goa_provider_lookup_credentials_sync (GOA_PROVIDER (provider),
                                                      object,
                                                      cancellable,
                                                      error);
  if (credentials == NULL)
    {
      if (error != NULL)
        {
          g_prefix_error (error, _("Credentials not found in keyring (%s, %d): "),
                          g_quark_to_string ((*error)->domain), (*error)->code);
          (*error)->domain = GOA_ERROR;
          (*error)->code = GOA_ERROR_NOT_AUTHORIZED;
        }
      goto out;
    }

  g_variant_iter_init (&iter, credentials);
  while (g_variant_iter_next (&iter, "{&sv}", &key, &value))
    {
      if (g_strcmp0 (key, "access_token") == 0)
        access_token = g_variant_dup_string (value, NULL);
      else if (g_strcmp0 (key, "access_token_secret") == 0)
        access_token_secret = g_variant_dup_string (value, NULL);
      else if (g_strcmp0 (key, "access_token_expires_at") == 0)
        access_token_expires_in = abs_usec_to_duration (g_variant_get_int64 (value));
      else if (g_strcmp0 (key, "session_handle") == 0)
        session_handle = g_variant_dup_string (value, NULL);
      else if (g_strcmp0 (key, "session_handle_expires_at") == 0)
        session_handle_expires_in = abs_usec_to_duration (g_variant_get_int64 (value));
      g_variant_unref (value);
    }

  if (access_token == NULL || access_token_secret == NULL)
    {
      g_set_error (error,
                   GOA_ERROR,
                   GOA_ERROR_NOT_AUTHORIZED,
                   _("Credentials do not contain access_token or access_token_secret"));
      goto out;
    }

  /* if we can't refresh the token, just return it no matter what */
  if (session_handle == NULL)
    {
      goa_debug ("Returning locally cached credentials that cannot be refreshed");
      success = TRUE;
      goto out;
    }

  /* If access_token is still "fresh enough" (e.g. more than ten
   * minutes of life left in it), just return it unless we've been
   * asked to forcibly refresh it
   */
  if (!force_refresh && access_token_expires_in > 10*60)
    {
      goa_debug ("Returning locally cached credentials (expires in %d seconds)", access_token_expires_in);
      success = TRUE;
      goto out;
    }

  goa_debug ("Refreshing locally cached credentials (expires in %d seconds, force_refresh=%d)", access_token_expires_in, force_refresh);

  /* Otherwise, refresh it */
  access_token_for_refresh        = access_token; access_token = NULL;
  access_token_secret_for_refresh = access_token_secret; access_token_secret = NULL;
  session_handle_for_refresh      = session_handle; session_handle = NULL;
  access_token = get_tokens_sync (provider,
                                  access_token_for_refresh,
                                  access_token_secret_for_refresh,
                                  session_handle_for_refresh,
                                  NULL, /* verifier */
                                  &access_token_secret,
                                  &access_token_expires_in,
                                  &session_handle,
                                  &session_handle_expires_in,
                                  cancellable,
                                  error);
  if (access_token == NULL)
    {
      if (error != NULL)
        {
          g_prefix_error (error, _("Failed to refresh access token (%s, %d): "),
                          g_quark_to_string ((*error)->domain), (*error)->code);
          (*error)->code = is_authorization_error (*error) ? GOA_ERROR_NOT_AUTHORIZED : GOA_ERROR_FAILED;
          (*error)->domain = GOA_ERROR;
        }
      goto out;
    }

  /* Good. Now update the keyring with the refreshed credentials */
  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&builder, "{sv}", "access_token", g_variant_new_string (access_token));
  g_variant_builder_add (&builder, "{sv}", "access_token_secret", g_variant_new_string (access_token_secret));
  if (access_token_expires_in > 0)
    g_variant_builder_add (&builder, "{sv}", "access_token_expires_at",
                           g_variant_new_int64 (duration_to_abs_usec (access_token_expires_in)));
  if (session_handle != NULL)
    g_variant_builder_add (&builder, "{sv}", "session_handle", g_variant_new_string (session_handle));
  if (session_handle_expires_in > 0)
    g_variant_builder_add (&builder, "{sv}", "session_handle_expires_at",
                           g_variant_new_int64 (duration_to_abs_usec (session_handle_expires_in)));

  /* TODO: run in worker thread */
  if (!goa_provider_store_credentials_sync (GOA_PROVIDER (provider),
                                            object,
                                            g_variant_builder_end (&builder),
                                            cancellable,
                                            error))
    {
      if (error != NULL)
        {
          g_prefix_error (error, _("Error storing credentials in keyring (%s, %d): "),
                          g_quark_to_string ((*error)->domain), (*error)->code);
          (*error)->domain = GOA_ERROR;
          (*error)->code = GOA_ERROR_NOT_AUTHORIZED;
        }
      goto out;
    }

  success = TRUE;

 out:
  if (success)
    {
      ret = access_token; access_token = NULL;
      g_assert (ret != NULL);
      if (out_access_token_secret != NULL)
        {
          *out_access_token_secret = access_token_secret; access_token_secret = NULL;
        }
      if (out_access_token_expires_in != NULL)
        *out_access_token_expires_in = access_token_expires_in;
    }
  g_free (access_token);
  g_free (access_token_secret);
  g_free (session_handle);
  g_free (access_token_for_refresh);
  g_free (access_token_secret_for_refresh);
  g_free (session_handle_for_refresh);
  if (credentials != NULL)
    g_variant_unref (credentials);

  g_mutex_unlock (lock);

  return ret;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean on_handle_get_access_token (GoaOAuthBased         *object,
                                            GDBusMethodInvocation *invocation,
                                            gpointer               user_data);

static gboolean
goa_oauth_provider_build_object (GoaProvider         *provider,
                                 GoaObjectSkeleton   *object,
                                 GKeyFile            *key_file,
                                 const gchar         *group,
                                 GError             **error)
{
  GoaOAuthBased *oauth_based;
  gchar *identity;

  identity = NULL;

  oauth_based = goa_object_get_oauth_based (GOA_OBJECT (object));
  if (oauth_based != NULL)
    goto out;

  oauth_based = goa_oauth_based_skeleton_new ();
  goa_oauth_based_set_consumer_key (oauth_based,
                                    goa_oauth_provider_get_consumer_key (GOA_OAUTH_PROVIDER (provider)));
  goa_oauth_based_set_consumer_secret (oauth_based,
                                       goa_oauth_provider_get_consumer_secret (GOA_OAUTH_PROVIDER (provider)));
  /* Ensure D-Bus method invocations run in their own thread */
  g_dbus_interface_skeleton_set_flags (G_DBUS_INTERFACE_SKELETON (oauth_based),
                                       G_DBUS_INTERFACE_SKELETON_FLAGS_HANDLE_METHOD_INVOCATIONS_IN_THREAD);
  goa_object_skeleton_set_oauth_based (object, oauth_based);
  g_signal_connect (oauth_based,
                    "handle-get-access-token",
                    G_CALLBACK (on_handle_get_access_token),
                    NULL);

 out:
  g_object_unref (oauth_based);
  g_free (identity);
  return TRUE;
}

/* ---------------------------------------------------------------------------------------------------- */

static gboolean
goa_oauth_provider_ensure_credentials_sync (GoaProvider    *_provider,
                                            GoaObject      *object,
                                            gint           *out_expires_in,
                                            GCancellable   *cancellable,
                                            GError        **error)
{
  GoaOAuthProvider *provider = GOA_OAUTH_PROVIDER (_provider);
  gboolean ret;
  gchar *access_token;
  gchar *access_token_secret;
  gint access_token_expires_in;
  gchar *identity;
  gboolean force_refresh;

  ret = FALSE;
  access_token = NULL;
  access_token_secret = NULL;
  identity = NULL;
  force_refresh = FALSE;

 again:
  access_token = goa_oauth_provider_get_access_token_sync (provider,
                                                                   object,
                                                                   force_refresh,
                                                                   &access_token_secret,
                                                                   &access_token_expires_in,
                                                                   cancellable,
                                                                   error);
  if (access_token == NULL)
    goto out;

  identity = goa_oauth_provider_get_identity_sync (provider,
                                                           access_token,
                                                           access_token_secret,
                                                           NULL, /* out_presentation_identity */
                                                           cancellable,
                                                           error);
  if (identity == NULL)
    {
      /* OK, try again, with forcing the locally cached credentials to be refreshed */
      if (!force_refresh)
        {
          force_refresh = TRUE;
          g_free (access_token); access_token = NULL;
          g_free (access_token_secret); access_token_secret = NULL;
          goto again;
        }
      else
        {
          goto out;
        }
    }

  /* TODO: maybe check with the identity we have */
  ret = TRUE;
  if (out_expires_in != NULL)
    *out_expires_in = access_token_expires_in;

 out:
  g_free (identity);
  g_free (access_token);
  g_free (access_token_secret);
  return ret;
}


/* ---------------------------------------------------------------------------------------------------- */

static void
goa_oauth_provider_init (GoaOAuthProvider *client)
{
}

static void
goa_oauth_provider_class_init (GoaOAuthProviderClass *klass)
{
  GoaProviderClass *provider_class;

  provider_class = GOA_PROVIDER_CLASS (klass);
  provider_class->add_account                = goa_oauth_provider_add_account;
  provider_class->refresh_account            = goa_oauth_provider_refresh_account;
  provider_class->build_object               = goa_oauth_provider_build_object;
  provider_class->ensure_credentials_sync    = goa_oauth_provider_ensure_credentials_sync;

  klass->build_authorization_uri  = goa_oauth_provider_build_authorization_uri_default;
  klass->get_use_external_browser = goa_oauth_provider_get_use_external_browser_default;
  klass->get_request_uri_params   = goa_oauth_provider_get_request_uri_params_default;
  klass->add_account_key_values   = goa_oauth_provider_add_account_key_values_default;
}

/* ---------------------------------------------------------------------------------------------------- */

/* runs in a thread dedicated to handling @invocation */
static gboolean
on_handle_get_access_token (GoaOAuthBased         *interface,
                            GDBusMethodInvocation *invocation,
                            gpointer               user_data)
{
  GoaObject *object;
  GoaAccount *account;
  GoaProvider *provider;
  GError *error;
  gchar *access_token;
  gchar *access_token_secret;
  gint access_token_expires_in;

  /* TODO: maybe log what app is requesting access */

  access_token = NULL;
  access_token_secret = NULL;

  object = GOA_OBJECT (g_dbus_interface_get_object (G_DBUS_INTERFACE (interface)));
  account = goa_object_peek_account (object);
  provider = goa_provider_get_for_provider_type (goa_account_get_provider_type (account));

  error = NULL;
  access_token = goa_oauth_provider_get_access_token_sync (GOA_OAUTH_PROVIDER (provider),
                                                                   object,
                                                                   FALSE, /* force_refresh */
                                                                   &access_token_secret,
                                                                   &access_token_expires_in,
                                                                   NULL, /* GCancellable* */
                                                                   &error);
  if (access_token == NULL)
    {
      g_dbus_method_invocation_return_gerror (invocation, error);
      g_error_free (error);
    }
  else
    {
      goa_oauth_based_complete_get_access_token (interface,
                                                 invocation,
                                                 access_token,
                                                 access_token_secret,
                                                 access_token_expires_in);
    }
  g_free (access_token);
  g_free (access_token_secret);
  g_object_unref (provider);
  return TRUE; /* invocation was handled */
}

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright 2009-2010  Red Hat, Inc,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Based on panels/user-accounts/um-editable-entry.c from
 * gnome-control-center by Matthias Clasen <mclasen@redhat.com>
 */

#ifndef _GOA_EDITABLE_LABEL_H_
#define _GOA_EDITABLE_LABEL_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GOA_TYPE_EDITABLE_LABEL  goa_editable_label_get_type()

#define GOA_EDITABLE_LABEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOA_TYPE_EDITABLE_LABEL, GoaEditableLabel))
#define GOA_EDITABLE_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GOA_TYPE_EDITABLE_LABEL, GoaEditableLabelClass))
#define GOA_IS_EDITABLE_LABEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOA_TYPE_EDITABLE_LABEL))
#define GOA_IS_EDITABLE_LABEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GOA_TYPE_EDITABLE_LABEL))
#define GOA_EDITABLE_LABEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GOA_TYPE_EDITABLE_LABEL, GoaEditableLabelClass))

typedef struct _GoaEditableLabel GoaEditableLabel;
typedef struct _GoaEditableLabelClass GoaEditableLabelClass;
typedef struct _GoaEditableLabelPrivate GoaEditableLabelPrivate;

struct _GoaEditableLabel
{
  GtkAlignment parent;

  GoaEditableLabelPrivate *priv;
};

struct _GoaEditableLabelClass
{
  GtkAlignmentClass parent_class;

  void (* editing_done) (GoaEditableLabel *entry);
};

GType        goa_editable_label_get_type     (void) G_GNUC_CONST;
GtkWidget   *goa_editable_label_new          (void);
void         goa_editable_label_set_text     (GoaEditableLabel *entry,
                                              const gchar     *text);
const gchar *goa_editable_label_get_text     (GoaEditableLabel *entry);
void         goa_editable_label_set_editable (GoaEditableLabel *entry,
                                              gboolean         editable);
gboolean     goa_editable_label_get_editable (GoaEditableLabel *entry);
void         goa_editable_label_set_weight   (GoaEditableLabel *entry,
                                              gint             weight);
gint         goa_editable_label_get_weight   (GoaEditableLabel *entry);
void         goa_editable_label_set_scale    (GoaEditableLabel *entry,
                                              gdouble          scale);
gdouble      goa_editable_label_get_scale    (GoaEditableLabel *entry);

G_END_DECLS

#endif /* _GOA_EDITABLE_LABEL_H_ */

/* GTK - The GIMP Toolkit
 * Copyright (C) 2014,2015 Benjamin Otte
 * 
 * Authors: Benjamin Otte <otte@gnome.org>
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gtkrendericonprivate.h"

#include "gtkcssfiltervalueprivate.h"
#include "gtkcssimagevalueprivate.h"
#include "gtkcssshadowvalueprivate.h"
#include "gtkcssstyleprivate.h"
#include "gtkcsstransformvalueprivate.h"
#include "gtkiconthemeprivate.h"
#include "gtksnapshot.h"
#include "gsktransform.h"

#include <math.h>

void
gtk_css_style_snapshot_icon (GtkCssStyle            *style,
                             GtkSnapshot            *snapshot,
                             double                  width,
                             double                  height)
{
  GskTransform *transform;
  GtkCssImage *image;
  gboolean has_shadow;

  g_return_if_fail (GTK_IS_CSS_STYLE (style));
  g_return_if_fail (snapshot != NULL);

  if (width == 0.0 || height == 0.0)
    return;

  image = _gtk_css_image_value_get_image (style->other->icon_source);
  if (image == NULL)
    return;

  transform = gtk_css_transform_value_get_transform (style->other->icon_transform);

  gtk_snapshot_push_debug (snapshot, "CSS Icon @ %gx%g", width, height);

  gtk_css_filter_value_push_snapshot (style->other->icon_filter, snapshot);

  has_shadow = gtk_css_shadow_value_push_snapshot (style->icon->icon_shadow, snapshot);

  if (transform == NULL)
    {
      gtk_css_image_snapshot (image, snapshot, width, height);
    }
  else
    {
      gtk_snapshot_save (snapshot);

      /* XXX: Implement -gtk-icon-transform-origin instead of hardcoding "50% 50%" here */
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (width / 2.0, height / 2.0));
      gtk_snapshot_transform (snapshot, transform);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- width / 2.0, - height / 2.0));

      gtk_css_image_snapshot (image, snapshot, width, height);

      gtk_snapshot_restore (snapshot);
    }

  if (has_shadow)
    gtk_snapshot_pop (snapshot);

  gtk_css_filter_value_pop_snapshot (style->other->icon_filter, snapshot);

  gtk_snapshot_pop (snapshot);

  gsk_transform_unref (transform);
}

void
gtk_css_style_snapshot_icon_paintable (GtkCssStyle  *style,
                                       GtkSnapshot  *snapshot,
                                       GdkPaintable *paintable,
                                       double        width,
                                       double        height,
                                       gboolean      recolor)
{
  GskTransform *transform;
  gboolean has_shadow;

  g_return_if_fail (GTK_IS_CSS_STYLE (style));
  g_return_if_fail (snapshot != NULL);
  g_return_if_fail (GDK_IS_PAINTABLE (paintable));
  g_return_if_fail (width > 0);
  g_return_if_fail (height > 0);

  transform = gtk_css_transform_value_get_transform (style->other->icon_transform);

  gtk_css_filter_value_push_snapshot (style->other->icon_filter, snapshot);

  has_shadow = gtk_css_shadow_value_push_snapshot (style->icon->icon_shadow, snapshot);

  if (recolor)
    {
      graphene_matrix_t color_matrix;
      graphene_vec4_t color_offset;
      GdkRGBA fg, sc, wc, ec;

      gtk_icon_theme_lookup_symbolic_colors (style, &fg, &sc, &wc, &ec);

      if (fg.alpha == 0.0f)
        goto transparent;

      graphene_matrix_init_from_float (&color_matrix, (float[16]) {
                                         sc.red - fg.red, sc.green - fg.green, sc.blue - fg.blue, 0,
                                         wc.red - fg.red, wc.green - fg.green, wc.blue - fg.blue, 0,
                                         ec.red - fg.red, ec.green - fg.green, ec.blue - fg.blue, 0,
                                         0, 0, 0, fg.alpha
                                       });
      graphene_vec4_init (&color_offset, fg.red, fg.green, fg.blue, 0);

      gtk_snapshot_push_color_matrix (snapshot, &color_matrix, &color_offset);
    }

  if (transform == NULL)
    {
      gdk_paintable_snapshot (paintable, snapshot, width, height);
    }
  else
    {
      gtk_snapshot_save (snapshot);

      /* XXX: Implement -gtk-icon-transform-origin instead of hardcoding "50% 50%" here */
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (width / 2.0, height / 2.0));
      gtk_snapshot_transform (snapshot, transform);
      gtk_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (- width / 2.0, - height / 2.0));

      gdk_paintable_snapshot (paintable, snapshot, width, height);

      gtk_snapshot_restore (snapshot);
    }

  if (recolor)
    gtk_snapshot_pop (snapshot);

transparent:
  if (has_shadow)
    gtk_snapshot_pop (snapshot);

  gtk_css_filter_value_pop_snapshot (style->other->icon_filter, snapshot);

  gsk_transform_unref (transform);
}

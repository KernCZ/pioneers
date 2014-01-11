#ifdef HAVE_OLD_GTK
#define gtk_combo_box_text_new(A) gtk_combo_box_new_text(A)
#define gtk_combo_box_text_append_text(A, B) gtk_combo_box_append_text(A, B)
#define GTK_COMBO_BOX_TEXT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_COMBO_BOX, GtkComboBox))
#define gdk_pixmap_get_size(A, B, C) gdk_drawable_get_size(A, B, C)

#ifdef GSEAL_ENABLE
#error "Don't use --enable-deprecation-checks when using old versions of Gtk"
#endif
#define gdk_visual_get_depth(A) A->depth
#define gtk_table_get_size(A, B, C) row = A->nrows
#endif

#ifndef HAVE_GTK3
#define gtk_box_new(A, B) (A == GTK_ORIENTATION_HORIZONTAL ? gtk_hbox_new(FALSE, B) : gtk_vbox_new(FALSE, B))
#define gtk_separator_new(A) (A == GTK_ORIENTATION_HORIZONTAL ? gtk_hseparator_new() : gtk_vseparator_new())
#define gtk_button_box_new(A) (A == GTK_ORIENTATION_HORIZONTAL ? gtk_hbutton_box_new() : gtk_vbutton_box_new())
#define gtk_paned_new(A) (A == GTK_ORIENTATION_HORIZONTAL ? gtk_hpaned_new() : gtk_vpaned_new())
#define gtk_scale_new_with_range(A, B, C, D) (A == GTK_ORIENTATION_HORIZONTAL ? gtk_hscale_new_with_range(B, C, D) : gtk_vscale_new_with_range(B, C, D))
#endif

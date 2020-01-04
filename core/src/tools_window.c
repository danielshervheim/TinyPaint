//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#include "tools_window.h"

struct _ToolsWindow {
    GtkWindow parent_instance;

    // instance properties
    Tool *m_tool;
    int m_recurse_flag;

    // parameter buttons
    GtkColorButton *m_toolColor;
    GtkAdjustment *m_toolRadius;

    // toggle buttons
    GtkToggleButton *m_pencilButton;
    GtkToggleButton *m_brushButton;
    GtkToggleButton *m_markerButton;
    GtkToggleButton *m_sprayCanButton;
    GtkToggleButton *m_floodFillButton;
    GtkToggleButton *m_eraserButton;
};

G_DEFINE_TYPE(ToolsWindow, tools_window, GTK_TYPE_WINDOW);



//
// SIGNAL handlers
//

/* Fires when the user presses a tool button from the palette */
void on_toolButton_toggled(GtkToggleButton *button, gpointer user_data) {
    ToolsWindow *self = (ToolsWindow *)user_data;

    if (self->m_tool == NULL) {
        printf("ERROR: m_tool has not been assigned yet!\n"); fflush(stdout);
        return;
    }

    /* calling "gtk_toggle_button_set_active()" as we must do, causes the "on_toolButton_toggled"
    signal to fire again, which if left unchecked leads to infinite recursion.
    This flag is to prevent recursing farther than a depth of 1. */
    if (self->m_recurse_flag == 1) {
        return;
    }
    else {
        self->m_recurse_flag = 1;
    }

    // set all toggles to false
    gtk_toggle_button_set_active(self->m_pencilButton, 0);
    gtk_toggle_button_set_active(self->m_brushButton, 0);
    gtk_toggle_button_set_active(self->m_markerButton, 0);
    gtk_toggle_button_set_active(self->m_sprayCanButton, 0);
    gtk_toggle_button_set_active(self->m_floodFillButton, 0);
    gtk_toggle_button_set_active(self->m_eraserButton, 0);

    if (button == self->m_pencilButton) {
        gtk_toggle_button_set_active(self->m_pencilButton, 1);
        self->m_tool->tooltype = PENCIL;
    }
    else if (button == self->m_brushButton) {
        gtk_toggle_button_set_active(self->m_brushButton, 1);
        self->m_tool->tooltype = BRUSH;
    }
    else if (button == self->m_markerButton) {
        gtk_toggle_button_set_active(self->m_markerButton, 1);
        self->m_tool->tooltype = MARKER;
    }
    else if (button == self->m_sprayCanButton) {
        gtk_toggle_button_set_active(self->m_sprayCanButton, 1);
        self->m_tool->tooltype = SPRAYCAN;
    }
    else if (button == self->m_floodFillButton) {
        gtk_toggle_button_set_active(self->m_floodFillButton, 1);
        self->m_tool->tooltype = FLOODFILL;
    }
    else if (button == self->m_eraserButton) {
        gtk_toggle_button_set_active(self->m_eraserButton, 1);
        self->m_tool->tooltype = ERASER;
    }

    // update the current tool
    tool_update_mask(self->m_tool);
    tool_reset_parameters(self->m_tool);

    // done recursing
    self->m_recurse_flag = 0;
}

/* Gets the height from the adjustment and updates this instance's tool's radius property */
void on_toolRadius_value_changed(GtkAdjustment *adjustment, gpointer user_data) {
    ToolsWindow *self = (ToolsWindow *)user_data;

    if (self->m_tool == NULL) {
        printf("ERROR: cannot set radius. Tool has not been set.\n"); fflush(stdout);
        return;
    }

    self->m_tool->radius = (int)gtk_adjustment_get_value(adjustment);
    tool_update_mask(self->m_tool);
}

/* Gets the color from the colorChooser and updates this instance's tools' color property */
void on_toolColor_color_set(GtkColorButton *button, gpointer user_data) {
    ToolsWindow *self = (ToolsWindow *)user_data;

    if (self->m_tool == NULL) {
        printf("ERROR: cannot set color. Tool has not been set.\n"); fflush(stdout);
        return;
    }

    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(button), &(self->m_tool->color));
}



//
// LINK methods
//

/* Links an instance of Tool to this ToolsWindow */
void tools_window_link_tool(ToolsWindow *self, Tool *tool) {
    if (self->m_tool != NULL) {
        printf("ERROR: this ToolsWindow already has a Tool assigned.\n"); fflush(stdout);
    }
    else {
        self->m_tool = tool;

        if (self->m_tool->mask != NULL) {
            printf("WARNING: attempting to link Tool with non-null mask ");
            printf("property. Did you initialize the tool with tool_new()?\n");
            fflush(stdout);
        }

        // set the default tool to pencil
        on_toolButton_toggled(self->m_pencilButton, self);

        // and set the default color and radius parameters
        on_toolColor_color_set(self->m_toolColor, self);
        on_toolRadius_value_changed(self->m_toolRadius, self);
    }
}

/* Links this ToolsWindow instance to an EditorWindow as its parent */
void tools_window_link_editorWindow(ToolsWindow *self, EditorWindow *parent) {
    gtk_window_set_transient_for(GTK_WINDOW(self), GTK_WINDOW(parent));
    gtk_window_set_attached_to(GTK_WINDOW(self), GTK_WIDGET(parent));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(self), 1);
}



//
// G Callbacks
//

/* Initializies the instance of ToolsWindow */
static void tools_window_init (ToolsWindow *self) {
    // set the window title
    gtk_window_set_title(GTK_WINDOW(self), "Tools");

    // set the window settings (cant delete, cant resize, not in taskbar or pager,
    // should stay on top of its parent window)
    gtk_window_set_deletable(GTK_WINDOW(self), 0);
    gtk_window_set_resizable(GTK_WINDOW(self), 0);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(self), 1);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(self), 1);
    gtk_window_set_type_hint(GTK_WINDOW(self), GDK_WINDOW_TYPE_HINT_DIALOG);

    // create a gtk builder from the resources
    GtkBuilder *builder = gtk_builder_new_from_resource("/tinypaint/ui/tools_window.glade");

    // get a reference to the toolsGrid
    GtkWidget *toolsGrid = GTK_WIDGET(gtk_builder_get_object(builder, "toolsGrid"));

    // and add it to this window
    gtk_container_add(GTK_CONTAINER(self), toolsGrid);

    // get references to all the buttons
    self->m_pencilButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "pencilButton"));
    self->m_brushButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "brushButton"));
    self->m_markerButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "markerButton"));
    self->m_sprayCanButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "sprayCanButton"));
    self->m_floodFillButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "floodFillButton"));
    self->m_eraserButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "eraserButton"));
    GtkMenuButton *toolRadiusMenuButton = GTK_MENU_BUTTON(gtk_builder_get_object(builder, "toolRadiusMenuButton"));

    // add the icons to the buttons
    gtk_button_set_image(GTK_BUTTON(self->m_pencilButton),
        gtk_image_new_from_resource("/tinypaint/icons/icons8-pencil-16.png"));
    gtk_button_set_image(GTK_BUTTON(self->m_brushButton),
        gtk_image_new_from_resource("/tinypaint/icons/icons8-paint-16.png"));
    gtk_button_set_image(GTK_BUTTON(self->m_markerButton),
        gtk_image_new_from_resource("/tinypaint/icons/icons8-chisel-tip-marker-16.png"));
    gtk_button_set_image(GTK_BUTTON(self->m_sprayCanButton),
        gtk_image_new_from_resource("/tinypaint/icons/icons8-paint-sprayer-16.png"));
    gtk_button_set_image(GTK_BUTTON(self->m_floodFillButton),
        gtk_image_new_from_resource("/tinypaint/icons/icons8-fill-color-16.png"));
    gtk_button_set_image(GTK_BUTTON(self->m_eraserButton),
        gtk_image_new_from_resource("/tinypaint/icons/icons8-eraser-16.png"));
    gtk_button_set_image(GTK_BUTTON(toolRadiusMenuButton),
        gtk_image_new_from_resource("/tinypaint/icons/icons8-line-width-16.png"));

    // then connect the appropriate signal handlers for the buttons
    g_signal_connect(self->m_pencilButton, "toggled",
        (GCallback)on_toolButton_toggled, self);
    g_signal_connect(self->m_brushButton, "toggled",
        (GCallback)on_toolButton_toggled, self);
    g_signal_connect(self->m_markerButton, "toggled",
        (GCallback)on_toolButton_toggled, self);
    g_signal_connect(self->m_sprayCanButton, "toggled",
        (GCallback)on_toolButton_toggled, self);
    g_signal_connect(self->m_floodFillButton, "toggled",
        (GCallback)on_toolButton_toggled, self);
    g_signal_connect(self->m_eraserButton, "toggled",
        (GCallback)on_toolButton_toggled, self);

    // get references to the tool radius and color widgets
    self->m_toolRadius = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "toolRadius"));
    self->m_toolColor = GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "toolColor"));

    // and connect the appropriate signal handlers
    g_signal_connect(self->m_toolRadius, "value-changed", (GCallback)on_toolRadius_value_changed, self);
    g_signal_connect(self->m_toolColor, "color-set", (GCallback)on_toolColor_color_set, self);

    // set the initial instance parameters to their defaults
    self->m_tool = NULL;
    self->m_recurse_flag = 0;

    // show the grid
    gtk_widget_show_all(toolsGrid);

    // and finally, unref the builder
    g_object_unref(builder);
}

/* Returns a new instance of ToolsWindow */
ToolsWindow* tools_window_new () {
    return g_object_new (TOOLS_WINDOW_TYPE_WINDOW, NULL);
}

/* Initializes the ToolsWindow class */
static void tools_window_class_init (ToolsWindowClass *class) {
    // not necessary???
    // GObject property stuff would go here...
}

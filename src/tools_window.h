//
// Copyright © Daniel Shervheim, 2019
// danielshervheim@gmail.com
// www.github.com/danielshervheim
//

#ifndef TOOLS_WINDOW_H_
#define TOOLS_WINDOW_H_

#include "tool.h"

#include "editor_window.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TOOLS_WINDOW_TYPE_WINDOW (tools_window_get_type ())
G_DECLARE_FINAL_TYPE(ToolsWindow, tools_window, TOOLS_WINDOW, WINDOW, GtkWindow)

ToolsWindow* tools_window_new(void);

void tools_window_link_tool(ToolsWindow *self, Tool *tool);

void tools_window_link_editorWindow(ToolsWindow *self, EditorWindow *parent);

G_END_DECLS

#endif  // TOOLS_WINDOW_H_

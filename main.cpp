#include "editor_config.h"
#include "splay_tree.h"
#include "utils.h"
#include <cairo.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <iostream>
#include <utility>


//TODO fix key combination processing
//TODO explore a bit to find bugs
guint timeoutRef;

PangoFontDescription *font;

bool drawCaret = true;

void initEditor() {

    //TODO change later
    E.startRow = 1;
    E.startColumn = 1;
    E.endRow = 1;
    E.endColumn = 1;
    E.n_rows = 0;
    E.displayRow = 1;
    E.displayColumn = 1;

    E.row = 1;
    E.column = 1;
    std::vector<unsigned long> v(1024, 0);
    E.n_words = v;
}

unsigned long calculatePosition(unsigned long row, unsigned long column) {
    // number of newlines
    unsigned long position = row - 1;
    for (int i = 1; i < row; i++)
        position += E.n_words[i];
    position += column - 1;
    return position;
}

static gboolean toggleCaret(gpointer data) {
    drawCaret = !drawCaret;
    g_signal_emit_by_name(data, "redraw", NULL);
    return TRUE;
}

void triggerAndResetRedraw(gpointer data) {
    drawCaret = true;
    g_signal_emit_by_name(data, "redraw", NULL);
    g_source_remove(timeoutRef);
    timeoutRef = g_timeout_add(1000, toggleCaret, (void *) data);
}


static void handleBackspace(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    int position = calculatePosition(E.row, E.column) - 1;
    if (position < 0)
        return;
    textBufferRemove(position, position);
    unsigned long new_row, new_column;
    if (E.column == 1) {
        new_row = std::max(E.row - 1, (unsigned long) 1);
        new_column = E.n_words[new_row] + 1;
    } else {
        new_row = E.row;
        new_column = E.column - 1;
    }
    editorMoveCursorToPosition(new_row, new_column);
}

static void handleArrowKeyPress(GtkWidget *widget, GdkEventKey *event, gpointer data) {

    unsigned long new_row, new_col;
    if (event->keyval == GDK_KEY_Right) {
        new_row = E.row;
        new_col =
                std::min(E.column + 1, E.n_words[new_row] + 1);
    } else if (event->keyval == GDK_KEY_Left) {
        new_row = E.row;
        new_col = std::max(E.column - 1, (unsigned long) 1);
    } else if (event->keyval == GDK_KEY_Up) {
        new_row = std::max(E.row - 1, (unsigned long) 1);
        new_col = std::min(E.column, E.n_words[new_row] + 1);
    } else if (event->keyval == GDK_KEY_Down) {
        new_row = std::min(E.row + 1, E.n_rows);
        new_col = std::min(E.column, E.n_words[new_row] + 1);
    }

    //check if display range needs to change
    unsigned long diff;
    if (new_row < E.startRow) {
        diff = E.startRow - new_row;
        E.startRow -= diff;
        E.endRow -= diff;
    } else if (new_row > E.endRow) {
        diff = new_row - E.endRow;
        E.startRow += diff;
        E.endRow += diff;
    }

    if (new_col < E.startColumn) {
        diff = E.startColumn - new_col;
        E.startColumn -= diff;
        E.endColumn -= diff;
    } else if (new_col > E.endColumn) {
        diff = new_col - E.endColumn;
        E.startColumn += diff;
        E.endColumn += diff;
    }
    editorMoveCursorToPosition(new_row, new_col);
}

static void handleReturn(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    unsigned long position = calculatePosition(E.row, E.column);
    char newLine = '\n';
    textBufferInsert(&newLine, 1, position);
    editorMoveCursorToPosition(E.row + 1, 1);

    if (E.row > E.endRow) {
        E.startRow++;
        E.endRow++;
    }
}

static void handleTab(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    int position = calculatePosition(E.row, E.column);
    textBufferInsert("    ", 4, position);
    E.column += 4;

    if (E.column > E.endColumn) {
        auto diff = E.column - E.endColumn;
        E.startColumn += diff;
        E.endColumn += diff;
    }
}

static void handleTextCharacter(GtkWidget *widget, GdkEventKey *event, gpointer data) {

    auto x = gdk_keyval_to_unicode(event -> keyval);
    if (!x || iscntrl(x))  {
        return;
     }


    int position = calculatePosition(E.row, E.column);
    textBufferInsert(std::string(1, (char) event->keyval), 1, position);
    E.column++;
    if (E.column > E.endColumn) {
        E.startColumn++;
        E.endColumn++;
    }
}

static gboolean keyPressCallback(GtkWidget *widget, GdkEventKey *event, gpointer data) {

    if (event->keyval == GDK_KEY_Right || event->keyval == GDK_KEY_Left || event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down) {
        //flush interBuffer before executing
        textBufferFlushInterBuffer();
        handleArrowKeyPress(widget, event, data);
    } else if (event->keyval == GDK_KEY_BackSpace) {
        textBufferFlushInterBuffer();
        handleBackspace(widget, event, data);
    } else if (event->keyval == GDK_KEY_Tab) {
        handleTab(widget, event, data);
    } else if (event->keyval == GDK_KEY_Return) {
        handleReturn(widget, event, data);
    } else {
        handleTextCharacter(widget, event, data);
    }
    triggerAndResetRedraw(data);
    return TRUE;
}

static void doDrawText() {
    displayText(0, UINT32_MAX);
}

static void doDrawCaret(cairo_t *cr, PangoLayout *layout) {

    std::pair<unsigned long, unsigned long> cursorPosition = getCaretCursorPixelPosition();
    pango_layout_set_font_description(layout, font);
    cairo_move_to(cr, cursorPosition.first, cursorPosition.second);
    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 1.0);
    pango_layout_set_text(layout, "|", 1);
    pango_cairo_show_layout(cr, layout);
}


static void doClearScreen() {
    //    cairo_move_to(cr, 0, 0);
    cairo_set_source_rgba(cr, backgroundColor.red, backgroundColor.green, backgroundColor.blue, 1.0);
    //get screen size instead of these numbers
    cairo_rectangle(cr, 0.0, 0.0, LAYOUT_WIDTH, LAYOUT_HEIGHT);
    cairo_fill(cr);
}

static gboolean onRedraw(GtkWidget *widget, GdkEventKey *event, gpointer data) {

    //    GtkWidget *widget = (GtkWidget *) data;

    window = gtk_widget_get_window(widget);

    context = gdk_pango_context_get();
    layout = pango_layout_new(context);
    pango_layout_set_font_description(layout, font);
    pango_layout_set_indent(layout, 1000);

    //clear screen
    cairoRegion = cairo_region_create();
    drawingContext = gdk_window_begin_draw_frame(window, cairoRegion);
    cr = gdk_drawing_context_get_cairo_context(drawingContext);
    doClearScreen();


    gdk_window_end_draw_frame(window, drawingContext);
    cairo_region_destroy(cairoRegion);

    //draw text
    doDrawText();

    //draw caret
    if (drawCaret) {
        cairoRegion = cairo_region_create();
        drawingContext = gdk_window_begin_draw_frame(window, cairoRegion);
        cr = gdk_drawing_context_get_cairo_context(drawingContext);
        doDrawCaret(cr, layout);
        gdk_window_end_draw_frame(window, drawingContext);
        cairo_region_destroy(cairoRegion);
    }

    return TRUE;
}


static void
onSizeAllocate(GtkWidget *widget, GtkAllocation *allocation) {
    int new_width, new_height;

    gtk_window_get_size(GTK_WINDOW(widget), &LAYOUT_WIDTH, &LAYOUT_HEIGHT);
    E.endRow = E.startRow + (LAYOUT_HEIGHT / APPROX_CHAR_HEIGHT) - 1;
    E.endColumn = E.startColumn + (LAYOUT_WIDTH / APPROX_CHAR_WIDTH) - 1;
}

static void
activate(GtkApplication *app,
         gpointer user_data) {

    GtkWidget *window;

    //CSS
    GtkCssProvider *provider;
    GdkDisplay *display;
    GdkScreen *screen;

    GtkDrawingArea *drawingArea;

    //editor init
    initEditor();

    //window
    window = gtk_application_window_new(app);
    g_signal_connect((GtkWidget *) window, "size_allocate", G_CALLBACK(onSizeAllocate), NULL);


    gtk_window_maximize(GTK_WINDOW(window));
    gtk_window_set_title(GTK_WINDOW(window), "DBedit");
    gtk_widget_set_name(window, "mainWindow");


    //CSS connection
    provider = gtk_css_provider_new();
    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);


    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    const gchar *cssFile = "../appStyle.css";
    GError *error = nullptr;
    gtk_css_provider_load_from_file(provider, g_file_new_for_path(cssFile), &error);
    g_object_unref(provider);


    //color fields
    gdk_rgba_parse(&backgroundColor, "rgb(0,0,50)");
    gdk_rgba_parse(&textColor, "rgb(255,255,255)");
    gdk_rgba_parse(&caretColor, "rgb(255,255,255)");
    //font
    font = pango_font_description_from_string("Courier 13");


    //drawing area
    drawingArea = (GtkDrawingArea *) gtk_drawing_area_new();
    //    GtkStyleContext *styleContext = gtk_widget_get_style_context((GtkWidget *) drawingArea);
    //    gtk_style_context_get_background_color(styleContext, GTK_STATE_FLAG_NORMAL, &backgroundColor);

    gtk_container_add(GTK_CONTAINER(window), (GtkWidget *) drawingArea);

    //    gtk_widget_get_size_request((GtkWidget *) drawingArea, &LAYOUT_WIDTH, &LAYOUT_HEIGHT);


    //    gtk_window_get_size((GtkWindow *) window, &LAYOUT_WIDTH, &LAYOUT_HEIGHT);

    //definitions
    PangoRectangle logical_rect;
    //    PangoLayout *layout;

    //get char width, height
    layout = gtk_widget_create_pango_layout((GtkWidget *) drawingArea, "W");
    pango_layout_set_font_description(layout, font);
    pango_layout_get_pixel_extents(layout, NULL, &logical_rect);


    //    pango_layout_get_pixel_size(layout, &LAYOUT_WIDTH, &LAYOUT_HEIGHT);


    APPROX_CHAR_HEIGHT = logical_rect.height;
    APPROX_CHAR_WIDTH = logical_rect.width;

    //add timeout
    gtk_widget_add_events(window, GDK_KEY_PRESS_MASK);

    timeoutRef = g_timeout_add(1000, toggleCaret, (void *) drawingArea);
    g_signal_new("redraw", G_TYPE_OBJECT, G_SIGNAL_RUN_FIRST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER, G_TYPE_NONE, 1, G_TYPE_POINTER);
    g_signal_connect(G_OBJECT(window), "key_press_event",
                     G_CALLBACK(keyPressCallback), (void *) drawingArea);
    g_signal_connect((GtkWidget *) drawingArea, "redraw", G_CALLBACK(onRedraw), NULL);

    gtk_widget_show_all((GtkWidget *) window);
    GtkAllocation allocation;
    gtk_widget_get_allocation((GtkWidget *) window, &allocation);

}


static void openFile(GtkApplication *app, GApplicationCommandLine *cmdLine) {

    int argc;
    gchar **args = g_application_command_line_get_arguments(cmdLine, &argc);


    if (argc > 1) {
        char *fileNameC = args[1];

        std::string fileName(fileNameC);
        readFile(fileName);
    }

    g_application_activate(reinterpret_cast<GApplication *>(app));
}

int main(int argc,
         char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("org.darko.dbedit", G_APPLICATION_HANDLES_COMMAND_LINE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(app, "command-line", G_CALLBACK(openFile), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
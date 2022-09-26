#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>

char *png_data = NULL;
char *image_data = NULL;
uint64_t image_width = 0;
uint64_t image_height = 0;

typedef struct bl_cairo_png_closure {
    unsigned char const* data;
    unsigned int read;
} bl_cairo_png_closure;

static void print_hello(GtkWidget *widget, gpointer data)
{
    g_print("Hello, world!\n");
}

static cairo_status_t cairo_read_png_func(void *closure,
        unsigned char *data, unsigned int length)
{
    bl_cairo_png_closure *c = (bl_cairo_png_closure*)closure;

    for (unsigned int i = 0; i < length; ++i) {
        data[i] = c->data[i + c->read];
    }
    c->read += length;

    return CAIRO_STATUS_SUCCESS;
}

static void load_image()
{
    FILE *f;
    int size;

    // Load PNG image.
    f = fopen("miku.png", "rb");
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    png_data = (char*)malloc(sizeof(uint8_t) * size);
    fread(png_data, 1, size, f);

    fclose(f);

    // Get image data.
    bl_cairo_png_closure closure = {
        png_data,
        0,
    };
    // Create Cairo surface.
    cairo_surface_t *cairo_surface =
        cairo_image_surface_create_from_png_stream(
            cairo_read_png_func,
            &closure
        );
    // Create Cairo.
    cairo_t *cr = cairo_create(cairo_surface);
    if (cairo_status(cr) != CAIRO_STATUS_SUCCESS) {
        fprintf(stderr, "cairo_create failed!\n");
        return;
    }
    image_width = cairo_image_surface_get_width(cairo_surface);
    image_height = cairo_image_surface_get_height(cairo_surface);
    fprintf(stderr, "Image size: %dx%d\n", image_width, image_height);

    cairo_format_t cairo_format = cairo_image_surface_get_format(cairo_surface);
    if (cairo_format == CAIRO_FORMAT_ARGB32) {
        fprintf(stderr, "cairo format is ARGB32\n");
    }

    // Set data.
    uint64_t data_size = sizeof(uint32_t) * (image_width * image_height);
    image_data = (char*)malloc(sizeof(uint8_t) * data_size);
    memcpy(
        image_data,
        cairo_image_surface_get_data(cairo_surface),
        data_size
    );

    // Free the Cairo resources.
    cairo_surface_destroy(cairo_surface);
    cairo_destroy(cr);
}

static void draw_function(GtkDrawingArea *area,
        cairo_t *cr,
        int width,
        int height,
        gpointer data)
{
    GdkRGBA color;
    int stride;

    stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32,
        image_width);
    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        image_data, CAIRO_FORMAT_ARGB32,
        image_width, image_height, stride
    );
    cairo_status_t status = cairo_surface_status(surface);
    if (status == CAIRO_STATUS_INVALID_STRIDE) {
        fprintf(stderr, "Invalid stride!\n");
    }

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    cairo_surface_destroy(surface);
}

static void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window;
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *header_bar;
    GtkWidget *drawing_area;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Window");
    gtk_window_set_default_size(GTK_WINDOW(window), 200, 200);

    header_bar = gtk_header_bar_new();

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

    gtk_window_set_child(GTK_WINDOW(window), box);

    button = gtk_button_new_with_label("Hello world");

    g_signal_connect(button, "clicked", G_CALLBACK(print_hello), NULL);
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_destroy),
        window);

    gtk_box_append(GTK_BOX(box), button);

    gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);
    // gtk_window_set_decorated(GTK_WINDOW(window), 0);

    // Image.
    drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area), 128);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area), 128);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area),
        draw_function,
        NULL, NULL);

    gtk_box_append(GTK_BOX(box), drawing_area);

    // CSS.
    GtkStyleContext *context;
    context = gtk_widget_get_style_context(window);
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "window.background.csd { border: 1px solid cyan; border-radius: 10px; box-shadow: 0px 0px 40px grey; }",
        -1
    );
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);

    gtk_widget_show(window);
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    int status;

    // Load image.
    load_image();

    app = gtk_application_new("io.orbitrc.hello", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}

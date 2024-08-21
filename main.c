#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define FPS 30.0
#define FRAME_TIME 1.0 / FPS
#define FRAME_TIME_US FRAME_TIME * 1e+6

static void panic(const char *message) {
  fprintf(stderr, "%s\n", message);
  exit(EXIT_FAILURE);
}

void draw(cairo_t *cairo, cairo_surface_t *image) {
  cairo_set_source_surface(cairo, image, 0, 0);
  cairo_paint(cairo);
}

static void ignore_mouse_input(Display *display, Window window) {
  XRectangle rect;
  XserverRegion region = XFixesCreateRegion(display, &rect, 1);
  XFixesSetWindowShapeRegion(display, window, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(display, region);
}

int main() {
  Display *display = XOpenDisplay(NULL);
  if (display == NULL) {
    panic("Could not open display");
  }
  Window root = DefaultRootWindow(display);
  XWindowAttributes attributes;
  XGetWindowAttributes(display, root, &attributes);
  XVisualInfo visual;
  if (!XMatchVisualInfo(display, DefaultScreen(display), 32, TrueColor,
                        &visual)) {
    panic("No visual found supporting 32 bit color, terminating");
  }
  cairo_surface_t *image = cairo_image_surface_create_from_png("dvd.png");
  XSetWindowAttributes attrs = {
      .override_redirect = true,
      .colormap = XCreateColormap(display, root, visual.visual, AllocNone),
  };
  Window window = XCreateWindow(
      display, root, 0, 0, attributes.width, attributes.height, 0, visual.depth,
      InputOutput, visual.visual,
      CWOverrideRedirect | CWColormap | CWBackPixel | CWBorderPixel, &attrs);

  ignore_mouse_input(display, window);
  XMapWindow(display, window);

  cairo_surface_t *surface = cairo_xlib_surface_create(
      display, window, visual.visual, attributes.width, attributes.height);
  cairo_t *cairo = cairo_create(surface);
  while (true) {
    draw(cairo, image);
    XFlush(display);
    usleep(FRAME_TIME_US);
  }

  cairo_surface_destroy(image);
  cairo_destroy(cairo);
  cairo_surface_destroy(surface);

  XUnmapWindow(display, window);
  XCloseDisplay(display);
  return 0;
}

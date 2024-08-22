#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DX 7
#define DY DX
#define FPS 30.0
#define FRAME_TIME 1.0 / FPS
#define FRAME_TIME_US FRAME_TIME * 1e+6

static void panic(const char *message) {
  fprintf(stderr, "%s\n", message);
  exit(EXIT_FAILURE);
}

typedef struct {
  cairo_surface_t *surface;
  int width;
  int height;
} image;

typedef struct {
  int x;
  int y;
  int dx;
  int dy;
} State;

// TODO: initialize with the logo in random position

void draw(cairo_t *cairo, State *state, image *image, int screen_width,
          int screen_height) {
  cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cairo);
  int new_x = state->x + state->dx;
  int new_y = state->y + state->dy;
  if (new_x < 0 || (new_x + image->width) > screen_width) {
    state->dx *= -1;
  } else {
    state->x = new_x;
  }
  if (new_y < 0 || new_y + image->height > screen_height) {
    state->dy *= -1;
  } else {
    state->y = new_y;
  }
  cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_surface(cairo, image->surface, state->x, state->y);
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
  image image = {
      .surface = cairo_image_surface_create_from_png("dvd.png"),
  };
  image.width = cairo_image_surface_get_width(image.surface);
  image.height = cairo_image_surface_get_height(image.surface);
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
  State state = {
      .x = 0,
      .y = 0,
      .dx = DX,
      .dy = DY,
  };

  while (true) {
    draw(cairo, &state, &image, attributes.width, attributes.height);
    XFlush(display);
    usleep(FRAME_TIME_US);
  }

  cairo_surface_destroy(image.surface);
  cairo_destroy(cairo);
  cairo_surface_destroy(surface);

  XUnmapWindow(display, window);
  XCloseDisplay(display);
  return 0;
}

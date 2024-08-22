#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#include <cairo/cairo-xlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define DX 7
#define DY DX
#define FPS 30.0
#define FADE_IN_ALPHA_TIME 1
#define FADE_IN_ALPHA_DIFF_PER_FRAME 1 / (FADE_IN_ALPHA_TIME * FPS)
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
} Image;

typedef struct {
  double r, g, b, a;
} Color;

typedef struct {
  int x;
  int y;
  int dx;
  int dy;
  Color color;
} State;

static bool x_is_within_screen_bounds(int x, int width, int screen_width) {
  return 0 <= x && (x + width) <= screen_width;
}

static bool y_is_within_screen_bounds(int y, int height, int screen_height) {
  return 0 <= y && y + height <= screen_height;
}

static void ignore_mouse_input(Display *display, Window window) {
  XRectangle rect;
  XserverRegion region = XFixesCreateRegion(display, &rect, 1);
  XFixesSetWindowShapeRegion(display, window, ShapeInput, 0, 0, region);
  XFixesDestroyRegion(display, region);
}

static double rand_double() { return rand() / (double)RAND_MAX; }

static int rand_int(int max) { return max * rand_double(); }

static State rand_state(Image *image, int screen_width, int screen_height) {
  while (true) {
    int x = rand_int(screen_width);
    int y = rand_int(screen_height);
    if (x_is_within_screen_bounds(x, image->width, screen_width) &&
        y_is_within_screen_bounds(y, image->height, screen_height)) {
      return (State){
          .x = x,
          .y = y,
          .dx = DX,
          .dy = DY,
          .color =
              {
                  .r = 1.0,
                  .g = 1.0,
                  .b = 1.0,
                  .a = 0,
              },
      };
    }
  }
}

static void set_random_color_as_source(State *state) {
  state->color = (Color){
      .r = rand_double(),
      .g = rand_double(),
      .b = rand_double(),
      .a = state->color.a,
  };
}

void draw(cairo_t *cairo, State *state, Image *image, int screen_width,
          int screen_height) {
  state->color.a += FADE_IN_ALPHA_DIFF_PER_FRAME;
  if (state->color.a > 1) {
    state->color.a = 1;
  }

  int new_x = state->x + state->dx;
  int new_y = state->y + state->dy;
  if (x_is_within_screen_bounds(new_x, image->width, screen_width)) {
    state->x = new_x;
  } else {
    set_random_color_as_source(state);
    state->dx *= -1;
  }
  if (y_is_within_screen_bounds(new_y, image->height, screen_height)) {
    state->y = new_y;
  } else {
    set_random_color_as_source(state);
    state->dy *= -1;
  }
  cairo_set_source_rgba(cairo, state->color.r, state->color.g, state->color.b,
                        state->color.a);
  cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cairo);
  cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
  cairo_mask_surface(cairo, image->surface, state->x, state->y);
}

int main() {
  srand(time(NULL));
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
  Image image = {
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
  State state = rand_state(&image, attributes.width, attributes.height);

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

#ifndef FB_H
#define FB_H

#include <linux/fb.h>

#define FB_ERRLEN 1024

/* 2D math */

typedef struct xy {
  long x;
  long y;
} s_xy;

s_xy xy (long x, long y);
s_xy xy_sub (s_xy a, s_xy b);
s_xy xy_add (s_xy a, s_xy b);

/* colors */

typedef struct rgb {
  unsigned char b;
  unsigned char g;
  unsigned char r;
} s_rgb;

typedef struct rgba {
  unsigned char b;
  unsigned char g;
  unsigned char r;
  unsigned char a;
} s_rgba;

s_rgb rgb (unsigned char r, unsigned char g, unsigned char b);
s_rgba rgba (unsigned char r, unsigned char g, unsigned char b,
             unsigned char a);

/* keyboard input */

int kb_poll (void);

#define KB_EVENT_MAX 8

typedef void (*kb_fun) (void);

typedef struct kb_event {
  char in[KB_EVENT_MAX];
  kb_fun fun;
} s_kb_event;

typedef struct kb_handler {
  char ring[KB_EVENT_MAX];
  unsigned char in;
  unsigned char out;
  unsigned char length;
} s_kb_handler;

void kb_in (unsigned char k);
s_kb_event* kb_handler ();

/* framebuffer */

struct fb {
  s_xy pos;
  s_rgb color;
  s_rgb clear_color;
  int fd;
  unsigned long line;
  unsigned long x;
  unsigned long y;
  unsigned long w;
  unsigned long h;
  unsigned long bpp;
  unsigned long gray;
  size_t size;
  void *buffer;
  void *front;
  void *back;
  char error[FB_ERRLEN];
  struct fb_var_screeninfo vinfo;
};

int fb_open (struct fb *fb, const char *path, void *back, unsigned long size);
int fb_close (struct fb *fb);

void * fb_buffer (struct fb *fb);
unsigned long fb_width (struct fb *fb);
unsigned long fb_height (struct fb *fb);
unsigned long fb_bits_per_pixel (struct fb *fb);

/* drawing */

void fb_color (struct fb *fb, s_rgb color);

void fb_clear (struct fb *fb);

void fb_move_to (struct fb *fb, s_xy pos);
void fb_line_to (struct fb *fb, s_xy pos);

/* framebuffer primitives */

unsigned long fb_pixel (struct fb *fb, int x, int y);
void fb_swap (struct fb *fb);

#endif /* FB_H */

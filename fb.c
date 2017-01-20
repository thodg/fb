#include <string.h>
#include <errno.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <math.h>
#include "fb.h"

#define FB_DEBUG 0

/* tty setup */

int g_tty_fd = -1;

void reset_tty (void)
{
  if (g_tty_fd > -1)
    ioctl(g_tty_fd, KDSETMODE, KD_TEXT);
  close(g_tty_fd);
  g_tty_fd = -1;
}

int open_tty ()
{
  if (atexit(reset_tty)) {
    fputs("atexit\n", stderr);
    return -1;
  }
  g_tty_fd = open("/dev/tty0", O_RDWR);
  ioctl(g_tty_fd, KDSETMODE, KD_GRAPHICS);
  return 0;
}

/* terminal setup */

struct termios g_termios;

static void write_out(const char *str)
{
  write(STDOUT_FILENO, str, strlen(str));
}

static void term_cleanup(void)
{
  tcsetattr(0, 0, &g_termios);
  write_out("\x1b[?25h\n");             /* show the cursor */
}

static int term_setup(void)
{
  struct termios rawtermios;
  if (atexit(term_cleanup)) {
    fputs("atexit\n", stderr);
    return -1;
  }
  tcgetattr(0, &g_termios);
  rawtermios = g_termios;
  cfmakeraw(&rawtermios);
  tcsetattr(0, TCSAFLUSH, &rawtermios);
  write_out("\x1b[?25l"     /* hide the cursor */
	    "\x1b[2J");    /* clear the screen */
  return 0;
}

/* keyboard input */

int kb_poll (void)
{
  fd_set readfd;
  int s;
  struct timeval timeout = {0, 0};
  FD_ZERO(&readfd);
  FD_SET(STDIN_FILENO, &readfd);
  s = select(STDIN_FILENO + 1, &readfd, NULL, NULL, &timeout);
  if (s == 1) {
    unsigned char b;
    if (read(0, &b, 1) <= 0)
      return -1;
    return b;
  }
  else if (s == 0)
    return 0;
  return -1;
}

extern s_kb_event g_kb[];

s_kb_handler g_kb_handler = {
  {0}, 0, 0, 0
};

void kb_in (unsigned char k)
{
  if (g_kb_handler.length == KB_EVENT_MAX) {
    g_kb_handler.in = (g_kb_handler.in + 1) % KB_EVENT_MAX;
    g_kb_handler.length--;
  }
  g_kb_handler.ring[g_kb_handler.out] = k;
  g_kb_handler.out = (g_kb_handler.out + 1) % KB_EVENT_MAX;
  g_kb_handler.length++;
}

int kb_event (s_kb_event *ke)
{
  int o = g_kb_handler.out;
  int p = 0;
  for (; o = (o - 1 + KB_EVENT_MAX) % KB_EVENT_MAX,
         ke->in[p] &&
         ke->in[p] == g_kb_handler.ring[o] &&
         (p++, o != g_kb_handler.in) ;)
    ;
  return (ke->in[p] == 0);
}

s_kb_event* kb_handler ()
{
  s_kb_event *ke;
  for (ke = g_kb; ke->fun; ke++) {
    if (kb_event(ke)) {
      g_kb_handler.in = 0;
      g_kb_handler.out = 0;
      g_kb_handler.length = 0;
      return ke;
    }
  }
  return 0;
}

/* math */

static inline size_t min (size_t a, size_t b)
{
  return (a < b) ? a : b;
}

static inline long abs_l (long x) {
  return (x < 0) ? -x : x;
}

/* 2D maths */

s_xy xy (long x, long y)
{
  return (s_xy) {.x = x, .y = y};
}

s_xy xy_sub (s_xy a, s_xy b)
{
  s_xy o;
  o.x = a.x - b.x;
  o.y = a.y - b.y;
  return o;
}

s_xy xy_add (s_xy a, s_xy b)
{
  s_xy o;
  o.x = a.x + b.x;
  o.y = a.y + b.y;
  return o;
}

/* colors */

s_rgb rgb (unsigned char r, unsigned char g, unsigned char b)
{
  return (s_rgb) {.r = r, .g = g, .b = b};
}

s_rgba rgba (unsigned char r, unsigned char g, unsigned char b,
             unsigned char a)
{
  return (s_rgba) {.r = r, .g = g, .b = b, .a = a};
}

/* framebuffer drawing */

void fb_move_to (struct fb *fb, s_xy pos)
{
  fb->pos = pos;
}

void fb_color (struct fb *fb, s_rgb color)
{
  fb->color = color;
}

long round_l (double d)
{
  if (d >= 0)
    return d + 0.5;
  return d - 0.5;
}

static inline void fb_line_y (struct fb *fb, s_xy d)
{
  long y;
  for (y = 0; y < d.y; y++) {
    long x = round_l(y * d.x / (double) d.y);
    s_rgb *p = fb->buffer + fb_pixel(fb, fb->pos.x + x, fb->pos.y + y);
    *p = fb->color;
  }
  fb->pos = xy_add(fb->pos, d);
}

static inline void fb_line_y_ (struct fb *fb, s_xy d)
{
  long y;
  for (y = 0; y > d.y; y--) {
    long x = round_l(y * d.x / (double) d.y);
    s_rgb *p = fb->buffer + fb_pixel(fb, fb->pos.x + x, fb->pos.y + y);
    *p = fb->color;
  }
  fb->pos = xy_add(fb->pos, d);
}

static inline void fb_line_x (struct fb *fb, s_xy d)
{
  long x;
  for (x = 0; x < d.x; x++) {
    long y = round_l(x * d.y / (double) d.x);
    s_rgb *p = fb->buffer + fb_pixel(fb, fb->pos.x + x, fb->pos.y + y);
    *p = fb->color;
  }
  fb->pos = xy_add(fb->pos, d);
}

static inline void fb_line_x_ (struct fb *fb, s_xy d)
{
  long x;
  for (x = 0; x > d.x; x--) {
    long y = round_l(x * d.y / (double) d.x);
    s_rgb *p = fb->buffer + fb_pixel(fb, fb->pos.x + x, fb->pos.y + y);
    *p = fb->color;
  }
  fb->pos = xy_add(fb->pos, d);
}

void fb_line_to (struct fb *fb, s_xy to)
{
  s_xy d = xy_sub(to, fb->pos);
  long x = abs(d.x);
  long y = abs(d.y);
  int px = 0 <= d.x;
  int py = 0 <= d.y;
  if (py && x <= y)
    fb_line_y(fb, d);
  else if (px && y <= x)
    fb_line_x(fb, d);
  else if (!py && x <= y)
    fb_line_y_(fb, d);
  else
    fb_line_x_(fb, d);
}

/* framebuffer primitives */

unsigned long fb_pixel (struct fb *fb, int x, int y)
{
  //return (fb->y + y) * fb->line + fb->x + x * (fb->bpp / 8);
  return y * fb->line + x * (fb->bpp / 8);
}

void fb_clear (struct fb *fb)
{
  s_rgb *b = fb->buffer;
  unsigned long p = 0;
  for (p = 0; p < fb->size; p += 4)
    *(b++) = fb->clear_color;
}

void fb_swap (struct fb *fb)
{
  s_rgb *buffer = fb->buffer;
  s_rgb *front = fb->front;
  if (buffer != front) {
    unsigned long p = 0;
    for (p = 0; p < fb->size; p += 3)
      *(front++) = *(buffer++);
  }
}

/* framebuffer setup */

int fb_err (struct fb *fb, const char *msg)
{
  int len = min(strlen(msg), FB_ERRLEN / 2);
  strncpy(fb->error, msg, len);
  strcpy(fb->error + len, ": ");
  strerror_r(errno, fb->error + len + 2, FB_ERRLEN - len - 2);
  fprintf(stderr, "fb: %s\n", fb->error);
  return -1;
}

void dump_finfo (struct fb_fix_screeninfo *finfo)
{
  printf("finfo.id %s\r\n", finfo->id);
  printf("finfo.smem_start %lu\r\n", finfo->smem_start);
  printf("finfo.smem_len %u\r\n", finfo->smem_len);
  printf("finfo.xpanstep %u\r\n", finfo->xpanstep);
  printf("finfo.ypanstep %u\r\n", finfo->ypanstep);
  printf("finfo.ywrapstep %u\r\n", finfo->ywrapstep);
  printf("finfo.line_length %u\r\n", finfo->line_length);
  printf("finfo.mmio_start %lu\r\n", finfo->mmio_start);
  printf("finfo.mmio_len %u\r\n", finfo->mmio_len);
  printf("\r\n");
}

void dump_vinfo (struct fb_var_screeninfo *vinfo)
{
  printf("vinfo.xres %u\r\n", vinfo->xres);
  printf("vinfo.yres %u\r\n", vinfo->yres);
  printf("vinfo.xres_virtual %u\r\n", vinfo->xres_virtual);
  printf("vinfo.yres_virtual %u\r\n", vinfo->yres_virtual);
  printf("vinfo.xoffset %u\r\n", vinfo->xoffset);
  printf("vinfo.yoffset %u\r\n", vinfo->yoffset);
  printf("vinfo.bits_per_pixel %u\r\n", vinfo->bits_per_pixel);
  printf("vinfo.grayscale %u\r\n", vinfo->grayscale);
  printf("\r\n");
}

int fb_open (struct fb *fb, const char *path, void *back, unsigned long size)
{
  struct fb_fix_screeninfo finfo;
  bzero(fb, sizeof(*fb));
  if (term_setup() == -1) {
    return fb_err(fb, "term_setup");
  }
  fb->fd = open(path, O_RDWR);
  if (fb->fd == -1)
    return fb_err(fb, path);
  if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &fb->vinfo) == -1)
    return fb_err(fb, "FBIOGET_VSCREENINFO");
  if (FB_DEBUG)
    dump_vinfo(&fb->vinfo);
  if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo) == -1)
    return fb_err(fb, "FBIOGET_FSCREENINFO");
  if (FB_DEBUG)
    dump_finfo(&finfo);
  fb->line = finfo.line_length;
  fb->w = fb->vinfo.xres;
  fb->h = fb->vinfo.yres;
  fb->bpp = fb->vinfo.bits_per_pixel;
  fb->size = fb->h * fb->line;
  if (back && size < fb->size)
    return fb_err(fb, "low back buffer size");
  fb->back = back;
  fb->buffer = back;
  fb->front = mmap(0, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
  if ((int) fb->front == -1)
    return fb_err(fb, "mmap");
  if (!fb->buffer)
    fb->buffer = fb->front;
  fb->color = rgb(255, 255, 255);
  fb->clear_color = rgb(0, 0, 0);
  return 0;
}

int fb_close (struct fb *fb)
{
  fb->error[0] = 0;
  if (munmap(fb->front, fb->size) == -1)
    fb_err(fb, "munmap");
  if (close(fb->fd) == -1)
    fb_err(fb, "close");
  return 0;
}

void * fb_buffer (struct fb *fb)
{
  return fb->buffer;
}

unsigned long fb_width (struct fb *fb)
{
  return fb->w;
}

unsigned long fb_height (struct fb *fb)
{
  return fb->h;
}

unsigned long fb_bits_per_pixel (struct fb *fb)
{
  return fb->bpp;
}

#include <stdio.h>
#include <string.h>
#include "fb.h"

int g_loop = 1;

void quit () {
  g_loop = 0;
}

s_kb_event g_kb[] = {
  {"\x3", quit},
  {"q", quit},
  {"", 0}
};

static void kb_events () {
  int k;
  while ((k = kb_poll())) {
    s_kb_event *e;
    kb_in(k);
    e = kb_handler();
    if (e)
      e->fun();
  }
}

#if 0
static void draw_12 (struct fb *fb)
{
  fb_move_to(fb, (s_xy) {14, 28});
  fb_line_to(fb, (s_xy) {28, 14});
  fb_line_to(fb, (s_xy) {42, 14});
  fb_line_to(fb, (s_xy) {42, 42});
  fb_line_to(fb, (s_xy) {14, 42});
  fb_move_to(fb, (s_xy) {56, 28});
  fb_line_to(fb, (s_xy) {70, 14});
  fb_line_to(fb, (s_xy) {84, 14});
  fb_line_to(fb, (s_xy) {56, 42});
  fb_line_to(fb, (s_xy) {84, 42});
}
#endif

typedef long s_t;
typedef unsigned long u_t;

#define B_ 14
#define A_ (1 << B_)
static const s_t B = B_;
static const s_t A = A_;
#define U / A_

#define Y_ (32 - B_)
#define Z_ (1 << Y_)
static const s_t Y = Y_;
static const s_t Z = Z_;

s_t s_abs (s_t a)
{
  if (a < 0)
    return -a;
  return a;
}

s_t s_square (s_t a)
{
  return a * a;
}

s_t s_squares (s_t a, s_t b)
{
  return a * a + b * b;
}

#define TANGENT_BITS B_
#define TANGENT_MAX (1 << TANGENT_BITS)

/*
s_t g_tan[TANGENT_MAX];

static void tan_table ()
{
  unsigned char b;
  unsigned long t;
  for (b = 0; b < TANGENT_BITS; b++) {
    unsigned long c = 1 << (b - 1);
    unsigned long d;
    for (d = 0; d < c; d++) {
      unsigned e = ((d << (TANGENT_BITS - b)) |
                    (1 << (TANGENT_BITS - b - 1)));
      g_tan[t++] = e;
    }
  }
}

u_t s_tan (s_t s)
{
  (void) s;
  return 0;
}
*/

#define SIN_MAX (2 * TANGENT_MAX)

s_t g_sin[SIN_MAX];

static void sin_table ()
{
  unsigned int l;
  s_t d = 0;
  s_t min = 0;
  for (l = 0; l <= (unsigned) A / 2; l++) {
    s_t c;
    s_t max = A;
    g_sin[l] = min;
    for (; c = s_squares(g_sin[l], A - l) - A * A,
           c != 0 &&
           c != d ;) {
      d = c;
      if (c < 0) {
        min = g_sin[l];
        g_sin[l] = (min + max + 1) / 2;
      }
      else {
        max = g_sin[l];
        g_sin[l] = (min + max + 1) / 2;
      }
    }
    if (l > 0)
      g_sin[A - l] = A - g_sin[l];
  }
}

s_t s_sin (s_t a)
{
  for (; 4 * A <= a; a -= 4 * A);
  for (; a < 0; a += 4 * A);
  if (a < A)
    return g_sin[a];
  if (a < 2 * A)
    return g_sin[2 * A - 1 - a];
  if (a < 3 * A)
    return -g_sin[a - 2 * A];
  return g_sin[4 * A - 1 - a];
}

s_t s_cos (s_t a)
{
  for (; 4 * A <= a; a -= 4 * A);
  for (; a < 0; a += 4 * A);
  if (a < A)
    return g_sin[A - 1 - a];
  if (a < 2 * A)
    return -g_sin[a - A];
  if (a < 3 * A)
    return -g_sin[3 * A - 1 - a];
  return g_sin[a - 3 * A];
}

static void draw_ortho10_w (struct fb *fb)
{
  unsigned w = fb->w;
  unsigned h = fb->h;
  unsigned w2 = fb->w / 2;
  unsigned h2 = fb->h / 2;
  unsigned i = 10;
  fb_move_to(fb, (s_xy) {0, h2});
  fb_line_to(fb, (s_xy) {w, h2});
  while (i < h / 2) {
    fb_move_to(fb, (s_xy) {w2 - 5, h2 - i});
    fb_line_to(fb, (s_xy) {w2 + 5, h2 - i});
    fb_move_to(fb, (s_xy) {w2 - 5, h2 + i});
    fb_line_to(fb, (s_xy) {w2 + 5, h2 + i});
    i = i + 10;
  }
}

static void draw_ortho10_h (struct fb *fb)
{
  unsigned w = fb->w;
  unsigned h = fb->h;
  unsigned w2 = fb->w / 2;
  unsigned h2 = fb->h / 2;
  unsigned i = 10;
  fb_move_to(fb, (s_xy) {w2, 0});
  fb_line_to(fb, (s_xy) {w2, h});
  while (i < w / 2) {
    fb_move_to(fb, (s_xy) {w2 - i, h2 - 5});
    fb_line_to(fb, (s_xy) {w2 - i, h2 + 5});
    fb_move_to(fb, (s_xy) {w2 + i, h2 - 5});
    fb_line_to(fb, (s_xy) {w2 + i, h2 + 5});
    i = i + 10;
  }
}

static void draw_ortho10 (struct fb *fb)
{
  fb_color(fb, (s_rgb) {255, 0, 0});
  draw_ortho10_w(fb);
  fb_color(fb, (s_rgb) {0, 255, 0});
  draw_ortho10_h(fb);
}

/*
typedef struct xyz
{
  s_t x;
  s_t y;
  s_t z;
} s_xyz;

long xyz_dot_product (s_xyz a, s_xyz b)
{
  return a.x * b.x U + a.y * b.y U + a.z * b.z U;
}

s_xyz xyz (s_t x, s_t y, s_t z)
{
  return (s_xyz) { x, y, z };
}

s_xyz xyz_s (s_xyz a, s_t b)
{
  return xyz(a.x * b U, a.y * b U, a.z * b U);
}

s_xyz xyz_add (s_xyz a, s_xyz b)
{
  return xyz(a.x + b.x, a.y + b.y, a.z + b.z);
}

s_xyz xyz_sub (s_xyz a, s_xyz b)
{
  return xyz(a.x - b.x, a.y - b.y, a.z - b.z);
}

s_t xyz_along (s_xyz a, s_xyz b)
{
  if (b.x > b.y && b.x > b.z)
    return a.x * A / b.x;
  if (b.y > b.x && b.y > b.z)
    return a.y * A / b.y;
  return a.z * A / b.z;
}

typedef struct m43
{
  s_t n[12];
} s_m43;

void m43_identity (s_m43 *m)
{
  m->n[0] = A; m->n[1] = 0; m->n[ 2] = 0; m->n[ 3] = 0;
  m->n[4] = 0; m->n[5] = A; m->n[ 6] = 0; m->n[ 7] = 0;
  m->n[8] = 0; m->n[9] = 0; m->n[10] = A; m->n[11] = 0;
}

void m43_translate (s_m43 *m, s_xyz d)
{
  m->n[ 3] += d.x;
  m->n[ 7] += d.y;
  m->n[11] += d.z;
}

void m43_scale (s_m43 *m, s_xyz s)
{
  m->n[ 0] = m->n[ 0] * s.x U;
  m->n[ 5] = m->n[ 5] * s.y U;
  m->n[10] = m->n[10] * s.z U;
}

void m43_rotate (s_m43 *m, s_xyz a, s_t b)
{
  (void) a;
  m->n[0] = m->n[0] * s_cos(b) U; m->n[1] = m->n[5] * -s_sin(b) U;
  m->n[4] = m->n[0] * s_sin(b) U; m->n[5] = m->n[5] *  s_cos(b) U;
}

s_xyz m43_xyz (s_m43 *m, s_xyz a)
{
  s_t *n = m->n;
  return xyz(n[0] * a.x U + n[1] * a.y U + n[ 2] * a.z U + n[ 3],
             n[4] * a.x U + n[5] * a.y U + n[ 6] * a.z U + n[ 7],
             n[8] * a.x U + n[9] * a.y U + n[10] * a.z U + n[11]);
}

s_m43 *g_model;

s_xyz model (s_xyz a)
{
  return m43_xyz(g_model, a);
}

s_xyz model_xyz (s_t x, s_t y, s_t z)
{
  return model(xyz(x, y, z));
}

typedef struct ray {
  s_xyz o;
  s_xyz d;
} s_ray;

s_xyz ray_point (s_ray *r, s_t l)
{
  return xyz_add(r->o, xyz_s(r->d, l));
}

typedef struct checker {
  s_xyz o;
  s_xyz u;
  s_xyz v;
  s_xyz n;
} s_checker;

s_xyz checker_point (s_checker *c, long u, long v)
{
  return xyz_add(c->o, xyz_add(xyz_s(c->u, u),
                               xyz_s(c->v, v)));
}

s_rgba checker_color (s_checker *c, s_xyz i)
{
  s_xyz ci = xyz_sub(i, c->o);
  long u = xyz_along(ci, c->u) U;
  long v = xyz_along(ci, c->v) U;
  if ((u + v) % 2)
    return (s_rgba) {255, 255, 255, 255};
  return (s_rgba) {80, 80, 80, 255};
}
*/
/*
  (rox + l rdx - cox) * nx + (roy + l rdy - coy) ny + (roz + l rdz - coz) nz = 0;
  l rdx nx + (rox - cox) nx + l rdy ny + (roy - coy) ny + l rdz nz + (roz - coz) nz = 0;
  l (rdx nx + rdy ny + rdz nz) + (rox - cox) nx + (roy - coy) ny + (roz - coz) nz = 0;
  l = ((cox - rox) nx + (coy - roy) ny + (coz - roz) nz) / (rdx nx + rdy ny + rdz nz);
  l = (co - ro).n / rd.n;
  p = ro + (co - ro).n / rd.n * rd;
  rc = co - ro;
  p = ro + rc.n * rd / rd.n;
*/
/*
s_rgba ray_checker (s_ray *r, s_checker *c)
{
  s_t rdn = xyz_dot_product(r->d, c->n);
  if (rdn) {
    s_xyz rc = xyz_sub(c->o, r->o);
    s_t rcn = xyz_dot_product(rc, c->n);
    s_xyz i = { r->o.x + rcn * r->d.x / rdn,
                r->o.y + rcn * r->d.y / rdn,
                r->o.z + rcn * r->d.z / rdn };
    s_rgba ic = checker_color(c, i);
    return ic;
  }
  return (s_rgba) {32, 0, 0, 0};
}

long g_depth = 2;

s_ray fb_ray (struct fb *fb, unsigned long x, unsigned long y)
{
  s_t w2 = fb->w / 2 * A;
  s_t h2 = fb->h / 2 * A;
  s_t z = g_depth * fb->w * A;
  return (s_ray) {
    .o = { w2,
           h2,
           -z },
    .d = { x * A - w2,
           y * A - h2,
           z }
  };
}

static void checker_scene (struct fb *fb)
{
  static s_m43 checker_model;
  unsigned long x;
  unsigned long y;
  fb_clear(fb);
  g_model = &checker_model;
  m43_identity(g_model);
  m43_scale(g_model, xyz(3 * A, A, A));
  m43_rotate(g_model, xyz(0, 0, A), A / 3);
  {
    s_checker c = { .o = model_xyz(0, 0, 0),
                    .u = model_xyz(10 * A, 0, 0),
                    .v = model_xyz(0, 10 * A, 0),
                    .n = model_xyz(0, 0, 10 * A) };
    for (y = 0; y < fb->h; y++) {
      for (x = 0; x < fb->w; x++) {
        s_rgba *p = fb->buffer + fb_pixel(fb, x, y);
        s_ray r = fb_ray(fb, x, y);
        *p = ray_checker(&r, &c);
      }
    }
  }
}
*/

static void draw (struct fb *fb)
{
  //checker_scene(fb);
  fb_clear(fb);
  draw_ortho10(fb);
  fb_swap(fb);
}

#define BUFFER_SIZE (1400 * 1050 * 4)

int main ()
{
  struct fb fb;
  s_rgba back[BUFFER_SIZE / 4];
  sin_table();
  if (fb_open(&fb, "/dev/fb0", back, BUFFER_SIZE))
    return 1;
  while (g_loop) {
    draw(&fb);
    kb_events();
  }
  fb_close(&fb);
  return 0;
}

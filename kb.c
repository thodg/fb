#include <stdio.h>
#include <string.h>
#include <time.h>
#include "fb.h"

int g_loop = 1;

void quit () {
  g_loop = 0;
}

s_kb_event g_kb[] = {
  {"\x3", quit},
  {"", 0}
};

static void kb_events () {
  int k;
  while ((k = kb_poll())) {
    s_kb_event *e;
    unsigned long t = 
    printf("%lu %x\n", t, k);
    fflush(stdout);
    kb_in(k);
    e = kb_handler();
    if (e)
      e->fun();
  }
}

#define BUFFER_SIZE (1400 * 1050 * 4)

int main ()
{
  struct fb fb;
  s_rgba back[BUFFER_SIZE / 4];
  if (fb_open(&fb, "/dev/fb0", back, BUFFER_SIZE))
    return 1;
  while (g_loop) {
    kb_events();
  }
  fb_close(&fb);
  return 0;
}

#include "timer.h"

static LARGE_INTEGER s,e,t,f;

void timer_start() {
  QueryPerformanceFrequency(&f);
  QueryPerformanceCounter(&s);
}

float timer_stop() {
  QueryPerformanceCounter(&e);
  t.QuadPart = e.QuadPart - s.QuadPart;
  t.QuadPart *= 1000000;
  t.QuadPart /= f.QuadPart;
  return (float)t.QuadPart;
}

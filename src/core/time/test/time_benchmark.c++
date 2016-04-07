#include <sys/time.h>
#include <time.h>

#include <gtest/gtest.h>

void
rdtscl(unsigned long long *ll) {
  unsigned int lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  *ll = ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

unsigned long long
rdtscl(void) {
  unsigned int lo, hi;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}

TEST(X, time_get_time_of_day) {
  for (int i = 0; i < 500000; i++) {
    timeval when;
    gettimeofday(&when, NULL);
  }
}

TEST(X, time_rdtscl) {
  for (int i = 0; i < 500000; i++) {
    unsigned long long when;
    rdtscl(&when);
  }
}

TEST(X, time_monotomic_get_time) {
  for (int i = 0; i < 500000; i++) {
    struct timespec when;
    clock_gettime(CLOCK_MONOTONIC, &when);
  }
}

TEST(X, time_monotomic_coarse_get_time) {
  for (int i = 0; i < 500000; i++) {
    struct timespec when;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &when);
  }
}

TEST(X, time_process_get_time) {
  for (int i = 0; i < 500000; i++) {
    struct timespec when;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &when);
  }
}

TEST(X, time_thread_get_time) {
  for (int i = 0; i < 500000; i++) {
    struct timespec when;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &when);
  }
}

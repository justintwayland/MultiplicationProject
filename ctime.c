#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#define NANOS_PER_SECF 1000000000.0
#define USECS_PER_SEC 1000000

#if _POSIX_TIMERS > 0 && defined(_POSIX_MONOTONIC_CLOCK)
  // If we have it, use clock_gettime and CLOCK_MONOTONIC.

  #include <time.h>

  double monotonic_seconds() {
    struct timespec time;
    // Note: Make sure to link with -lrt to define clock_gettime.
    clock_gettime(CLOCK_MONOTONIC, &time);
    return ((double) time.tv_sec) + ((double) time.tv_nsec / (NANOS_PER_SECF));
  }

#elif defined(__APPLE__)
  // If we don't have CLOCK_MONOTONIC, we might be on a Mac. There we instead
  // use mach_absolute_time().

  #include <mach/mach_time.h>

  static mach_timebase_info_data_t info;
  static void __attribute__((constructor)) init_info() {
    mach_timebase_info(&info);
  }

  double monotonic_seconds() {
    uint64_t time = mach_absolute_time();
    double dtime = (double) time;
    dtime *= (double) info.numer;
    dtime /= (double) info.denom;
    return dtime / NANOS_PER_SECF;
  }

#elif defined(_MSC_VER)
  // On Windows, use QueryPerformanceCounter and QueryPerformanceFrequency.

  #include <windows.h>

  static double PCFreq = 0.0;

  // According to http://stackoverflow.com/q/1113409/447288, this will
  // make this function a constructor.
  // TODO(awreece) Actually attempt to compile on windows.
  static void __cdecl init_pcfreq();
  __declspec(allocate(".CRT$XCU")) void (__cdecl*init_pcfreq_)() = init_pcfreq;
  static void __cdecl init_pcfreq() {
    // Accoring to http://stackoverflow.com/a/1739265/447288, this will
    // properly initialize the QueryPerformanceCounter.
    LARGE_INTEGER li;
    int has_qpc = QueryPerformanceFrequency(&li);
    assert(has_qpc);

    PCFreq = ((double) li.QuadPart) / 1000.0;
  }

  double monotonic_seconds() {
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return ((double) li.QuadPart) / PCFreq;
  }

#else
  // Fall back to rdtsc. The reason we don't use clock() is this scary message
  // from the man page:
  //     "On several other implementations, the value returned by clock() also
  //      includes the times of any children whose status has been collected via
  //      wait(2) (or another wait-type call)."
  //
  // Also, clock() only has microsecond accuracy.
  //
  // This whitepaper offered excellent advice on how to use rdtscp for
  // profiling: http://download.intel.com/embedded/software/IA/324264.pdf
  //
  // Unfortunately, we can't follow its advice exactly with our semantics,
  // so we're just going to use rdtscp with cpuid.
  //
  // Note that rdtscp will only be available on new processors.

  #include <stdint.h>

  static inline uint64_t rdtsc() {
    uint32_t hi, lo;
    asm volatile("rdtscp\n"
                 "movl %%edx, %0\n"
                 "movl %%eax, %1\n"
                 "cpuid"
                 : "=r" (hi), "=r" (lo) : : "%rax", "%rbx", "%rcx", "%rdx");
    return (((uint64_t)hi) << 32) | (uint64_t)lo;
  }

  static uint64_t rdtsc_per_sec = 0;
  static void __attribute__((constructor)) init_rdtsc_per_sec() {
    uint64_t before, after;

    before = rdtsc();
    usleep(USECS_PER_SEC);
    after = rdtsc();

    rdtsc_per_sec = after - before;
  }

  double monotonic_seconds() {
    return (double) rdtsc() / (double) rdtsc_per_sec;
  }

#endif

int main(int argc, char** argv) {
  pid_t pid=fork();
  if (pid==0) {
    execv(argv[1], argv+1);
    switch(errno) {
    case E2BIG: fprintf(stderr, "argv was too big\n"); break;
    case EACCES: fprintf(stderr, "%s: Permission denied\n", argv[1]); break;
    case EFAULT: fprintf(stderr, "%s is outside your accessible address space\n", argv[1]); break;
    case EINVAL: fprintf(stderr, "%s tried to use more than one interpreter\n", argv[1]); break;
    case EIO: fprintf(stderr, "%s: An IO error occurred\n", argv[1]); break;
    case EISDIR: fprintf(stderr, "%s: Interpreter named is directory\n", argv[1]); break;
    case ELOOP: fprintf(stderr, "%s: Too many symlinks were encountered in resolving a path.\n", argv[1]); break;
    case EMFILE: fprintf(stderr, "%s: Process has maximum number of files open.\n", argv[1]); break;
    case ENAMETOOLONG: fprintf(stderr, "%s: Filename too long.\n", argv[1]); break;
    case ENFILE: fprintf(stderr, "The system on the total number of open files has been reached.\n"); break;
    case ENOENT: fprintf(stderr, "%s: File does not exist.\n", argv[1]); break;
    case ENOEXEC: fprintf(stderr, "%s cannot be executed.\n", argv[1]); break;
    case ENOMEM: fprintf(stderr, "%s: Insufficient kernel memory.\n", argv[1]); break;
    default: fprintf(stderr, "%s exited with error number %i.\n", argv[1], errno); break;
    }
    exit(127);
  } else {
    struct rusage cpu_time;
    double time_before = monotonic_seconds();
    waitpid(pid, 0, 0);
    double time_after = monotonic_seconds();
    getrusage(RUSAGE_CHILDREN, &cpu_time);
    fprintf(stderr,
	    "real %f user %li.%06i sys %li.%06i\n", time_after - time_before,cpu_time.ru_utime.tv_sec,cpu_time.ru_utime.tv_usec,cpu_time.ru_stime.tv_sec,cpu_time.ru_stime.tv_usec);
  }
  return 0;
}

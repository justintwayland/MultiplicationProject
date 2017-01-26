#include <mach/mach_time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static mach_timebase_info_data_t info;
static void __attribute__((constructor)) init_info() { mach_timebase_info(&info); }
static double monotonic_seconds() {
  double dtime = (double) mach_absolute_time();
  dtime *= (double) info.numer;
  dtime /= (double) info.denom;
  return dtime / 1000000000.0;
}

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

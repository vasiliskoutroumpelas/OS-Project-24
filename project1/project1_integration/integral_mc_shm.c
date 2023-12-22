/*
Vasileios Koutroumpelas, 1093397
Filippos Minopetros, 1093431
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

double get_wtime(void) {
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double)t.tv_sec + (double)t.tv_usec * 1.0e-6;
}

double f(double x) {
  return sin(cos(x));
}

int main(int argc, char *argv[]) {
  unsigned long n = 4;  // Number of cores available in the testing machine
  
  if (argc == 2) {
    n = atol(argv[1]);
  }

  double a = 0.0;
  double b = 1.0;
  const double h = (b-a)/n;
  const double ref = 0.73864299803689018;
  double res = 0;
  double t0, t1;
  int status;

  t0 = get_wtime();

  // Create shared memory
  double *partial_results = mmap(NULL, n*sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  
  if (partial_results == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }
  
  // Fork processes
  pid_t *pid;
  pid = (pid_t *)malloc(n*sizeof(pid_t));
  for (unsigned long i = 0; i < n; i++) {
    pid[i] = fork();
    
    if (pid[i] == -1) {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    
    if (pid[i] == 0) { // Child process
      srand48(getpid()); // Set a different seed for every process
      double xi = drand48();
      partial_results[i] = f(xi);
      exit(EXIT_SUCCESS);
    }
  }

  
  for (unsigned long i = 0; i < n; i++) {
    waitpid(pid[i], &status, 0); // Wait for child process to finish
    res += partial_results[i];
  }

  res *= h;
  t1 = get_wtime();

  printf("Result=%.16f Error=%e Rel.Error=%e Time=%lf seconds\n", res, fabs(res - ref), fabs(res - ref) / ref, t1 - t0);

  // Unmap shared memory
  if (munmap(partial_results, sizeof(double)*n) == -1) {
    perror("munmap");
    exit(EXIT_FAILURE);
  }
  
  free(pid);
  
  return 0;
}

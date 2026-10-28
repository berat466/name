// maxrss komut [arg...] : komutu calistirir (stdout/stderr atilir), cocugun tepe bellegini KiB olarak basar
#include <cstdio>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
int main(int argc, char** argv) {
  pid_t pid = fork();
  if (pid == 0) { int n = open("/dev/null", O_RDWR); dup2(n, 0); dup2(n, 1); dup2(n, 2); execv(argv[1], argv + 1); _exit(127); }
  int st; struct rusage ru; wait4(pid, &st, 0, &ru);
  printf("%ld", ru.ru_maxrss);
}

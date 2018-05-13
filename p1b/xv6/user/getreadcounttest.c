#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
  int x = getreadcount();
  char buffer;
  printf(1, "%d \n", x);
  int fd = open("test.txt", 0);
  read(fd, &buffer, 1);
  read(fd, &buffer, 1);
  read(fd, &buffer, 1);
  read(fd, &buffer, 1);
  read(fd, &buffer, 1);
  close(fd);
  x = getreadcount();
  printf(1, "%d \n", x);
  exit();
}

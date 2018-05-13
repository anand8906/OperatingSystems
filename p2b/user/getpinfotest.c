#include "types.h"
#include "pstat.h"
#include "user.h"
#include "fs.h"

int
main(int argc, char *argv[])
{
   struct pstat st;
   getpinfo(&st);
   printf(1,"%s\n","first");
   getpinfo(NULL);
   printf(1,"%s\n","second");
   getpinfo((struct pstat *)1000000);
   printf(1,"%s\n","third");
   exit(); 
}

#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"
#include "pstat.h"


int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_settickets(void)
{
  int tickets;
  if(argint(0, &tickets) < 0)
    return -1; 
  if(tickets <= 0) return -1;
  ticketcount += tickets;
  proc->ptickets = tickets;
  return 0;
}

int
sys_getpinfo(void)
{
  struct pstat *ps;
  //cprintf("%s\n","getting arg pointer");
  if(argptr(0,(void *)&ps, sizeof(*ps)) < 0)
    return -1;
  if(ps == NULL) return -1;
  //cprintf("%s\n","got arg pointer");
  getpinfo_p(ps);
  //cprintf("%s\n","out of getpinfo_p");
  return 0;
  /*
  struct pstat *ps;
  if(argptr(0,(void *)&ps, sizeof(*ps)) < 0)
    return -1;
  struct proc *p;
  int i = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++,i++)
  {
    ps->inuse[i] = (p->state != 0);
    ps->tickets[i] = p->ptickets;
    ps->pid[i] = p->pid;
    ps->ticks[i] = 0;  
  }
  */
}

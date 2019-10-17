/*
Walks through an example of the clone() syscall. Prints results to stdout.
*/
#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 0x1000

// this function will be run by the cloned process
void func(void *arg)
{
  exit();
}

int main(int argc, char *argv[])
{
  int child_pid;
  char *stack_bottom;

  // test join() before any threads were created: should return -1
  int freed_pid = join();
  if (freed_pid != -1)
  {
    printf(1, "Error: join() result should be -1 if this process has no threads\n");
  }

  // expand address space by 1 page
  // `stack_bottom` is now the address of the bottom of the new page
  stack_bottom = sbrk(PGSIZE);

  // run clone(), and provide the address to the test value, as well as the stack
  child_pid = clone(&func, (void*) 0, stack_bottom);

  freed_pid = join();

  // make sure freed_pid = child_pid
  if (freed_pid != child_pid)
  {
    printf(1, "Error: join() did not return the child's pid\n");
  }

  printf(1, "Test finished\n");
  exit();
}

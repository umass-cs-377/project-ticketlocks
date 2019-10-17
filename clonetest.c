/*
Walks through an example of the clone() syscall. Prints results to stdout.
*/
#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 0x1000

// this variable should be accessible and modifiable by the child
int sharedVal = 20;

// this function will be run by the cloned process
// it takes a void* argument which will be dereferenced to an integer pointer
void func(void *arg)
{
  int pid = getpid();
  printf(1, "Child: pid is %d\n", pid);
  printf(1, "Child: Dereferenced function arg to %d\n", *(int*) arg);
  *(int*) arg += 10;
  printf(1, "Child: Incremented arg's value by 10. arg is now %d\n", *(int*) arg);
  sharedVal += 10;
  printf(1, "Child: Incremented sharedVal by 10. sharedVal is now %d\n", sharedVal);

  exit();
  printf(1, "ERROR: Child continued past exit()\n");
}

int main(int argc, char *argv[])
{
  int parent_pid, child_pid;
  char *stack_bottom;
  int test_val = 0;

  parent_pid = getpid();
  printf(1, "Parent: pid is %d\n", parent_pid);

  // expand address space by 1 page
  // `stack_bottom` is now the address of the bottom of the new page
  stack_bottom = sbrk(PGSIZE);

  // run clone(), providing the function to be run, the address
  // to an arg, and the address of the bottom of the newly-
  // allocated page
  child_pid = clone(&func, (void*) &test_val, stack_bottom);

  // sleep while the cloned process runs
  // we do this so that we can run this test without using join()
  sleep(10);

  printf(1, "Parent: pid of cloned thread is %d\n", child_pid);
  if (child_pid <= parent_pid)
  {
    printf(1, "Error: Child pid should be greater than parent pid\n");
  }

  printf(1, "Parent: test_val is now %d\n", test_val);
  if (test_val != 10)
  {
    printf(1, "Error: test_val should have been incremented by 10\n");
  }

  printf(1, "Parent: sharedVal is now %d\n", sharedVal);
  if (sharedVal != 30)
  {
    printf(1, "Error: sharedVal should have been incremented to 30\n");
  }

  printf(1, "Test finished\n");
  exit();
  printf(1, "ERROR: Parent continued past exit()\n");
}

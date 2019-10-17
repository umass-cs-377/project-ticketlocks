/*
Runs several tests of the ticketlock.
*/
#include "types.h"
#include "stat.h"
#include "user.h"
#include "ticketlock.h"

#define PGSIZE 0x1000

// glabal variable
int sharedVal = 0;
int numAdditions = 200;
struct ticketlock lock;

void child_func(void *sleep_s) // TODO: WHAT ARE THE UNITS OF SLEEP?
{
  int pid = getpid();
  sleep((int) sleep_s);

  for (int i = 0; i < numAdditions; i++)
  {
    printf(1, "(%d) i is %d\n", pid, i);
    acquire_t(&lock);
    sharedVal++;
    release_t(&lock);
  }
  printf(1, "(%d) Finished\n", pid);
  exit();
}

void test_single_process()
{
  initlock_t(&lock);

  for (int i = 0; i < numAdditions; i++)
  {
    acquire_t(&lock);
    sharedVal++;
    release_t(&lock);
  }

  printf(1, "sharedVal is now %d\n", sharedVal);
}

void test_cloned_process()
{
  initlock_t(&lock);
  char *stack;
  stack = sbrk(PGSIZE);
  clone(&child_func, (void*) 0, stack);
  join();
}

void test_two_cloned_processes()
{
  initlock_t(&lock);
  char *stack1, *stack2;

  stack1 = sbrk(PGSIZE);
  clone(&child_func, (void*) 10, stack1);

  stack2 = sbrk(PGSIZE);
  clone(&child_func, (void*) 0, stack2);

  join();
  join();
}

int main(int argc, char *argv[])  // TODO: INCLUDE WIKIPEDIA TICKETLOCK PAGE IN README
{
  printf(1, "Testing single process\n");
  test_single_process();
  printf(1, "Testing cloned process\n");
  test_cloned_process();
  printf(1, "Testing two cloned processes\n");
  test_two_cloned_processes();

  printf(1, "Parent: sharedVal is now %d\n", sharedVal);
  exit();
}

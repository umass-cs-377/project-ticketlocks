# Overview

In this project, you will implement support for multi-threading in xv6. There
are three parts to the assignment:

1. Implement the `clone()` system call
2. Implement the `join()` system call
3. Implement ticket locks

The `clone()` and `join()` system calls are necessary for introducing the notion
of a _thread_ in the xv6 kernel. The ticket lock mechanism is used to
synchronize across multiple threads. The wikipedia entry for ticket locks
describe ticket locks as:

> A ticket lock is similar to the ticket queue management system. This is the
> method that many bakeries and delis use to serve customers in the order that
> they arrive, without making them stand in a line. Generally, there is some
> type of dispenser from which customers pull sequentially numbered tickets upon
> arrival. The dispenser usually has a sign above or near it stating something
> like "Please take a number". There is also typically a dynamic sign, usually
> digital, that displays the ticket number that is now being served. Each time
> the next ticket number (customer) is ready to be served, the "Now Serving"
> sign is incremented and the number called out. This allows all of the waiting
> customers to know how many people are still ahead of them in the queue or
> line.

> Like this system, a ticket lock is a first in first out (FIFO) queue-based
> mechanism. It adds the benefit of fairness of lock acquisition and works as
> follows; there are two integer values which begin at 0. The first value is the
> queue ticket, the second is the dequeue ticket. The queue ticket is the
> thread's position in the queue, and the dequeue ticket is the ticket, or queue
> position, that now has the lock (Now Serving).

> When a thread arrives, it atomically obtains and then increments the queue
> ticket. The atomicity of this operation is required to prevent two threads
> from simultaneously being able to obtain the same ticket number. It then
> compares its ticket value, before the increment, with the dequeue ticket's
> value. If they are the same, the thread is permitted to enter the critical
> section. If they are not the same, then another thread must already be in the
> critical section and this thread must busy-wait or yield. When a thread leaves
> the critical section controlled by the lock, it atomically increments the
> dequeue ticket. This permits the next waiting thread, the one with the next
> sequential ticket number, to enter the critical section. See the
> [wikipedia entry](https://en.wikipedia.org/wiki/Ticket_lock) for additional
> details.

The hardest part of this project is not necessarily in coming up with how the
`clone()` and `join()` systems calls are implemented nor in how to implement
ticket locks. Indeed, we give you much of what you need for the implementation.
The hard part is understanding where everything needs to go (which source
files). To be successful with this project requires you to follow the
instructions in this document precisely and to review each of the source files
that we mention.

# Getting Started

To get started, open terminal and `cd` into the project directory
(where this `README.md` file is). Run the CS377 Docker container and
attach the current directory as file storage. If you are not using
Docker then use the Linux environment that you have been using for
previous projects. Here is the command you need to run to get Docker
going:

```
docker run --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --privileged -it -v /absolute/path/to/here:/mnt/files mcorner/os377 bash
```

Make sure you get the path correct! You need to replace
`/absolute/path/to/here` with the directory containing this
`README.md` file and other assets included with this project. It might
look something like this: `/home/user/Documents/CS377/ticket-locks`.

In the Docker container, navigate to the mounted directory.

```
$ cd mnt/files
```

At this point, if you run `ls` in this directory you should see:

```shell
Makefile
README.md
clonetest.c
git_setup.sh
jointest.c
locktest.c
```

Use git to clone the xv6 repository.

```
$ git clone https://github.com/mit-pdos/xv6-public.git
```

This will copy the xv6 repository into a folder called
`xv6-public`. Now if you run `ls` in this directory you should see:

```shell
Makefile
README.md
clonetest.c
git_setup.sh
jointest.c
locktest.c
xv6-public/
```

Next, navigate to the `xv6-public` directory and run the `git
checkout` command below:

```
$ cd xv6-public
$ git checkout b818915f793cd20c5d1e24f668534a9d690f3cc8
```

**Don't forget to run `git checkout`**, which will get you to the
exact version you need for this project. From here on out, you will do
all of your work inside the `xv6-public` directory.

# Running xv6

In the xv6-public directory, execute the following commands:

```
make clean
make
make qemu-nox
```

If this is giving you problems, make sure you run `make clean` before `make`.
This won't fix any programming errors, but it may fix strange compilation
behavior. Any changes you make to the source code as described below will
require you to run each of these commands in order. `make clean` will clean up
any previously generated files, `make` will build the xv6 kernel, and
`make qemu-nox` will build the `qemu` simulator and run the xv6 in the
simulator. To exit the qemu simulator type `<ctrl+x a>` (`control` and the `x`
key at the same time followed by the character `a`).

You should try compiling and running xv6 before you make any changes to the code
to ensure that you are starting from something that works.

# Part 1: `clone()`

The `clone()` system call creates a new thread-like process. It is very similar
to `fork()` in that it creates a child process from the current process, but
there are a few key differences:

- the child process shares the address space of the original process (`fork()`
  creates a whole new copy of the address space, keeping the child completely
  separate from the parent)
- the child process's stack is in the address space of the original process
  (`fork()` creates a separate stack for the child process)
- clone() takes a pointer to a function, and runs the function as soon as the
  child process starts (`fork()` starts the child at the same place in code as
  the parent)

The full signature of the function is
`int clone(void (*fcn)(void*), void *arg, void *stack)`:

- `void (*fcn)(void*)` is a pointer to the function that the created thread will
  run once it starts. It must return `void` and take a single `void*` argument.
  Take a look at [this](https://www.zentut.com/c-tutorial/c-function-pointer)
  for further information on function pointer declarations in C.
- `void *arg` is the argument that will be passed to `*fcn` once the child runs
- `void *stack` is a memory address in the parent's address space which tells
  the child where to put its stack. This allows the child to use the parent's
  address space for its stack, making it a thread of the parent process (and not
  its own self-sufficient process).

Remember: a `void*` is used to point to a data type that is not yet specified.
This allows the software engineer to decide the data type that is passed in and
later dereferenced. For example, in this case `void *stack` indicates a memory
location and is later cast to `uint`.

We suggest you read about the
[Linux clone() call](https://linux.die.net/man/2/clone), which is very similar
to what we will implement.

## Adding a System Call

Adding a system call to xv6 requires changing multiple files. For each
instruction, pay attention to the way other system calls have already been
implemented. Follow the conventions! Use the already-existing system calls to
help you figure out how to implement the new one. Certain files will use the
definition `int sys_clone(void)`, and others will use the full definition
(`int clone(void(*fcn)(void*), void *arg, void *stack)`). You can figure this
out by looking at the other system calls in the file.

You should also look at the `Details` section below to better understand what is
going on.

## Implementation: `sysproc.c`

We'll start by adding to `sysproc.c`, which provides wrapper functions to the
raw system calls. These wrapper functions make sure the user has provided the
right number and type of arguments before forwarding the arguments to the actual
system call.

This wrapper function will have the signature:

```c
int sys_clone(void) {
	// To be filled in...
}
```

Note that it doesn't take arguments! The arguments are waiting in the stack at
known offsets from the `%esp` register. xv6 provides access to these arguments
through the `argptr()` function, which you can check out in `syscall.c`.

The `clone()` system call takes three arguments: `void *fcn, *arg, *stack`.
We'll need to retrieve each of these values. Start by declaring them in the
`sys_clone()` function:

```c
void *fcn, *arg, *stack;
```

The code below shows how to get the 0th argument, cast to `void*` and load it
into `*fcn`:

```c
// load argument 0 into the fcn pointer and return -1 if it is not void*
if (argptr(0, (void *)&fcn, sizeof(void *)) < 0)
	return -1;
```

Follow this approach for the other two arguments. Finally, call and return the
result of the actual system call:

```c
return clone(fcn, arg, stack);
```

## Implementation: `proc.c`

With this in place, we have the groundwork in place to implement `clone()`. Open
`proc.c` and declare the clone function. Copy the body of the `fork()` system
call into the body of our new `clone()` function. We will use this code as a
starting point and add and remove parts as we go:

```c
01: int clone(void(*fcn)(void*), void *arg, void *stack) {
02:   int i, pid;
03:   struct proc *np;
04:   struct proc *curproc = myproc();
05:
06:   // Allocate process.
07:   if ((np = allocproc()) == 0) {
08:     return -1;
09:   }
10:
11:   // Copy process state from proc.
12:   if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
13:     kfree(np->kstack);
14:     np->kstack = 0;
15:     np->state = UNUSED;
16:     return -1;
17:   }
18:   np->sz = curproc->sz;
19:   np->parent = curproc;
20:   *np->tf = *curproc->tf;
21:
22:   // Clear %eax so that fork returns 0 in the child.
23:   np->tf->eax = 0;
24:
25:   for (i = 0; i < NOFILE; i++)
26:     if (curproc->ofile[i]) np->ofile[i] = filedup(curproc->ofile[i]);
27:   np->cwd = idup(curproc->cwd);
28:
29:   safestrcpy(np->name, curproc->name, sizeof(curproc->name));
30:
31:   pid = np->pid;
32:
33:   acquire(&ptable.lock);
34:
35:   np->state = RUNNABLE;
36:
37:   release(&ptable.lock);
38:
39:   return pid;
40: }
```

1.  The first thing we want to do is get rid of variables that we will not use
    in our `clone()` implementation. We can get rid of the declaration for `pid`
    (line 02). There is no need to have this variable in our `clone`
    implementation.
2.  In order to "clone" the current process, the stack address provided must be
    page-aligned and it must have at least one page of memory. We need to check
    that both of these conditions are true. If they are not, then we need to
    return `-1`. To do this you need to add two `if` statements on line 05. The
    first `if` statement performs the alignment check,
    `((uint) stack % PGSIZE) != 0`, to see if the stack is not aligned. If this
    is true, we return `-1`. The second `if` statement returns `-1` if we have
    less than 1 page of memory, `(curproc->sz - (uint) stack) < PGSIZE`.
3.  We can keep the code from line 06 - 09 as we still need to allocate a new
    process structure to hold the state for the clone. `fork()` creates a full
    copy of the parent's address space, while `clone()` shared the parent's
    address space. In xv6, the address space is recorded in the process
    structure with the `pgdir` (page directory) field. To share the parent's
    address space with the clone we can simply replace lines 11 to 17 with
    `np->pgdir = curproc->pgdir;`. That is, the cloned process `np` and the
    current process `curproc` share the same page directory (e.g., memory
    space).
4.  Lines 18 to 20 need not be touched as we are saving important information
    from the parent process to the clone. In particular, line 18 is recording
    the size of process memory in bytes, line 19 is recording the parent
    process, and line 20 is saving the trap frame for the current system call
    (see lines 161 to 194 in x86.h).
5.  The stack and stack base frame registers, `esp` and `ebp` respectively, must
    be set properly as we want to return into the function `fcn` that we are
    providing to the `clone` system call. Also, the child process needs two
    values on its stack once it starts: the first is `0xffffffff` and the second
    is the argument passed to `*fcn`. The cloned process runs `fcn` as soon as
    it exits the trap frame. To accomplish this, the `eip` register of the trap
    frame should be set to the memory address of `fcn`. You can accomplish all
    of this with the following code placed into line 21:

    ```c
    uint user_stack[2];
    user_stack[0] = 0xffffffff;
    user_stack[1] = (uint) arg;
    // set top of the stack to the allocated page
    // (stack is actually the bottom of the page)
    uint stack_top = (uint) stack + PGSIZE;
    // subtract 8 bytes from the stack top to
    // make space for the two values being saved
    stack_top -= 8;
    // copy user stack values to np's memory
    if (copyout(np->pgdir, stack_top, user_stack, 8) < 0) {
    	return -1;
    }

    // set stack base and stack pointers for return-from-trap
    // they will be the same value because we are returning into a function
    np->tf->ebp = (uint) stack_top;
    np->tf->esp = (uint) stack_top;
    // set instruction pointer to address of function
    np->tf->eip = (uint) fcn;
    ```

6.  Lines 22 and 23 do not require changes. We are simply setting the return
    address of the clone (which is stored in the `eax` register) to `0` - the
    same value as returned in the `fork()` system call.
7.  Lines 25 to 39 can mostly remain the same with two small changes. You can
    delete line 31 and then change line 39 to be `return np->pid`.

## System Call Glue

Now that we have the implementation of the `clone` system call, we need to glue
it in to the rest of the operating system. To do this we will need to make
changes to the following files:

- `syscall.h`
- `syscall.c`
- `usys.S`
- `user.h`
- `defs.h`

These additions do not require you to know the internal guts of the operating
system, rather, you need to simply follow the pattern. Look at how other system
calls have been defined. In short, to glue the system call implementation of
`clone` into the rest of the system you need to add the following:

1. `syscall.h`: Add a new system call number to the end of the `#define` list.
2. `syscall.c`: Add an extern definition for the new system call and add an
   entry to the table of system calls. Search for `fork` to see what you need to
   do. Follow the pattern.
3. `usys.S`: Add an entry for clone. This makes use of a C macro that generates
   some inline assembly which loads the `eax` register with the address of the
   system call and traps to the operating system using the x86 `int` (interrupt)
   instruction.
4. `user.h`: Add a function prototype for the system call (`clone` for this
   part).
5. `defs.h`: Add a function prototype for the system call (`clone` for this
   part).

Once you've done this, the system call is recognized by the operating system and
can be called by user programs.

# Tips

It is the parent processes' job to allocate memory for the child's stack. The
below example, taken from `clonetest.c`, shows how to call `clone()` from a user
program:

```c
// expand address space by 1 page and get address of bottom
char *stack = sbrk(PGSIZE);
// run clone() with the given function and the argument 10
int cloned_pid = clone(&func, (void*) 10, stack);
```

Because we haven't implemented a `join()` method yet, the OS will never receive
a signal from the parent to kill the child. After your `clone()` executes, your
program will hang, and xv6 will print "zombie process". Don't worry about that,
it's unavoidable without a thread `join()` method. See `clonetest.c` to see how
`clone()` is used in practice, and follow the instructions below explaining how
to test the clone system call.

## Details

Essentially, a `thread` is a special type of process that shares the address
space of its parent.

To allocate space for the new thread's stack, we use the `sbrk` syscall, which
allocates more virtual memory to the process. `sbrk` returns the _previous_ size
of the virtual address space, which happens to be the _bottom_ of the newly
allocated memory. This allows us to check how much memory is available for the
stack by calculating the difference between the new memory size (`curproc->sz`)
and the stack pointer provided.

Why does `clone` need to take a stack address at all? It would be a lot simpler
if `clone` performed the `sbrk` allocation on its own. However, by requiring an
address, it gives the user the option. In reality, we would normally build a
thread library such as `pthread` on top of the raw syscalls that would do this
automatically, and the user wouldn't need to know about any stack addresses.

Much of `clone` is similar to `fork` because both involve creating a copy of the
current process. However, because the cloned child process shares the address
space of the parent process, it will set its `pgdir` (address space) to that of
the parent, instead of making a full, separate copy.

`clone` must also set things up so that the cloned process starts off by calling
the given function. In order to accomplish that, we need to set up the child's
stack and trap frame. The trap frame stores the state of the processes'
registers, and will be restored by the OS on the return-from-trap. The program
will continue execution with whatever is in its stack and registers, oblivious
that the operating system ever took control. You can read the section on system
calls "Code: Assembly trap handlers" in the
[xv6 textbook](https://pdos.csail.mit.edu/6.828/2014/xv6/book-rev8.pdf) to learn
more.

The stack starts at a high address, and expands downward by subtraction (this is
worth reviewing online). A process manages its stack using a `base pointer`
(`ebp`), which stores the address of the top of the current frame and a
`stack pointer` (`esp`) storing the lowest address the stack has reached.
Between the `base pointer` and `stack pointer` is the current stack frame, which
stores the local variables of the current scope.

The value stored at the base pointer is the address of the previous base, and is
used to go back to the previous stack frame once the current function returns.
The values immediately following the base pointer are the arguments passed to
the current function being executed.

With that in mind:

- `user_stack[0] = 0xffffffff`: This sets the first value in the stack, which
  the base pointer will be set to. It's a "fake" value because there is no
  previous base pointer.
- `user_stack[1] = (uint) arg`: This sets the first value after the base pointer
  to the function argument. As mentioned earlier, function arguments are placed
  in order on the stack. So, the cloned function will retrieve `arg` as its
  first argument.
- `np->tf->ebp = (uint) stack_top`: The base pointer is set to the top of the
  newly-allocated memory.
- `np->tf->esp = (uint) stack_top`: The stack pointer is also set to the of the
  page because that is where our function will enter. The process will use `esp`
  to find the function args passed to it, and start execution from there.
  Normally, `ebp` and `esp` would not be the same, but this is a special case
  because we are starting a new process!
- `np->tf->eip = (uint) fcn`: This sets the instruction pointer register. This
  ensures the cloned process will run `fcn` on start.
- `np->tf->eax = 0`: This is the return register. The child process returning
  from `clone` should get `0` as a return value.

When xv6 returns from trap, it will set the processes' registers to those stored
in the trap frame.

## Testing

In order to test your system calls, we've provided several simple user programs
that you can run inside xv6: `clonetest.c`, `jointest.c` and `locktest.c`. You
can copy each of these source files into your `xv6-public` directory (along with
the rest of the xv6 source code). If you want to modify these files, make sure
to use the special conventions used for xv6 user programs (see the `print()` and
`exit()` calls, for example).

This section describes how to add a user program to xv6. It uses `clonetest.c`
as an example, but the other test programs can be added in the same way.

Find the line `EXTRA=\` inside the `Makefile`, and add `clonetest.c` to the list
of files:

```
EXTRA=\
	mkfs.c ulib.c user.h cat.c echo.c forktest.c grep.c kill.c\
	ln.c ls.c mkdir.c rm.c stressfs.c usertests.c wc.c zombie.c\
	printf.c umalloc.c\
	README dot-bochsrc *.pl toc.* runoff runoff1 runoff.list\
	.gdbinit.tmpl gdbutil\
	# Add it at the end like so:
	clonetest.c\
```

Then find the line `UPROGS=\` inside the `Makefile`. Add your file in the same
format as the other entries in this definition. Your line should be formatted
like `_clonetest\`. The end result should look like this:

```c
UPROGS=\
	_cat\
	_echo\
	_forktest\
	_grep\
	_init\
	_kill\
	_ln\
	_ls\
	_mkdir\
	_rm\
	_sh\
	_stressfs\
	_usertests\
	_wc\
	_zombie\
	# Add it at the end like so:
	_clonetest\
```

For both of these modifications it is **important** that you:

1. Ensure your indentation matches the other lines that you are adding your
   entry after.
2. Ensure that you add the backslash `\` after your entry.

If you do not do this correctly you are likely to see errors and your will not
be able to run xv6 or test the system calls.

To run the test program, start xv6 (as described previously) and run `clonetest`
from the command line in the running xv6 kernel. If you have followed the above
instructions precisely, you will see the following output after you run the
`clonetest` program:

```shell
$ ./clonetest
Parent: pid is 4
Child: pid is 5
Child: Dereferenced function arg tPao 0
Child: Incremented arg's varent: pid of lcloned thread is 5
Parent: test_ue by 10. argva is now 10
Child: Incl is now 10
Parent: sharemented sharedVal by 10. sharedVal is now 30
redVal is now 30
Test finished
zombie
```

# Part 2: `join()`

`clone()` creates a thread, but we're missing some fundamental
functionality.  First, we have no way for our parent process to wait
for a child to finish execution. Second, the parent has no way to read
the exit status of a child.  This causes the child to become
a [zombie](https://en.wikipedia.org/wiki/Zombie_process) once it
exits. A zombie is a process that has finished execution, but remains
in the operating system's process table until the parent checks its
exit status.

The `int join()` system call will fix both of these problems. It will
check the list of currently running processes, looking for a thread
belonging to the parent process. If it finds such a process, it will
"kill" it (clear its entry in the process table) by setting it to the
UNUSED state and resetting all of its values. It will then return the
pid of the child thread that was killed.

## Implementation: `sysproc.c`

As we did for `clone`, we need to first add a `sys_join` wrapper
function that actually does the job of invoking `join`. `join` returns
`-1` if no child thread is found, and will sleep if it finds a child
thread that is still running. The `sysproc.c` wrapper function looks
like this:

```c
int sys_join(void) {
  return join();
}
```

## Implementation: `proc.c`

You can now add the implementation in `proc.c`. Just like `clone` was similar to
`fork`, `join` is similar to `wait` (which does the same thing, but for
processes). Create a new function called `join` and copy the code body
from `wait` into our newly created function `join`:

```c
01: int join(void) {
02:   struct proc *p;
03:   int havekids, pid;
04:   struct proc *curproc = myproc();
05:
06:   acquire(&ptable.lock);
07:   for(;;){
08:     // Scan through table looking for exited children.
09:     havekids = 0;
10:     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
11:       if(p->parent != curproc)
12:         continue;
13:       havekids = 1;
14:       if(p->state == ZOMBIE){
15:         // Found one.
16:         pid = p->pid;
17:         kfree(p->kstack);
18:         p->kstack = 0;
19:         freevm(p->pgdir);
20:         p->pid = 0;
21:         p->parent = 0;
22:         p->name[0] = 0;
23:         p->killed = 0;
24:         p->state = UNUSED;
25:         release(&ptable.lock);
26:         return pid;
27:       }
28:     }
29:
30:     // No point waiting if we don't have any children.
31:     if(!havekids || curproc->killed){
32:       release(&ptable.lock);
33:       return -1;
34:     }
35:
36:     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
37:     sleep(curproc, &ptable.lock);  //DOC: wait-sleep
38:   }
39: }
```

The key difference between `join` and `wait` is that the thread to be joined
shares the address space of its parent, so `join` shouldn't touch its virtual
memory (`wait` frees the virtual memory of the child process).

Because of this, a child thread will have the same `pgdir` attribute as its
parent. To tell if a process is a child thread of the current process, it must
have its parent equal to the current process _and_ have the same `pgdir` (`wait`
just needs to find a process whose parent is equal to the current process).

The required changes are stated below:

1. Rename the `havekids` variable to `havethreads` (or some other
   variant).  There are multiple occurrences of `havekids`. Make sure
   you change all occurrences inside the `join` system call. This will
   be on lines 03, 09, 13, and 31.
2. In the for-loop that loops over processes in the process table
   (`for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)`), add an `if`
   statement between lines 10 and 11 that checks that the process `p`
   has the same `pgdir` as the current process 
   (`p->pgdir == curproc->pgdir`). If it doesn't, then we can simply 
   `continue`. If they share the same page directory it means that the `p` must be a
   "child" thread of the parent process (e.g., `curproc`).
3. In the condition that checks if a child thread is a zombie (`if
   (p->state == ZOMBIE)`), remove the line that frees the child's
   `pgdir` (line 19). If we left this line in, the child thread's
   address space would be freed.  Because the child is a thread, this
   is also the parent's address space!  Freeing it would break the
   parent process.

## System Call Glue

Now that you have the implementation for `join` in place, you need to
glue the system call implementation into the rest of the
system. Follow the instructions in the **System Call Glue** section
for `clone` to complete the job.

## Testing

Refer to the documentation on how to add a user program for testing `clone()`.
To test `join()` you should add the `jointest.c` user program. Running
the test will result in the following output:

```shell
$ ./jointest
Test finished
```

# Part 3: Ticket Locks

With `clone` and `join` we have the ability to create and join
threads, but we lack a way to protect data from being accessed by
multiple threads simultaneously. To provide support for
synchronization we will add a spinning ticketlock to xv6.

A ticketlock is one way to implement a mutex, but adds a little bit of
complexity in order to improve fairness. Normal mutexes can have
starvation issues, where one thread manages to acquire the lock before
other threads almost all the time, preventing other threads from
getting access. A ticketlock has a `turn` and a `ticket`. When a
thread wants to acquire the lock, it receives a `ticket` number. It
then waits for the lock's `turn` to equal its `ticket`.  Meanwhile,
the lock increments the `turn` each time a thread releases the lock.
This creates a simple FIFO queue
system. Read
[the wikipedia page](https://en.wikipedia.org/wiki/Ticket_lock) for
further details.

## Implementation

Create a new file in the `xv6-public` directory called
`ticketlock.h`. This will define the `ticketlock` structure, which
user programs can use to declare and use ticketlocks. We will
implement ticketlock methods as system calls, with xv6 supporting the
functionality.

```c
// Spinning ticket lock.
typedef struct ticketlock {
  uint ticket;        // current ticket number being served
  uint turn;          // next ticket number to be given
  struct proc *proc;  // process currently holding the lock
} ticketlock;
```

As you can see, the ticketlock stores the current ticket and turn
numbers. It also stores a reference to the process currently holding
the lock. This will allow it to make sure a process can't request the
lock while it is already holding the lock.

Next, add three system calls:

```c
void initlock_t(struct ticketlock *lk)
void acquire_t(struct ticketlock *lk)
void release_t(struct ticketlock *lk)
```

Follow the instructions for adding system calls that were provided in the
`clone()` and `join()` sections above.

Add the wrapper functions in `sysproc.c`. They will all follow this
general format:

```c
int sys_initlock_t(void)
{
  struct ticketlock *tl;
  if (argptr(0, (char**)&tl, sizeof(struct ticketlock*)) < 0)
  {
    return -1;
  }

  initlock_t(tl);
  return 0;
}
```

The ticketlock functions will require an _atomic_ fetch-and-add
operator. This is to ensure that there are no concurrency bugs when
xv6 increments the lock's `ticket` variable (which is the ticket
number for the next process that requests the lock). Add the following
`fetch_and_add` method, which runs an atomic assembly instruction, to
`x86.h`:

```c
// atomic fetch-and-add operation
static inline uint
fetch_and_add(volatile uint *addr, uint val)
{
  asm volatile("lock; xaddl %%eax, %2;" :
               "=a" (val) :
               "a" (val) , "m" (*addr) :
               "memory");
  return val;
}
```

`fetch_and_add` atomically adds one to the value at the given memory
address and returns the original value. When a thread requests the
lock, it will therefore get the _current_ ticket value, and increment
it for the next thread to request the lock. A thread receives access
when its `ticket` equals the current `turn`.  Therefore, both `ticket`
and `turn` start at zero.

You will also need to add a helper method in `proc.c` to determine if the
current process is holding the lock.

```c
// return whether the given ticketlock is held by this process.
int holding_t(struct ticketlock *lk)
{
  ...
}
```

For a process to be holding a lock, the lock's `proc` must equal the
current process (which you can get with `myproc()`). The lock must
also be held by a process - this will be the case when `turn !=
ticket`. Return whether both of these conditions are true.

In `proc.c` write the `initlock_t` method:

```c
void initlock_t(struct ticketlock *lk)
{
  ...
}
```

This method must:

- set lk's `ticket` to 0
- set lk's `turn` to 0
- set lk's `proc` to 0

Next, write the `acquire_t` method:

```c
void acquire_t(struct ticketlock *lk)
{
  ...
}
```

This function must:

1. check whether the process is already holding the lock (call
   `holding_t`). If it is, call `panic()`, which is essentially used
   as a way to stop xv6 if there is an exception. You can pass a
   string message to `panic`, for example `panic("lock already
   acquired");`
2. use `fetch_and_add` to add one to the lock's current `ticket`
   field. Save the result as a `uint` - this is the current processes'
   ticket number.
3. spin while the lock's `turn` is not equal to the `ticket` value you
   saved. Use a while loop and continue looping until the condition
   has been met.
4. set the lock's `proc` field to `myproc()`

Finally, implement `release_t`:

```c
void release_t(struct ticketlock *lk)
{
  ...
}
```

This method must:

1. check whether the process is already holding the lock, and panic if
   it isn't.
2. set the lock's `proc` field to `0` (to indicate the ticketlock
   isn't held by any process at the moment)
3. increment the lock's `turn` field by one. This does not need to be
   atomic, because only one thread can be in this function at a time
   (the thread currently holding the lock).

You will also need to add `ticketlock` includes in the following files:

- `struct ticketlock` at the top of `user.h`
- `#include "ticketlock.h"` at the top of `proc.c`
- `struct ticketlock` at the top of `defs.h`

And that's it! You can now use ticketlocks by including `ticketlock.h`
in your user programs.

## Testing

Refer to the documentation on how to add a user program for testing
`clone()` and `join()`. To test locks you should add the `locktest.c`
user program.

# Submission

To submit your project, run `make submission` in the directory
containing this `README.md` file (i.e., not inside the `xv6-public`
directory. Submit the created zip file to Gradescope.

# Additional Resources

- [How `clone()` works in Linux systems](https://linux.die.net/man/2/clone)
- [The `fork()` implementation in xv6](https://github.com/mit-pdos/xv6-public/blob/b818915f793cd20c5d1e24f668534a9d690f3cc8/proc.c#L181)

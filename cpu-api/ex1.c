#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/wait.h>

void dothething1and2(void)
{
  int x = 10;
  FILE *fd;
  char buf[100];

  // if the fd is open here, they read simultaneous from the same stream!
  // fd = fopen("test.txt", "rw");
  // assert(fd != NULL);

  printf("start {%d}: {%d}\n", getpid(), x);

  pid_t rc = fork();
  assert(rc >= 0);

  if (rc == 0)
  {
    // child

    // but if they're opened here, when in different processes,
    // they have their own stream!
    fd = fopen("test.txt", "rw");
    assert(fd != NULL);
    x++;
    fgets(buf, x, fd);
    printf("child {%d}: {%d} %s\n", getpid(), x, buf);
  }
  else
  {
    // parent
    rc = wait(NULL);
    assert(rc > 0);

    fd = fopen("test.txt", "rw");
    assert(fd != NULL);
    x += 2;
    fgets(buf, x, fd);
    printf("parent {%d}: {%d} %s\n", getpid(), x, buf);
    x += 3;
  }

  printf("end {%d}: {%d}\n", getpid(), x);
}

void dothething3(void)
{
  // Both processes have access to this pointer,
  // So we can implement `wait` without the syscall `wait`
  bool *t = 1;
  bool *wait = &t;
  pid_t rc = fork();
  if (rc == 0)
  {
    printf("hello!\n");
    *wait = 0;
  }
  else
  {
    // Guarantees the parent to always print last
    // in other words, a manual implementation of `wait`
    while (wait)
    {
      // If there's no sleep, we are locked into eternity...
      sleep(1);
      break;
    }
    printf("goodbye!\n");
  }
}

void dothething4(void)
{
  pid_t rc = fork();
  if (rc == 0)
  {
    // exec - l variant
    // int err = execl("/bin/ls", "~");

    // let's try with the v variant
    char *args[2] = {
        "~",
        (char *)NULL};
    int err = execv("/bin/ls", args);
    if (err == -1)
    {
      printf("error executing ls\n");
      exit(-1);
    }
  }
  else
  {
    rc = wait(NULL);
  }
}

void dothething5(void)
{
  pid_t rc = fork();
  if (rc == 0)
  {
    // what happens when we wait in the child?
    // -1 returns, since there is no child of this child to wait on!
    rc = wait(NULL);
    printf("child: %d\n", (int)rc);
  }
  else
  {
    printf("parent\n");
  }
}

void dothething6(void)
{
  pid_t rc = fork();
  if (rc == 0)
  {
    printf("hello %d\n", getpid());
  }
  else
  {
    pid_t other_rc = fork();

    if (other_rc == 0)
    {
      printf("hello %d\n", getpid());
      sleep(3);
    }
    else
    {
      // waitpid - when you want more control of the async
      //   we spawned two children, but we wait on the second to finish first
      //   even though it was fork'ed later, and takes longer to execute!
      int *statloc;
      other_rc = waitpid(other_rc, statloc, 0);
      printf("goodbye child #2 %d %d\n", other_rc, statloc);

      rc = wait(NULL);
      printf("most final goodbye to child #1 %d\n", rc);
    }
  }
}

void dothething7(void)
{
  pid_t rc = fork();
  if (rc == 0)
  {
    printf("child\n");
    printf("closing stdout in child\n");
    close(STDOUT_FILENO);

    printf("we don't see this!\n");
  }
  else
  {
    printf("parent\n");
    rc = wait(NULL);
    printf("we still see this in the parent!\n");
  }
}

void dothething8(void)
{
  int pipefd[2];
  char buf[100];

  // establish the pipe before any forks so we're working with the same fd's
  if (pipe(pipefd) == -1)
  {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  pid_t rc = fork();
  if (rc == 0)
  {
    printf("child #1 - gets the write end\n");

    // close the unused read end for this process
    close(pipefd[0]);

    // write to the pipe
    const char *msg = "child 2 should print this!\n";
    write(pipefd[1], msg, strlen(msg));

    // close the write end of the pipe, we're done
    // the reader will see EOF after this
    close(pipefd[1]);
  }
  else
  {
    rc = fork();
    if (rc == 0)
    {
      printf("child #2 - gets the read end\n");

      // close the unused write end of the pipe
      close(pipefd[1]);

      // read from the pipe and print to stdout
      while (read(pipefd[0], &buf, 1) > 0)
        write(STDOUT_FILENO, &buf, 1);

      // close the read end of the pipe, we're done
      close(pipefd[0]);
    }
    else
    {
      printf("parent\n");

      // close both ends of the pipe in the parent
      // the parent is only an orchestrator, not involved in the pipe
      close(pipefd[0]);
      close(pipefd[1]);

      wait(NULL);
    }
  }
}

int main(void)
{
  // dothething1and2();
  // dothething3();
  // dothething4();
  // dothething5();
  // dothething6();
  // dothething7();
  dothething8();
  return 0;
}

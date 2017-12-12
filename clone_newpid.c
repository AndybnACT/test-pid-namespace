#include <stdio.h> // IO
#include <stdlib.h> // exit perror

#define __USE_GNU
#include <sched.h>
#include <unistd.h> // fork
#include <sys/wait.h> // waitfor



#define STACKSIZE 8192

#include "processes.h"
#define NUM_SUBSPACE 2

int main(void) {
    pid_t pid, c_pid;
    int status=-1;
    pid = getpid();

    // arrange stack for clone
    void *clone_stack;
    if ( !(clone_stack = (void *) malloc(STACKSIZE*sizeof(char))) ) {
        perror("malloc error");
        exit(1);
    }
    // arguments
    struct init_p_arg init;
    init.ppid = pid;
    init.sub_namespace = NUM_SUBSPACE;

    // clone
    if ( (c_pid = clone( &init_p, (void *)( (char *)(clone_stack)+STACKSIZE), CLONE_NEWPID | SIGCHLD, (void *)(&init) ) ) == -1) {
        perror("clone error");
    }else{
        printf("[%d] created [%d]\n", pid, c_pid);
    }

    // wait for child
    //sleep(1);
    if (waitpid(c_pid, &status, 0) != c_pid){
        perror("wait pid error");
        exit(1);
    }
    printf("[%d] termainated with exit code %d\n", c_pid, status);

    free(clone_stack);
    return 0;
}

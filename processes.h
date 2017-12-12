#include <stdio.h> // IO
#include <stdlib.h> // exit perror
#include <stdint.h> // multiple int tyoes

#ifndef __USE_GNU // set _GNU_SOURCE instead at some cases
#define __USE_GNU
#endif
#include <sched.h>
#include <sys/wait.h> // waitfor

#define SUBP_STACKSIZE 4096
#define NUMP 2 // max 16-1 due to bitmap_find function + init process

const uint16_t pow2[16] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768};

struct init_p_arg {
    pid_t ppid;
    int sub_namespace;
};

//uint16_t bitmap_find(const uint16_t);
int child_p(void *);
int init_p(void *);



/*  test was successful, while not necessary in this case
uint16_t bitmap_find(const uint16_t map){
    // return the location of least significant bit where bit != 0 in a given bitmap
    // bitmap_find will return 0 on map == 0
    uint16_t numbit=UINT16_MAX, mask = 0, st = 0;

    if ( (map % 2) == 1) return 0;
    for (size_t i = 8; i > 0; i/=2) {
        numbit >>= i;
        mask = numbit << st;
        if (mask & map) {
            continue;
        }else {
            st += i;
        }
    }
    return st;
}
*/

int init_p(void *arg){
    pid_t child[NUMP+1], ppid;
    void *(*child_stack[NUMP+1]);
    uint16_t alive_p;

    struct init_p_arg argnext;

    // arguments
    ppid = ((struct init_p_arg *)arg)->ppid;
    argnext.sub_namespace = ((struct init_p_arg *)arg)->sub_namespace - 1;
    argnext.ppid = getpid();
    // stacks
    for (size_t i = 0; i < NUMP; i++) {
        if ( !(*(child_stack+i) = malloc(SUBP_STACKSIZE*sizeof(char))) ){
            perror("malloc error at init_p");
            exit(1);
        }
    }

    // clone child processes
    for (size_t i = 0; i < NUMP; i++) {
        if ( (child[i] = clone(&child_p, (void *)((char *)child_stack[i] + SUBP_STACKSIZE), SIGCHLD, arg)) == -1 ) {
            perror("clone error at init_p");
            exit(1);
        }else{
            printf("pid namespace of ppid [%d] created [%d]\n", ppid, child[i]);
            // alive_map |= pow2[i];
            alive_p++;
        }
    }

    // clone new pid if sub_namespace >= 0

    if (argnext.sub_namespace) {
        // stack
        if ( !(child_stack[NUMP] = malloc(STACKSIZE*sizeof(char))) ) {
            perror("malloc error at init_p, CLONE_NEWPID");
            exit(1);
        }
        // clone new pid
        if ( ( child[NUMP] = clone(&init_p,\
                                    (void *)( (char *)(child_stack[NUMP]) + STACKSIZE ),\
                                    CLONE_NEWPID | SIGCHLD, (void *)(&argnext) ) ) == -1 ) {
            perror("clone error at init_p");
        }else{
            printf("init proc in a pidnamespce of ppid [%d] created a init process [%d] \n", ppid, child[NUMP]);
            // alive_map |= pow2[NUMP];
            alive_p++;
        }

    }



    // init process pid = 1
    //sleep(5);
    int stat;
    pid_t waitfor, waiting;
    while (alive_p) {
        // alive_p = bitmap_find(alive_map);
        // waitfor = child[alive_p];
        // printf("waiting for [%d]\n", waitfor);
        if ( (waiting = waitpid(-1, &stat, WNOHANG)) < 1) {
            if (waiting == -1) {
                perror("waitpid error");
            }
        }else{
            printf("process [%d] has terminated with status code %d\n",waitfor, stat);
            // alive_map ^= pow2[alive_p];
            alive_p--;
        }
        //printf("%p\n",  alive_map);
        sleep(1);

    }

}


int child_p(void *arg){
    pid_t pid;
    pid = getpid();
    while (1) {
        printf("child process pid = [%d] in pid namespace of ppid [%d]\n", pid, ((struct init_p_arg *)arg)->ppid);
        sleep(3);
    }
    return 0;
}

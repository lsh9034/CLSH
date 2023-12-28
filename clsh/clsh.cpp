          

#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include<cstring>
#include "option_handle.h"
using namespace std;

opt_val opt;
int** to_child_pipes;
int** to_parent_pipes;
int** to_parent_err_pipes;
int fd_to_child_idx[1000];
int is_err_fd[1000];
int *child_pid;

void print_result(char* buf, int child_idx, int fd){
    string s = "";
    s+=opt.nodes[child_idx];
    s+=": ";
    s+=buf;
    write(fd, s.c_str(), s.size());
    // printf("%s: %s", opt.nodes[child_idx], buf);
    return;
}


void child(int child_idx) {
    close(to_child_pipes[child_idx][1]);
    close(to_parent_pipes[child_idx][0]);
    dup2(to_parent_pipes[child_idx][1], 1);  // 부모에게 쓰기
    dup2(to_parent_err_pipes[child_idx][1], 2);
    dup2(to_child_pipes[child_idx][0], 0);  // 부모가 쓴 걸 읽기

    if (execlp("ssh", "ssh", "-T", "-o", "LogLevel=quiet", "-l", "ubuntu", opt.nodes[child_idx], NULL) == -1) {
        perror("execlp");
        exit(EXIT_FAILURE);
    }
}


void communicate_once(int epoll_fd, struct epoll_event* events, char* command) {
    int receive_cnt = 0;
    int num_fds = 0;
    command = strcat(command, "\n");
    printf("send command: %s", command);
    for(int i=0; i<opt.node_count; i++){
        write(to_child_pipes[i][1], command, strlen(command)+1);
    }
    int timeout=0;
    while (1) {
        num_fds = epoll_wait(epoll_fd, events, 1, 500);
        if (num_fds == -1) {
            perror("epoll_wait");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < num_fds; i++) {
            if (events[i].events & EPOLLIN) {
                char buf[1000] = {
                    0,
                };
                read(events[i].data.fd, buf, sizeof(buf));
                int child_idx = fd_to_child_idx[events[i].data.fd];
                if(is_err_fd[events[i].data.fd]){
                    print_result(buf, child_idx, opt.err_fd);
                }
                else{
                    print_result(buf, child_idx,opt.out_fd);
                }
                //printf("%d\n", events[i].data.fd);
                //fflush(0);
                receive_cnt += 1;
            }
        }
        if (receive_cnt == opt.node_count || timeout>1){ 
            break;
        }
        timeout++;
    }
    return;
}
void communicate_one_child(int child_idx,int epoll_fd, struct epoll_event* events, char* command){
    int receive_cnt = 0;
    int num_fds = 0;
    command = strcat(command, "\n");
    printf("send command: %s", command);
    write(to_child_pipes[child_idx][1], command, strlen(command)+1);
    int timeout=0;
    num_fds = epoll_wait(epoll_fd, events, 1, 500);
    if (num_fds == -1) {
        perror("epoll_wait");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < num_fds; i++) {
        if (events[i].events & EPOLLIN) {
            char buf[1000] = {0,};
            read(events[i].data.fd, buf, sizeof(buf));
            int child_idx = fd_to_child_idx[events[i].data.fd];
            if (is_err_fd[events[i].data.fd]) {
                print_result(buf, child_idx, opt.err_fd);
            } else {
                print_result(buf, child_idx, opt.out_fd);
            }
        }
    }
    return;
}
void parent(int argc, char** argv) {
    for (int i = 0; i < opt.node_count; i++) {
        close(to_child_pipes[i][0]);
        close(to_parent_pipes[i][1]);
    }
    fflush(0);
    int epoll_fd, num_fds;
    struct epoll_event *events;
    events = (struct epoll_event*)malloc(sizeof(epoll_event) * opt.node_count);

    epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("epoll_create");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < opt.node_count; i++) {
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = to_parent_pipes[i][0];
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
            perror("epool_ctl");
            exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < opt.node_count; i++) {
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.fd = to_parent_err_pipes[i][0];
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, event.data.fd, &event) == -1) {
            perror("epool_ctl");
            exit(EXIT_FAILURE);
        }
    }

    if (opt.is_i) {
        printf("Enter 'quit' to leave this interactive mode\n");
        printf("Working with nodes: ");
        for(int i=0; i<opt.node_count-1; i++){
            printf("%s, ", opt.nodes[i]);
        }
        printf("%s\n", opt.nodes[opt.node_count-1]);
        char name[100]={0,};
        strcpy(name, "clsh");
        int child_idx=-1;

        char buf[300]={0,};
        while(1){
            printf("%s> ", name);
            memset(buf, 0, sizeof(char)*300);
            cin.getline(buf, 300);
            if(strcmp(buf, "quit")==0){
                break;
            }
            printf("----------------------------------------------------------------------------\n");
            int is_changed=0;
            for(int i=0; i<opt.node_count; i++){
                if(strcmp(buf, opt.nodes[i])==0){
                    child_idx=i;
                    is_changed=1;
                    strcpy(name, opt.nodes[i]);
                    break;
                }
            }
            if(strcmp(buf, "clsh")==0){
                child_idx=-1;
                is_changed=1;
                strcpy(name, "clsh");
            }
            if(is_changed)continue;
            if(child_idx!=-1){
                communicate_one_child(child_idx, epoll_fd, events, buf);
            }
            else if(buf[0]!='!'){
                communicate_once(epoll_fd, events, buf);
            }
            else{
                char *temp = buf+1;
                printf("LOCAL: ");
                fflush(0);
                system(temp);
            }
            printf("----------------------------------------------------------------------------\n");
        }
    } 
    else {
        get_command(argc, argv, &opt);
        communicate_once(epoll_fd, events, opt.command);
    }
    kill(0, SIGINT);
    return;
}
int main(int argc, char** argv) {
    setvbuf(stdin, NULL, _IOLBF, 0);
    setvbuf(stdout, NULL, _IOLBF, 0);
    
    opt = get_optval(argc, argv);

    to_child_pipes = make_2d<int>(opt.node_count, 2);
    to_parent_pipes = make_2d<int>(opt.node_count, 2);
    to_parent_err_pipes = make_2d<int>(opt.node_count, 2);
    for(int i=0; i<opt.node_count; i++){
        pipe(to_child_pipes[i]);
        pipe(to_parent_pipes[i]);
        pipe(to_parent_err_pipes[i]);
        for(int j=0; j<2; j++){
            fd_to_child_idx[to_child_pipes[i][j]]=i;
            fd_to_child_idx[to_parent_pipes[i][j]]=i;
            fd_to_child_idx[to_parent_err_pipes[i][j]]=i;
            is_err_fd[to_parent_err_pipes[i][j]]=1;
        }
    }

    int pid = 0;
    int i = 0;
    child_pid = (int*)malloc(sizeof(int)*opt.node_count);
    for (i; i < opt.node_count; i++) {
        pid = fork();
        if (pid == 0) break;
        child_pid[i]=pid;
    }
               
    if (pid == 0) {
        child(i);
    } else if (pid > 0) {
        parent(argc, argv);
    }
    return 0;
}
               
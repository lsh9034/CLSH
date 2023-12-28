#define _GNU_SOURCE
#include<stdio.h>
#include<getopt.h>
#include<stdlib.h>
#include<string.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<mqueue.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<iostream>
#include"utils.h"

void read_file(char *file_name, char ***nodes,int *rows){
    FILE *f = fopen(file_name, "r");
    char* line;
    size_t len=0;
    ssize_t read;
    
    while((read = getline(&line, &len, f))!=-1){
        line[strlen(line)-1]=0;
        appendToStringArray(nodes, rows, line);
    }
    return;
}


extern char *optarg;
extern int optind;

class opt_val{
public:
    char **nodes=NULL;
    int node_count=0;
    int is_i=0;
    char* command=NULL;

    int out_fd = 1;
    int err_fd = 2;
};

opt_val get_optval(int argc, char** argv){
    int n;
    opt_val opt;
    char ***nodes = &opt.nodes;
    int *node_count = &opt.node_count;
    int *is_i = &opt.is_i;
    int option_index=0;
    struct option long_options[] ={
        {"hostfile", required_argument, 0,0},
        {"interactive", no_argument, 0, 'i'},
        {"out", required_argument, 0, 1},
        {"err", required_argument, 0, 2},
        {0,0,0,0},
    };
    while((n = getopt_long(argc, argv, "ih:", long_options, &option_index)) != -1){
        if(n=='h'){
            *nodes = split(optarg, ",",  node_count);
        }
        else if(n==0){
            read_file(optarg, nodes, node_count);
        }
        else if(n==1){
            FILE *f = fopen(optarg, "w+");
            opt.out_fd = f->_fileno;
        }
        else if(n==2){
            FILE *f = fopen(optarg, "w+");
            opt.err_fd = f->_fileno;
        }
        else if(n=='i'){
            *is_i=1;
        }
    }
    if(*nodes==NULL){
        char *input = getenv("CLSH_HOSTS");
        if(input!=NULL){
            *nodes = split(input, ":", node_count);
        } 
        else {
            input = getenv("CLSH_HOSTFILE");
            if (input != NULL) {
                read_file(input, nodes, node_count);
            }
            else{
                struct stat buf;
                int exist = stat(".hostfile", &buf);
                if(exist==0){
                    read_file(".hostfile", nodes, node_count);
                }
                else{
                    printf("--hostfile 옵션이 제공되지 않았습니다.\n");
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    return opt;
}

char* get_command(int argc, char**argv, opt_val *opt=NULL){
    char *command;
    int command_length=0;
    for(int i=optind; i<argc; i++){
        command_length+=strlen(argv[i])+1;
    }
    command = (char*)malloc(command_length*sizeof(char));
    strcpy(command, "");
    for(int i=optind; i<argc; i++){
        strcat(command, argv[i]);
        if(i!=argc-1){
            strcat(command, " ");
        }
    }
    if(opt!=NULL)opt->command=command;
    return command;
}
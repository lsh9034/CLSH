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

#define MAX_BUFSIZE 10000
struct msgbuf {
    long mtype;
    char text[MAX_BUFSIZE];
};

void sendall(mqd_t mq, char* msg, int count, int *nodes_pid){
    for(int i=0; i<count; i++){
        printf("send to %d %d\n", nodes_pid[i], count);
        if(mq_send(mq, msg, strlen(msg)+1, (void*)NULL)==-1){
            perror("mq_send");
            mq_unlink("/m.mq");
            exit(EXIT_FAILURE);
        }
    }
}

void read_file(char *file_name, char ***nodes,int *rows){
    FILE *f = fopen(file_name, "r");
    char* line;
    size_t len=0;
    ssize_t read;
    
    while((read = getline(&line, &len, f))!=-1){
        // printf("%s", line);
        appendToStringArray(nodes, rows, line);
    }
    return;
}

int main(int argc, char *argv[])
{
    int n;
    extern char *optarg;
    extern int optind;

    char **nodes=NULL;
    int node_count=0;

    struct option long_options[] ={
        {"hostfile", required_argument, 0}
    };
    int option_index=0;
    while((n = getopt_long(argc, argv, "h:", long_options, &option_index)) != -1){
        if(n=='h'){
            nodes = split(optarg, ",",  &node_count);
        }
        else if(n==0){
            printf("%s %s",long_options[option_index].name, optarg);
            fflush(0);
            read_file(optarg, &nodes, &node_count);
        }
        else{
            printf("else %s", optarg);
        }
    }
    for(int i=0; i<node_count; i++){
        printf("%s",nodes[i]);
    }
    // char *command;
    // int command_length=0;
    // for(int i=optind; i<argc; i++){
    //     command_length+=strlen(argv[i])+1;
    // }
    // command = (char*)malloc(command_length*sizeof(char));
    // strcpy(command, "");
    // for(int i=optind; i<argc; i++){
    //     strcat(command, argv[i]);
    //     printf("%s\n", command);
    //     if(i!=argc-1){
    //         strcat(command, " ");
    //     }
    // }
    // mqd_t mq;
    // struct mq_attr mq_attrib = {.mq_maxmsg=10, .mq_msgsize=8192, .mq_flags=O_NONBLOCK};
    // mq = mq_open("/m.mq", O_RDWR|O_CREAT|O_NONBLOCK, 0666, &mq_attrib); //포크 하기 전 미리 메세지큐 생성
    // if(mq==-1){
    //     perror("mq_open");
    //     exit(EXIT_FAILURE);
    // }

    // int *nodes_pid = malloc(sizeof(int)*node_count);
    // int pid;
    // int i=0;
    // for(i; i<node_count; i++){
    //     pid = fork();
    //     if(pid==0)break;
    //     nodes_pid[i]=pid;
    // }
    // if(pid==0){ //exec to remote_ssh
    //     printf("Launch remote_ssh with %s", nodes[i]);
    //     execlp("./remote_ssh", "./remote_ssh", nodes[i], NULL);
    // }
    // else if(pid>0){
    //     sendall(mq, command, node_count, nodes_pid);
    // }
    // printf("1");
    // while(1);
    // mq_unlink("/m.mq");
    return 0;
}
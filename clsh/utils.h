#include<stdio.h>
#include<getopt.h>
#include<stdlib.h>
#include<string.h>
#include<sys/epoll.h>
#include<unistd.h>
#include<mqueue.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<string>

char** split(char* str, const char* delimiter, int* count) {
    int i, j, len;
    char* token;
    char** result = NULL;

    // 구분자로 문자열을 분리한 후, 문자열 개수(count)를 구합니다.
    token = strtok(str, delimiter);
    while (token != NULL) {
        (*count)++;
        result = (char**)realloc(result, (*count) * sizeof(char*));
        result[(*count) - 1] = token;
        token = strtok(NULL, delimiter);
    }

    // 문자열 개수(count)만큼의 문자열 배열을 동적으로 할당합니다.
    result = (char**)realloc(result, (*count) * sizeof(char*));

    // 문자열 배열에 분리된 문자열을 복사합니다.
    len = strlen(str);
    j = 0;
    for (i = 0; i < (*count); i++) {
        len -= strlen(result[i]) + strlen(delimiter);
        strncpy(result[i], str + j, strlen(result[i]));
        j += strlen(result[i]) + strlen(delimiter);
        result[i][strlen(result[i])] = '\0';
    }

    return result;
}

void append(char *str1, char* str2) {
    char * new_str ;
    if((new_str = (char*)malloc(strlen(str1)+strlen(str2)+1)) != NULL){
        new_str[0] = '\0';   // ensures the memory is an empty string
        strcat(new_str,str1);
        strcat(new_str,str2);
    }
}

void appendToStringArray(char ***arr, int *size, char *newString) {
    *size = *size + 1;
    *arr = (char**)realloc(*arr, (*size) * sizeof(char*));
    if (*arr == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    (*arr)[*size - 1] = strdup(newString);
    if ((*arr)[*size - 1] == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
}
template<typename T>
T** make_2d(int r, int c){
    T** arr = (T**)malloc(sizeof(T*)*r);
    for(int i=0; i<r; i++){
        arr[i] = (T*)malloc(sizeof(T)*c);
    }
    return arr;
}
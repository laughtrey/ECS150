#include<iostream>
#include<cstring>
#include<stdlib.h>
#include<string>
#include<unistd.h>
#include<sys/types.h>
#include<stdio.h>
#include<fcntl.h>
#include<sys/wait.h>

//      Global Variables:
int BSIZE = 4096;
char error_message[30] = "An error has occurred\n";
bool batch_mode,menu;
// std::string path = "/bin/";
char *path;
char char_multi_path[512][512];
int custom_path,empty_path,multi_path = 0;
int CLOSED = 0;


//      Functions:

//      Prints error message and exits with error code 1: 
void errorMessage(){
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
}
//      Checks if there is only whitespace in the buffer
int checkWhiteSpace(char *buffer){
    int flag = 0;
    for(size_t i = 0; i < strlen(buffer); i++){
        if(isspace(buffer[i]) == 0){
            flag = 1;
            break;
        }
    }
    return flag;
}

int createFork(char *my_args[]){
    int ret = fork();
    if(ret < 0){//fork checking
        errorMessage();
    }
    else if (ret == 0 && empty_path !=1){
        if(custom_path == 0){
            path = strdup("/bin/"); // "/bin/(command)"
            path = strcat(path, my_args[0]);
            if(access(path,X_OK) != 0 && custom_path == 0){
                path = strdup("/usr/bin/");  //try the other default path for linux programs
                path = strcat(path, my_args[0]);
                if(access(path,X_OK) != 0){
                    errorMessage();
                }
            }
        }
        else if(custom_path == 1 && multi_path == 0){
            path = strcat(path, my_args[0]);
            if(access(path,X_OK) != 0){
                errorMessage();
            }
        }
        if(execv(path, my_args) == -1){
            errorMessage();
        }
    }
    // else{
    //      int return_status = 0;
    // }
    return ret;
}

int preProcess(char *buffer){
    int stdout_copy = 0;
    int ret;
    if(strstr(buffer, ">") != NULL){
        int n = 0;

        char *redirect[sizeof(char) * 512];
        redirect[0]= strtok(strdup(buffer), " \n\t>"); //splitting input by whitespace
        while(redirect[n] != NULL){
            n++;
            redirect[n] = strtok(NULL, " \n\t>");
        }
        if (n == 1){
            errorMessage();
        }
        int i = 0;
        char *my_args[sizeof(buffer)];
        my_args[0] = strtok(buffer, "\n\t>");
        while (my_args[i] != NULL){
            i++;
            my_args[i] = strtok(NULL, " \n\t>");
        }
        if(i > 2){
            errorMessage();
        }
        int j = 0;
        char *token[sizeof(my_args[1])];
        token[0] = strtok(my_args[1], " \n\t");
        while (token[j] != NULL){
            j++;
            token[j] = strtok(NULL, " \n\t");
        }
        char *fout = strdup(token[0]);
        stdout_copy = dup(1);
        int out = open(fout, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        int err = open(fout, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        fflush(stdout);
        dup2(out, STDOUT_FILENO);        
        dup2(out, STDOUT_FILENO);
        close(out);
        CLOSED = 1;
        if(i > 1 || j > 1 || out == -1 || err == -1){
            errorMessage();
        }
        my_args[i + 1] = NULL;
        token[j + 1] = NULL;
        strcpy(buffer, my_args[0]);
    }

    if(buffer[0] != '\0' && buffer[0] !='\n'){
        char *command[sizeof(buffer)];
        command[0] = strtok(buffer, " \t\n");
        int p = 0;
        while (command[p] != NULL){
            p++;
            command[p] = strtok(NULL, " \n\t");
        }
        command[p+1] = NULL;
        if(strcmp(command[0], "cd") == 0){
            //cd
            if(p == 2){
                if(chdir(command[1]) !=0){
                    errorMessage();
                }
            }
            else{
                errorMessage();
            }
        }
        else if(strcmp(command[0], "path") == 0){
            custom_path = 1;
            if (p == 2){
                empty_path = 0;
                path = strdup(command[1]);
                if (path[strlen(path) - 1] != '/'){
                    strcat(path, "/");
                }
            }
            else if(p == 1){
                empty_path = 1;
            }
            else{
                empty_path = 0;
                for(int i = 1; i < p; i++){
                    char *temp = strdup(command[i]);
                    if(temp[strlen(temp) - 1] != '/'){
                        strcat(temp, "/");
                    }
                    strcpy(char_multi_path[i-1], temp);
                    multi_path++;
                }
            }
        }
        else if(strcmp(command[0], "exit") == 0){
            if(p == 1){exit(0);}
            else{errorMessage();}
        }
        else{
            if(empty_path == 1){errorMessage();}
            else{ret = createFork(command);}
        }
    }
    if(CLOSED == 1){
        dup2(stdout_copy, 1);
        close(stdout_copy);
    }
    return ret;

}

int main(int argc, char *argv[]){

    FILE *file = NULL;
    path = (char*)malloc(BSIZE);
    char buffer[BSIZE];

    if(argc == 1){
        file = stdin;
        std::cout << "wish> ";
    }
    else if(argc == 2){
        char *bFile = strdup(argv[1]);
        file = fopen(bFile, "r");
        if(file == NULL){
            errorMessage();
        }
        batch_mode = true;
    }
    else{
        errorMessage();
    }

    while(fgets(buffer,BSIZE,file)){
        CLOSED = 0;
        if(checkWhiteSpace(buffer) == 0){
            continue;
        }
        if(strstr(buffer,"&") != NULL){
            int j = 0;
            char *my_args[sizeof(buffer)];
            my_args[0] = strtok(buffer, "\n\t&");
            while(my_args[j] != NULL){
                j++;
                my_args[j] = strtok(NULL, "\n\t&");
            }
            my_args[j+1]=NULL;
            int pid[j];
            for(int i = 0; i < j; i++){
                pid[i] = preProcess(my_args[i]);
                for (int n = 0; n < j; i++){
                    int return_status = 0;
                    waitpid(pid[n], &return_status, 0);
                    if(return_status == 1){
                        errorMessage();
                    }
                }
            }
        }
        else{
            preProcess(buffer);
        }
        if(argc == 1){
            std::cout << "wish> ";
        }
    }








/*
    const int SIZE = argc;
    std::string input;
    //int ret = fork();   //when i create new process


    if(argc > 1){
        batch_mode = true;
        menu = false;
    }
    else{
        batch_mode = false;
        menu = true;
    }


    while(menu){
        write(STDOUT_FILENO, "wish> ", 6);
        std::getline(std::cin,input);
        //TODO: Parse input. First command should be input_first_command, second command is args, etc.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  


        if(input == "exit"){
            exit(0);
            //menu = false;
        }
        else if (input == "ls")
        {
            pid_t ret = fork();
            char *args[3];
            args[0] = strdup("ls");
            args[1] = strdup("-a");
            execvp(args[0], args);
            // if(access("/bin/ls", X_OK) == 0){
            //     execvp(args[0], args);
            // }
        }
        
    }

*/
    return 0;
}
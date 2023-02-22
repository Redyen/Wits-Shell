#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

//Functions

void witsshell(); //run the main shell
int batchmode(char *MainArgv); //run the shell in batch mode9
void currentCom(char* command);//store the currently used command
void executeCom(char* command[]);//pass in current command and process [0] is command name
int exec(char* command[]);//this computes some functions by itself. can be useful
void executeLS(char* command[]);
void executeCD(char* command[]);
void executePath(char* command[]);
bool scanDirectory(char* directory, char* name);
void addPaths(char* command[]);
void executeFile(char* exeName);
bool containRedir(char* command[]);
void redirect(char* command[]);
bool containPara(char* command[]);
void parallel(char* command[]);

//=================================================================================

//variables

#define PATH "/bin/"
bool pathSet = true;
char* currCommand[10]; //this hold current command being processed. [0] is command name

extern char** environ;

int batchI = 0; //stores the amount of batch commands
char* batchList[15]; //stores a lsit of the batch commands

int newI = 0; //stores the amount of new commands
char* newList[15]; //stores a lsit of the new commands

char* currPath[15];
int numPaths = 1;

bool exitToken = false;
char* lastDir;

bool canexecute = false;

int numCom;//this stores the amount of calues in a single line

extern int errno ;

//=================================================================================

int main(int MainArgc, char *MainArgv[]){

    currPath[0] = PATH;
    if(MainArgc <= 1){
        witsshell();
    }
    else if(MainArgc == 2){
        if(batchmode(MainArgv[1]) == 1){
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                return 1;
           }
    }
    else{
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 1;
    }

	return(0);
}

void witsshell(){

    char *buffer;
    size_t buffsize = 32;
    size_t input;
    while(!exitToken){
        buffer = (char *)malloc(buffsize * sizeof(char));
        printf("witsshell>");

        if (feof(stdin)) exit(0);
        int c = getc(stdin);
        if (c == EOF) exit(0);
        ungetc(c, stdin);

        if (buffer != NULL){
            input = getline(&buffer, &buffsize, stdin);
            buffer[strcspn(buffer, "\n")] = 0;
        }

        //buffer now stores the line of input
        currentCom(buffer);//pass the line fo input to the currCommand
    }
    exit(0);
}

int batchmode(char *MainArgv){
    if(access(MainArgv, X_OK) == 0){

    FILE   *textfile;
    char   line[1000];
    char   temp[50];
    //printf("%s", MainArgv);

    textfile = fopen(MainArgv, "r");
    if(textfile == NULL){
        printf("An error has occured");
    }
    while(fgets(line, 1000, textfile)){
        line[strcspn(line, "\n")] = 0;
        //printf("%s", line);
        batchList[batchI] = strdup(line);
        batchI++;
    }
    fclose(textfile);
    //int b = 0;
    //while(!exitToken || b != (batchI-1)){
        //printf("%d\n",batchI);
        //currentCom(batchList[b]);
        //b++;
    //}
    if(batchI == 0){
        return 1;
    }

    for(int b = 0; b < batchI; b++){
        currentCom(batchList[b]);
    }
    exit(0);
    return 0;
    }
    else{
        return 1;
    }

}

//====================================================================================================


void currentCom(char* command){ //pass the full line of a command and it will split it to its parts in the currCommand list

    memset(currCommand, 0, sizeof(currCommand)); //reset value of currCommand
    char *found;
    int i = 0; // this will store num of commands
    while((found = strsep(&command, " ")) != NULL){
        currCommand[i] = found;
        i++;
    }
    numCom = i; //num Com st

    executeCom(currCommand);

}

void executeCom(char* command[]){ //pass in the currCommand list
    //printf("%s", command[0]);
    if(strcasecmp(command[0], "exit") == 0){
        if(numCom != 1){
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        else{
            exitToken = true;
        }
    }
    else if(containPara(currCommand)){
        parallel(currCommand);
    }
    else if(containRedir(currCommand)){
        redirect(currCommand);
    }
    else if(strcasecmp(command[0], "echo") == 0){
        for(int j = 1; j < numCom; j++){
            printf("%s", command[j]);
        }
        printf("\n");
    }
    else if(strcasecmp(command[0], "ls") == 0){
        if (pathSet){
            executeLS(currCommand);
        }
        else{
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    else if(strcasecmp(command[0], "cd") == 0){
        executeCD(currCommand);
    }
    else if(strcasecmp(command[0], "path") == 0){
        addPaths(currCommand);
    }
    else{
        if(numCom == 1 && canexecute){
            executeFile(command[0]);
        }
        else if(strcasecmp(command[0], "") != 0){
          char error_message[30] = "An error has occurred\n";//////////////////////////////////////////////////////////////
          write(STDERR_FILENO, error_message, strlen(error_message));
        }



    }
}

void executeLS(char* command[]){
    if(numCom>1){
            int Accesisble;
            Accesisble = access(command[1], F_OK);

            if(Accesisble!=0){
                char error_message[100] = "ls: cannot access '";
                strcat(error_message, command[1]);
                strcat(error_message, "': No such file or directory\n");
                write(STDERR_FILENO, error_message, strlen(error_message));
            }else{
                exec(command);
            }
        }else { //if(fork()==0) if you dont wanna close witsshell after ls
            exec(command);
        }
}

void executeCD(char* command[]){

    if(numCom != 2){
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(0);
    }
    else{
      int Accesisble = access(command[1], F_OK);

      if(Accesisble == -1){
            printf("%s \n", command[1]);
      }

      if(Accesisble != 0){
        char error_message[30] = "An error has occurred\n"; //////////////////////////////////////////////////////////
        write(STDERR_FILENO, error_message, strlen(error_message));
      }
      else{
        if(chdir(command[1]) != 0){
            char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));

        }
      }
    }

}

bool scanDirectory(char* directory, char* name){ //returns whether the file exists in the directory

    DIR *folder;
    struct dirent *entry;
    int files = 0;

    folder = opendir(directory);
    if(folder == NULL)
    {
        char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        //return false;
    }

    while( (entry=readdir(folder)) )
    {
        files++;
        if(strcmp(entry->d_name, name) == 0){
            return true;
        }

    }
    closedir(folder);
    return false;

}

void addPaths(char* commands[]){
    //pathSet = true;
    if(numCom <= 1){
        pathSet = false;
        memset(currPath, 0, sizeof(currPath));
        //printf("error");
    }
    else{
        pathSet = true;
        canexecute = true;
        currPath[0] = "/bin/";
        for(int j = 1; j<numCom; j++){
            currPath[numPaths] = strdup(commands[j]);
            numPaths++;
        }
    }
    //printf("%s\n", currPath[0]);
}

void executeFile(char* exeName){
        if(!pathSet){
            //printf("path not set");
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            //exit(0);
        }
        else{

        bool found = false;
        int pathNum = 0;
        for(int j = 0; j < numPaths; j++){
            if(scanDirectory(currPath[j], exeName)){
                found = true;
                pathNum = j;
            }
        }

    if(found){
        //printf("File Found");
        if (fork() == 0){
            char* tempPath = strcat(currPath[pathNum], exeName);
            char executable[50] = "";
            char *args[10] = {"sh", tempPath};
            execv("/bin/sh", args);
            exit(0);
        }
        else{
            wait(NULL);
        }


    }
    else{
       char error_message[30] = "An error has occurred\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    }
}
bool containRedir(char * commands[]){
 //return true if has  > in it. no otherwise
    for(int j = 0; j < numCom; j++){
        if(strcmp(commands[j], ">") == 0){
            return true;
        }
    }
    return false;

}

void redirect(char* commands[]){
    for(int j = 0; j < numCom; j++){
        if(strcmp(commands[j], ">") == 0){
            if(j+1 == numCom || j<numCom -1 || j==0){
                char error_message[30] = "An error has occurred\n";
                write(STDERR_FILENO, error_message, strlen(error_message));
                break;
            }
        }
    }
}

bool containPara(char * commands[]){
 //return true if has  > in it. no otherwise
    for(int j = 0; j < numCom; j++){
        if(strcmp(commands[j], "&") == 0){
            return true;
        }
    }
    return false;

}

void parallel(char* commands[]){
    for(int j = 0; j < numCom; j++){
        if(strcmp(commands[j], "&") == 0){
            if(j+1 == numCom){
                break;
            }
        }
    }
}

int exec(char* command[]){
    if (fork() == 0){
        char executable[50] = "";
        strcat(executable, currPath[0]); //     ie. "/bin/"
        strcat(executable, command[0]); //     ie. "/bin/ls"
        execv(executable, currCommand);
        exit(0);
    }
    else{
        wait(NULL);
    }

    //       execute the function
}

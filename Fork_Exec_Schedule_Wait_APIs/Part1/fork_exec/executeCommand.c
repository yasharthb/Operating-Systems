#include<stdio.h>
#include <string.h>
#include <unistd.h>
#include<stdlib.h>
#include<wait.h>
int executeCommand (char *cmd) {

    int status,argc = 0;
    pid_t pid, cpid;
    char *path,*paths = getenv("CS330_PATH");
    char path_cpy[100];
	  char *argv[100];

    char *arg = strtok(cmd, " ");
    while(arg != NULL && argc < 99){
        argv[argc++] = arg;
        arg = strtok(NULL, " ");
    }
    argv[argc] = 0;
   
    pid = fork();
    if(pid < 0){
         perror("fork");
         exit(-1);
    }
    if(!pid){ /*Child*/
        
        path = strtok(paths, ":");
        while( path != NULL ) {
            strcpy(path_cpy,path);
            strcat(path_cpy,"/");
            strcat(path_cpy,argv[0]);

            if(execv(path_cpy,argv)){ 
                path = strtok(NULL, ":");
                if(path == NULL)
                  exit(-1);
              }
      }
          
    }
   	cpid = wait(&status);    /*Wait for the child to finish*/

    if(pid){/*Parent*/
   	
    if(WEXITSTATUS(status)){
        printf("UNABLE TO EXECUTE\n");
        return -1;
      }
    }

    return WEXITSTATUS(status);
}

int main (int argc, char *argv[]) {
	return executeCommand(argv[1]);
}
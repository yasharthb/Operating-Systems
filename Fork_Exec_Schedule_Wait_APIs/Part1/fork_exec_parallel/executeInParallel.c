#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <wait.h>
#include <errno.h> 
#include <limits.h>

#define BUF_SIZE 	4096
#define STDOUT 		1
#define STDERR 		2
//extern int errno; 
int execute_in_parallel(char *infile, char *outfile)
{
   char buf[BUF_SIZE], pipeBuf[50][PIPE_BUF];
   char *line;
   int fdIn,fdOut,fd[50][2],count=0,pid,status;
   fdIn = open(infile, O_RDONLY);
   fdOut = open(outfile, O_CREAT | O_RDWR, 0666);
   dup2(fdOut,STDOUT);
   //dup2(fdOut,STDERR);
   while(read(fdIn,buf,BUF_SIZE) > 0){
   		line = strtok(buf, "\n");
   		while(line != NULL && count < 50){

   			if(pipe(fd[count])<0){
			      perror("pipe");
			      exit(-1);
			   }
   			pid = fork();
		      if(pid < 0){
		         perror("fork");
		         exit(-1);
		      }
		      if(!pid){ /*Child*/

   		    	close(fd[count][0]);
   		    	dup2(fd[count][1],STDOUT);

   		    	int argc=0;
   		      char *path,*paths = getenv("CS330_PATH");
   			   char path_cpy[100];
   		     	char *argv[100];
               char line_cpy[100];

               strcpy(line_cpy,line);
       			char *arg = strtok(line_cpy, " ");
   			   while(arg != NULL && argc < 99){
           		     argv[argc++] = arg;
           		     arg = strtok(NULL, " ");
       			}
      				argv[argc] = NULL;
   		        path = strtok(paths, ":");
   		        while( path != NULL ) {
   		            strcpy(path_cpy,path);
   		            strcat(path_cpy,"/");
   		            strcat(path_cpy,argv[0]);

   		            if(execv(path_cpy,argv)){ 
   		                path = strtok(NULL, ":");
   		                if(path == NULL){
   		                  printf("UNABLE TO EXECUTE\n" );
   		                  exit(-1);
   		                }
   		            }
   		      	}
		          
		      }

		    if(pid){
		    	close(fd[count][1]);
		    	count++;
   				line = strtok(NULL,"\n");
		    }

   	}
   }
   if(pid){
   		for(int i=0 ; i < count; i++){
   			
   			while(read(fd[i][0],pipeBuf[i],PIPE_BUF) > 0){

   				char *line_1 = strtok(pipeBuf[i], "\n");
   				while(line_1 != NULL){
   		   			printf("%s\n",line_1);
   					line_1 = strtok(NULL,"\n");
   				}
   			}
            close(fd[i][0]);
   		}
	}
}

int main(int argc, char *argv[])
{
	return execute_in_parallel(argv[1], argv[2]);
}

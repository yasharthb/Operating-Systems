#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <errno.h>

#define BUFSIZE 	8 
#define GO_MSG_SIZE 	3 
#define STDIN 		0
#define STDOUT 		1


#define TOTAL_MOVES	3

int main(int argc, char* argv[]) {
	
	char *goMsg = "GO";
	int pid_1=0, pid_2=0;
	char buf1[1],buf2[1];
	int fd[4][2];
	int count_1 = 0,count_2 = 0;

		if(pipe(fd[0])<0){
	      perror("pipe");
		  exit(-1);
		}

		if(pipe(fd[2])<0){
	       perror("pipe");
		  exit(-1);
		}

		pid_1 =fork();

        if(pid_1 < 0){
          perror("fork");
          exit(-1);
        }
        if(!pid_1){ /*Child 1*/
            close(fd[0][1]);  //Parent Write End
            close(fd[2][0]);  //Parent Read End

            dup2(fd[2][1],STDOUT);
   		    dup2(fd[0][0],STDIN);

   		    if(execv(argv[1],argv)){
   		    	perror("exec");
   		    	exit(-1);
   		    }
   		}

   		if(pid_1){
   			
   			if(pipe(fd[1])<0){
	         perror("pipe");
		     exit(-1);
		    }

		    if(pipe(fd[3])<0){
	          perror("pipe");
		      exit(-1);
		    }

		    pid_2 =fork();

	        if(pid_2 < 0){
	          perror("fork");
	          exit(-1);
	        }
	        if(!pid_2){ /*Child 2*/
	            pid_1 = 0;

	            close(fd[1][1]); //Parent Write End
	            close(fd[3][0]); //Parent Read End

                dup2(fd[3][1],STDOUT);
	   		    dup2(fd[1][0],STDIN);

	   		    if(execv(argv[2],argv+2)){
   		    	perror("exec");
   		    	exit(-1);
   		    }
	   		}
   		}
   		if(pid_1){

   			close(fd[0][0]); //Child process STDIN
   			close(fd[1][0]);

   		    close(fd[2][1]); //Child process STDOUT
	        close(fd[3][1]);


   			for(int i=0;i<10;i++){
   		     	
   		     	write(fd[0][1],goMsg,GO_MSG_SIZE);
   			    write(fd[1][1],goMsg,GO_MSG_SIZE);

   			    read(fd[2][0],buf1,1);
   			    read(fd[3][0],buf2,1);
   			    
   			    if(!(buf1[0]==buf2[0])){

   			    	if(buf1[0]=='0'  && buf2[0]=='1')
   			    		count_2++;
   			    	if(buf1[0]=='0' && buf2[0] == '2')
   			    	    count_1++;

   			    	if(buf1[0]=='1'  && buf2[0]=='2')
   			    		count_2++;
   			    	if(buf1[0]=='1' && buf2[0] == '0')
   			    	    count_1++;
   			    	
   			    	if(buf1[0]=='2'  && buf2[0]=='0')
   			    		count_2++;
   			    	if(buf1[0]=='2' && buf2[0] == '1')
   			    	    count_1++;
   			    }

   		    }
            close(fd[1][1]);
            close(fd[0][1]);

	    	printf("%d %d",count_1,count_2);
   		}
	
	return 0;
}
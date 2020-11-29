#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h> 
#include <wait.h>
#include <errno.h> 
#include <limits.h>

#define ROCK        0 
#define PAPER       1 
#define SCISSORS    2 

#define STDIN 		0
#define STDOUT 		1
#define STDERR		2
#define BUF_SIZE	4096
#define GO_MSG_SIZE 	3 

#include "gameUtils.h"

int getWalkOver(int numPlayers); // Returns a number between [1, numPlayers]

int main(int argc, char *argv[])
{   
	//printf("%d\n",argc);
	char *playersFile;
	char buf[128];
	char *goMsg = "GO";
	//char lineBuf[100][128];
	int fdPlayers;
    int N = 10, P, i=0, line_count = 0, pid = 0,tpid=0;
    int active_count;
    int winner,matches=-1;


    char **lineBuf = (char **)malloc(100 * sizeof(char *)); 
    for (int i=0; i<100; i++) 
         lineBuf[i] = (char *)malloc(128 * sizeof(char));

    if(argc < 2)
    	return -1;

    if(argv[1][0]== '-')
    {
    	if(argv[1][1]== 'r')
    	{	N = atoi(argv[2]);
    		playersFile = argv[3];
        }
    	else
    		return -1;
    }
    else{
    	playersFile = argv[1];
    }

    fdPlayers = open(playersFile, O_RDONLY);

    while (read(fdPlayers, &buf[i], 1) == 1) {

	    if (buf[i] == '\n' || buf[i] == 0x0) {

	        buf[i] = 0;
	        lineBuf[line_count][i] = '\0';
	        line_count++;
	        i = 0;
	        continue;
	    }
        lineBuf[line_count][i] = buf[i];
	    i++;
	}
    
    // for(int l =0;l<line_count;l++){
    // 	    printf("\n%s\n",lineBuf[l]);
    // }

    P = atoi(lineBuf[0]);
    active_count=P;
    //printf("%d\n",P);
    char *args[2]; 
    args[1] = NULL;
    

    int **fd1 = (int **)malloc(P * sizeof(int *)); 
    for (int i=0; i<P; i++) 
         fd1[i] = (int *)malloc(2 * sizeof(int)); 

     int **fd2 = (int **)malloc(P * sizeof(int *)); 
    for (int i=0; i<P; i++) 
         fd2[i] = (int *)malloc(2 * sizeof(int));

    int alpha= (int)(P*(P+1)/2.0);
    int **fd3 = (int **)malloc( alpha * sizeof(int *)); 
    for (int i=0; i<alpha; i++) 
         fd3[i] = (int *)malloc(2 * sizeof(int)); 

        		int *active =(int *)malloc(P * sizeof(int));
   		for(int j=0;j<P;j++)
   			active[j]= 1;

 

    for(i=0;i<P;i++)
    {
    	if(pipe(fd1[i])<0){
	      perror("pipe");
		  exit(-1);
		}

		if(pipe(fd2[i])<0){
	       perror("pipe");
		  exit(-1);
		}

		pid=fork();

        if(pid < 0){
          perror("fork");
          exit(-1);
        }

        if(!pid){  /*Child i*/
          	break;
		}

        if(pid){
        	close(fd1[i][0]);
        	close(fd2[i][1]);
        }
    }

    if(!pid){ /*Child i*/

            close(fd1[i][1]);  //Parent Write End
            close(fd2[i][0]);  //Parent Read End

   		    dup2(fd1[i][0],STDIN);
   		    dup2(fd2[i][1],STDOUT);
            
            args[0] = lineBuf[i+1];
   		    if(execv(lineBuf[i+1],args)){
   		    	perror("exec");
   		    	exit(-1);
   		    }
   		}


 if(pid){

	while(active_count >1){
		
		int dummy_count=0;
			for(int j=0;j<P;j++){
	   			if(active[j]>0)
	   			{   
	   				active[j]=1;
	   				dummy_count++;
	   				if(dummy_count==active_count)
	   					printf("p%d\n",j);
	   				else
	   					printf("p%d ",j);

	   			}
	   		}

	   	    fflush(stdout);

	   		
	   		if(active_count%2 == 1)
	   		{	int index = getWalkOver(active_count);
	   			int count = 0;
	   			for(int i=0;i<P;i++)
	   			{
	   				if(active[i]==1)
	   					count++;
	   				if(count==index)
	   			        active[i] = 2;
	   			}
			}

	  // for(int j=0;j<P;j++){

	  //  			printf("%d ",active[j]);
	  //  		}
	  //  		printf("\n");

	        int pair=0,x[2];

			for(int i=0;i<P;i++){
	           if(active[i]==1)
	             x[pair++]=i;

		           	if(pair==2){

		           		
		           		pair = 0;
		           		matches++;
						if(pipe(fd3[matches])<0){
							      perror("pipe");
								  exit(-1);
								}

		           		  tpid =fork();
					           		
      					  if(tpid < 0){
					          perror("fork");
          					  exit(-1);
        					}

						if(!tpid){	

							close(fd3[matches][0]);

							//dup2(fd3[matches][1],STDOUT);

				            char buf1[1],buf2[1];
				            int count_1 = 0,count_2 = 0;

			           		for(int j=0;j<N;j++){
			   		     	
				   		     	write(fd1[x[0]][1],goMsg,GO_MSG_SIZE);
				   			    write(fd1[x[1]][1],goMsg,GO_MSG_SIZE);

				   			    read(fd2[x[0]][0],buf1,1);
				   			    read(fd2[x[1]][0],buf2,1);
				   			    
				   			    if(!(buf1[0]==buf2[0])){

				   			    	if(buf1[0]=='0' && buf2[0]=='1')
				   			    		count_2++;
				   			    	if(buf1[0]=='0' && buf2[0] == '2')
				   			    	    count_1++;

				   			    	if(buf1[0]=='1' && buf2[0]=='2')
				   			    		count_2++;
				   			    	if(buf1[0]=='1' && buf2[0] == '0')
				   			    	    count_1++;
				   			    	
				   			    	if(buf1[0]=='2' && buf2[0]=='0')
				   			    		count_2++;
				   			    	if(buf1[0]=='2' && buf2[0] == '1')
				   			    	    count_1++;
				   			    }
							}
									if(count_1>=count_2)
									{	
								        //printf("%d",x[1]);
								        write(fd3[matches][1], &x[1], sizeof(x[1]));

									}
									else
									{  //printf("%d",x[0]);
									write(fd3[matches][1], &x[0], sizeof(x[0]));
							}

									exit(0);
									break;
						}
						
						if(tpid){
							close(fd3[matches][1]);
							//char inbuf[4];
							//printf("%d\n",fd3[matches][1]);
							int looser;
							read(fd3[matches][0],&looser,sizeof(looser));
							 // for(int l =0 ; l<8; l++)
							 // 	printf("%c\n",inbuf[l]);
							//printf("%s\n",inbuf);
							//printf("%d\n",looser);
					    	active_count--;
							active[looser] = 0;
							if(active_count==1)
							{
								if(x[0]==looser)
									winner = x[1];
								else
									winner = x[0];
							}
							close(fd3[matches][0]);
					}

					if(!tpid)
					break;
			    }
		    }

		   	 if(!tpid)
				break;

		// for(int j=0;j<P;j++){

	 //   			printf("%d ",active[j]);
	 //   		}
		   
		}

			

		printf("p%d",winner);
   	}
    return 0;
}

/*
Duisembayev Dias
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <pthread.h>

#define	QLEN			5
#define	BUFSIZE			4096
int clientCounter=0;
int numberOfPlayers=0;	
int waitForAnswer;
int firstAnswer=1;
int isFull=1;
int answered=0;
int finishQuiz=0;
int exitThread = 0;
pthread_mutex_t mutex;
char** player;

char answer[2048];
int score[1011];
char question[2048];
int sockets[1011];
FILE *fp;

int passivesock( char *service, char *protocol, int qlen, int *rport );
// THIS FUNCTION CLOSES GIVEN SOCKET AND DECREMENETS NECCESSERY VARIABLES
void killClient(int ssock){
				printf( "The client has gone.\n" );
				fflush(stdout);
				close(ssock);
				pthread_mutex_lock(&mutex);
				numberOfPlayers--;
				clientCounter--;
				pthread_mutex_unlock(&mutex);


}
//THIS FUNCTION CLOSES ALL SOCKETS AND ALSO INITIALIZES NEEDED VARIABLES
void killAllClients(){
	int i=0;
	for (i=1;i<1011;i++){
		score[i]=0;
		player[i]=(char*)(malloc(sizeof(char)));
	 if (sockets[i]!=-1){
	 	printf("%i   socket: %i\n", i, sockets[i]);
	  killClient(sockets[i]);	
	  	sockets[i]=-1;
     }
    }

}


void *acceptClients(void *argv){
int			ssock=*(int*)argv;
int s;
	// THIS BLOCK OF CODE RESTRICTS NEW USER TO ENTER THE QUIZ IF IT IS STARTED
	if (isFull==0){
	printf("%i\n", ssock );
    close(ssock);
    			     	for(s=1;s<1011;s++){
						   if (sockets[s]!=-1){
							 if ( write( sockets[s], "", 0 ) < 0 ){
								sockets[s]=-1;
							}
						   }
						}
 	pthread_exit(NULL);
	}

	char			buf[BUFSIZE];
	int			cc;
	int i=0, firstTime=1, j=0, k=0;
	int playerId;
	int isAnswer=0;
    char* winner=(char*)malloc(sizeof(char));
    char* bufAnswer=(char*)malloc(sizeof(char));
    char* fulQuestion = (char*)malloc(sizeof(char));
    char standings[1015*2*100];
    char temp[10];
pthread_mutex_lock(&mutex);
clientCounter++;
playerId=clientCounter;
pthread_mutex_unlock(&mutex);
printf("ID, %i", playerId);
fflush(stdout);


	if(clientCounter==1){
		if ( write( ssock, "QS|ADMIN\r\n", 10 ) < 0 )
				{
					/*killClient(ssock);
								return NULL;*/
				}
		fflush( stdout );
	}
	
	if(clientCounter>1){	
		if ( write( ssock, "QS|JOIN\r\n", 9 ) < 0 )
				{
				    /*
					killClient(ssock);
								return NULL;*/
				}
		fflush( stdout );
	}
	
	for (;;)
	{
       



		for (;;)
		{
			if ( (cc = read( ssock, buf, BUFSIZE )) <= 0 )
			{
				
			}
			else
			{
                if (exitThread==1){
                	printf("exited\n");
 					pthread_exit(NULL);
 					printf("allo\n");

                }

				buf[cc-1] = '\0';
				if (clientCounter==1 && firstTime==1){
							if ( write( ssock, "WAIT\r\n", 6 ) < 0 )
				{

				}
					fflush( stdout );
					firstTime=0;
					char* tok=strtok(buf, "|");
					while(i<3 || tok!=NULL){
					 if (i==1){
					 	pthread_mutex_lock(&mutex);
					 	strcpy(player[playerId], tok);
					 	pthread_mutex_unlock(&mutex);
					 } else if (i==2){
					 	numberOfPlayers=atoi(tok);	
					   }
					  tok=strtok(NULL, "|");  
					  i++;
					}
					i=0;				
				} else  {
                char* tok=strtok(buf, "|");
                 	i=0;
					while(tok!=NULL){
					if (i==0){
						if (strcmp(tok, "ANS")==0){
							isAnswer=1;
						}

					}
					 if (i==1){
						if (isAnswer==0){	
						firstTime=0;
								if ( write( ssock, "WAIT\r\n", 6 ) < 0 )
								{

								}
								fflush( stdout );
						fflush(stdout);
						pthread_mutex_lock(&mutex);
						tok[strlen(tok)-1]='\0';
						strcpy(player[playerId], tok);
						pthread_mutex_unlock(&mutex);
						} else {
                  	    strcpy(bufAnswer, tok );
						}
					 
					 } 
					  tok=strtok(NULL, "|");  
					  i++;
					}
					i=0;
				 }
				 bufAnswer[strlen(bufAnswer)-1]='\0';

				 //NEXT BLOCK OF CODE CHECKS USER'S ANSWERS AND GIVES POINTS ACCORDING TO THEIR ANSWERS
				if (waitForAnswer==1){
					pthread_mutex_lock(&mutex);
					waitForAnswer==0;
					pthread_mutex_unlock(&mutex);
                	if(strcmp(bufAnswer, answer)==0){
                		if (firstAnswer==1) {
                			score[playerId]+=2;
                			char realWinnner[10000];
                			strcpy(winner, player[playerId]);
                			strcpy(realWinnner, "WIN|");
                			strcat(realWinnner, winner);
                			strcat(realWinnner, "\r\n");
                				for(s=1;s<1011;s++){
                				  if (sockets[s]!=-1){
                					if ( write( sockets[s], realWinnner, strlen(realWinnner) ) < 0 )
									{
										sockets[s]=-1;
										killClient(sockets[s]);
										pthread_exit(NULL);
										//return NULL;
									}
								   }
							    }
                			fflush(stdout);
                			pthread_mutex_lock(&mutex);
                			answered++;
                			firstAnswer=0;
                			pthread_mutex_unlock(&mutex);
                		} else {
                			score[playerId]++;
                			pthread_mutex_lock(&mutex);
                			answered++;
                			pthread_mutex_unlock(&mutex);
                		}

                	}   else if (strcmp(bufAnswer, "NOANS")==0){
                			pthread_mutex_lock(&mutex);
                			answered++;
                			pthread_mutex_unlock(&mutex);

           				} else {
           					score[playerId]--;
           					pthread_mutex_lock(&mutex);
                			answered++;
                			pthread_mutex_unlock(&mutex);
           				}
           		 }
                pthread_mutex_lock(&mutex);

                //next block of code finishes the quiz, it closes all the sockets
                if ((finishQuiz==1 && answered==numberOfPlayers) || clientCounter==0){
                	strcpy(standings, "RESULT");
                	fflush(stdout);
                	k=0;
                	while(k<numberOfPlayers){
                	char tempo[128*3];
                	strcat(standings, "|");
                	strcat(standings, player[k+1]);
                	strcat(standings, "|");
                	sprintf(tempo, "%d", score[k+1] );
					strcat(standings, tempo);
                	k++;
                	}
                		for(s=1;s<1011;s++){
                		   if (sockets[s]!=-1){
                			if ( write( sockets[s], standings, strlen(standings) ) < 0 )
							{
								sockets[s]=-1;
								killClient(sockets[s]);
								pthread_exit(NULL);
							}
						   }
						}
					fflush( stdout );
					firstAnswer=1;
				    isFull=1;
				    finishQuiz=0;
				    fseek(fp, 0, SEEK_SET);
				    answered=0;
				    exitThread=1;
                	pthread_mutex_unlock(&mutex);
                	 killAllClients();
                }
                // WE NEED TO EXIT THE THREAD IF QUIZ IS FINISHED
                if (exitThread==1){
                	exitThread=0;
 					pthread_exit(NULL);
                }

                    pthread_mutex_unlock(&mutex);

                if (answered==numberOfPlayers){
                	pthread_mutex_lock(&mutex);
                	answered=0;
                	waitForAnswer=0;
                	pthread_mutex_unlock(&mutex);
                }

				// this block of code writes to all clients that quiz is about to start
				if(clientCounter!=0 && clientCounter==numberOfPlayers && isFull==1){
					pthread_mutex_lock(&mutex);
						for(s=1;s<1011;s++){
							score[s]=0;
						   if (sockets[s]!=-1){
							if ( write( sockets[s], "QS|FULL\r\n", 9 ) < 0 )
							{
								sockets[s]=-1;
								killClient(sockets[s]);
								pthread_exit(NULL);
								//return NULL;
							}
						   }
						}
					fflush(stdout);
               		isFull=0;
               		waitForAnswer=0;
          			pthread_mutex_unlock(&mutex);
				}

				// next block of code reads and prints questions by question, it also stores the answer for each quesion
				// we need to exeute this blcok of code when we do no wait for answer and do no want to finish the quiz
				if(clientCounter!=0 && clientCounter==numberOfPlayers  && waitForAnswer==0 && finishQuiz==0   ){
					j=0;
						pthread_mutex_lock(&mutex);
						while(strcmp(fgets(question, 2048, fp), "\n") ){
							if (j==0){
							strcpy(fulQuestion, question);	
							} else {
							strcat(fulQuestion, question);	
							}
							j++;
						}
						pthread_mutex_unlock(&mutex);
					j=strlen(fulQuestion);
					char realQuestion[1000*5*10];
					strcpy(realQuestion, "QUES|");
					char tempi [1000];
					sprintf(tempi, "%d", j);
					strcat(realQuestion, tempi);
					strcat(realQuestion, "|");
					strcat(realQuestion, fulQuestion);
					printf("wooot\n");
					for(s=1;s<1011;s++){
					   if (sockets[s]!=-1){
					   	printf("trying to write\n");
						if ( write( sockets[s], realQuestion, strlen(realQuestion) ) < 0 )
						{
							printf("CANNOT WRITE THE QUESTION\n");
								sockets[s]=-1;
								killClient(sockets[s]);
								pthread_exit(NULL);
								//return NULL;
						}
					   }
								fflush( stdout );
				    }
				    printf("left the loop\n");
					if (fgets(answer, sizeof(answer), fp)==NULL){
						printf("Wrong formatted file\n");
						exit(1);	
					}
					if (fgets(temp, sizeof(temp), fp)==NULL){
						pthread_mutex_lock(&mutex);
						finishQuiz=1;
						pthread_mutex_unlock(&mutex);
					}
					answer[(strlen(answer)-1)]='\0';
				 	firstAnswer=1; 	
				 	waitForAnswer=1;
                }
                				

			}
		}
	



	}






}


int
main( int argc, char *argv[] )
{
	
	int			rport = 0;
	int			msock;
	int k=0;
	char			*service;
	char			*fileName;
	pthread_t client;
	pthread_mutex_init(&mutex,NULL);
	player=(char**)malloc(sizeof(char*));
	for (k=0;k<1011;k++){
        score[k]=0;
        sockets[k]=-1;
     	 player[k]=(char*)(malloc(sizeof(char)));

	}

	switch (argc) 
	{

		case	2:
			// User provides a port? then use it
			fileName = argv[1];
						rport = 1;
			break;
		case 	3:
			fileName = argv[1];
			service = argv[2];
			break;
		default:
			fprintf( stderr, "usage: filename , server [port]\n" );
			exit(-1);
	}

	
	fp=fopen(fileName, "r");
	if (fp==NULL){
		perror("Error");
		exit(-1);
	}

	msock = passivesock( service, "tcp", QLEN, &rport );
	if (rport)
		{
		//	Tell the user the selected port
		printf( "server: port %d\n", rport );	
		fflush( stdout );
		}
k=1;

for(;;)
	{   
		
		int	ssock;
		struct sockaddr_in	fsin;
		int			alen;
		alen = sizeof(fsin);
		ssock = accept( msock, (struct sockaddr *)&fsin, &alen );
		if (ssock < 0)
		{
			fprintf( stderr, "accept: %s\n", strerror(errno) );
			exit(-1);
		}
	    sockets[k]=ssock;
	    printf("Creating a thread\n");
		if( pthread_create( &client , NULL ,  acceptClients , &ssock) )
			{
            	perror("could not create thread");
            	return 1;
			}
        k++;
      
	}

}



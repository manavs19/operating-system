#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MIN 0
#define MAX 1
#define BUFSIZE 10

/* The signal handler for the child process C, signal passes message of its victory or defeat */
void CSigHandler (int sig)
{
   if (sig == SIGUSR1)
   {
      printf("C says I have won\n");
   }
   else if (sig == SIGUSR2)
   {
      printf("C says I have lost\n");
   }
   exit(0);
}

/* The signal handler for the child process D, signal passes message of its victory or defeat */
void DSigHandler (int sig)
{
   if (sig == SIGUSR1)
   {
      printf("D says I have won\n");
   }
   else if (sig == SIGUSR2)
   {
      printf("D says I have lost\n");
   }
   exit(0);
}


/* Main function */
int main ()
{
   srand((unsigned int)time(NULL));  //seeding time
   int pidC, pidD;   //pid for child processes
   int fd1[2], fd2[2];   //for pipes

   char number1[10], number2[10];  //to pass numbers in string format

   if(pipe(fd1)==-1)   //if pipe could not be created
   {
      printf("Could not make pipe between P and C\n");
      exit(0);
   }
   if(pipe(fd2)==-1)  //if pipe could not be created
   {
      printf("Could not make pipe between P and D\n");
      exit(0);
   }
   

   if ((pidC=fork()) == 0)//Child C
   {
      srand(pidC);  //seeding
      //block fd2
      close(fd2[0]);
      close(fd2[1]);

      close(fd1[0]);//close read end

      signal(SIGUSR1, CSigHandler);           /* Register SIGUSR1 handler */
      signal(SIGUSR2, CSigHandler);           /* Register SIGUSR2 handler */
      
      while(1)
      {                   
         int n = rand()%100;		      //ranodom number generated to be passed        
         
         sprintf(number1, "%d", n);	      //writing to string
         write(fd1[1], number1, BUFSIZE);     //writing to pipe
         sleep(1);
      }
   }
   else if((pidD = fork()) == 0)//Child D
   {
      //block fd1
      close(fd1[0]);
      close(fd1[1]);

      close(fd2[0]);//close read end

      signal(SIGUSR1, DSigHandler);           /* Register SIGUSR1 handler */
      signal(SIGUSR2, DSigHandler);           /* Register SIGUSR2 handler */

      while(1)
      {
         int n = rand()%100;		      //ranodom number generated to be passed
         
         sprintf(number2, "%d", n);	      //writing to string
         write(fd2[1], number2, BUFSIZE);     //writing to pipe
         sleep(1);   
      }
   }
   else
   {
      close(fd1[1]);//close write end
      close(fd2[1]);//close write end

      int scoreC = 0, scoreD = 0, round = 1;

      //Loop for the matches between C and D, while one does not reach a score of 10
      while(scoreC!=10 && scoreD != 10)
      {
         int flag = rand()%2;    //random flag is selected
         printf("\nRound %d :\nChoice of flag is ",round);
         round++;
         if (flag==MIN)
         {
            printf("MIN\n");
         }
         else
         {
            printf("MAX\n");
         }

         read(fd1[0], number1, BUFSIZE);   //number from child C is read
         int numberC = atoi(number1);
         read(fd2[0], number2, BUFSIZE);   //number from child D is read
         int numberD = atoi(number2);

         printf("Number received from C is %d\n", numberC);
         printf("Number received from D is %d\n", numberD);

         if(flag == MIN)
         {
            if(numberC < numberD)
            {
               ++scoreC;
               printf("C gets the point\n");
            }
            else if(numberC > numberD)
            {
               ++scoreD;
               printf("D gets the point\n");
            }
         }
         else
         {
            if(numberC > numberD)
            {
               ++scoreC;
               printf("C gets the point\n");
            }
            else if(numberC < numberD)
            {
               ++scoreD;
               printf("D gets the point\n");
            }
         }

         printf("Score of C : %d\n", scoreC);  
         printf("Score of D : %d\n", scoreD);  
      }
      if(scoreC==10)
      {
         printf("\nP says C has won\n");
         kill(pidC, SIGUSR1);            //process C killed
         kill(pidD, SIGUSR2);		 //process D killed
      }
      else
      {
         printf("\nP says D has won\n");
         kill(pidD, SIGUSR1);            //process D killed
         kill(pidC, SIGUSR2);            //process C killed
      }

      exit(0);				 //exiting
   }
}
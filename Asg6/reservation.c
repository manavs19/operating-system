#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <time.h>   
#include <unistd.h>  
#include <pthread.h>

#define t 100         //Number of trains
#define c 500        //Capacity of each train
#define s 20         //Number of worker threads
#define MAX 5        //Max queries at any time
#define T (1*60)     //Total running time

#define INQUIRE 1
#define BOOK 2
#define CANCEL 3

#define SLEEPTIME 1

/* Data type for parameters to be passed to worker threads during start up */
typedef struct {
   int tno;
   char tname[5];
} tinfo;


int numActiveQueries = 0;//Number of active queries

//{train number, query type, thread number}
int activeQueries[MAX][3];//The currently active queries
int * trains;//trains[i] = number of seats available in train i


pthread_barrier_t barrier; // barrier synchronization object
/* mutex for mutually exclusive updating of the table activeQueries */
pthread_mutex_t csmutex;

/* mutex and condition variables for checking active queries less than MAX */
pthread_mutex_t querymutex;
pthread_cond_t querycond;

/* mutex and condition variables for winding up */
pthread_mutex_t donemutex;
pthread_cond_t donecond;
int mdone = 0, wdone = 0;


void inquiry(int no)
{
   int i, otherThread, otherQueryType;
   int trainNumber = rand()%t;

   pthread_mutex_lock(&csmutex);//activeQueries is shared table
   for(i=0;i<MAX;++i)
   {
      if(activeQueries[i][0]==trainNumber && activeQueries[i][1] != INQUIRE)//Write query on same train is active
      {         
         otherQueryType = activeQueries[i][1];
         otherThread = activeQueries[i][2];
         break;
      }
   }
   pthread_mutex_unlock(&csmutex);
   if(i != MAX)//Failed
   {      
      char otherQuery[20];
      if(otherQueryType == BOOK)
         strcpy(otherQuery, "booking");
      else
         strcpy(otherQuery, "cancellation");

      printf("\nThread %d : Inquire Query on train %d failed since thread %d is making %s query\n", no, trainNumber, otherThread, otherQuery);
      return;
   }

   //Fill an empty entry
   int entry;
   pthread_mutex_lock(&csmutex);//activeQueries is shared table
   for(i=0;i<MAX;++i)
   {
      if(activeQueries[i][0] == -1)//Empty entry
      {
         entry = i;
         activeQueries[i][0] = trainNumber;
         activeQueries[i][1] = INQUIRE;
         activeQueries[i][2] = no;
         break;
      }
   }
   pthread_mutex_unlock(&csmutex);

   printf("\nThread %d : Begin Inquiry about train %d\n", no, trainNumber);
   sleep(SLEEPTIME);
   printf("\nThread %d : Number of seats in train %d is %d\n", no, trainNumber, trains[trainNumber]);

   pthread_mutex_lock(&csmutex);
   if(entry >= MAX || entry <= -1*MAX)
      entry = rand()%MAX;
   activeQueries[entry][0] = -1;//Empty the corresponding entry
   pthread_mutex_unlock(&csmutex);
}

void booking(int no, int isBooked[], int trainsBooked[], int * numTrainsBooked)
{
   int i, otherThread, otherQueryType;
   int trainNumber = rand()%t;
         
   pthread_mutex_lock(&csmutex);//active Queries is shared table
   for(i=0;i<MAX;++i)
   {
      if(activeQueries[i][0]==trainNumber)//Another query on same train
      {         
         otherQueryType = activeQueries[i][1];
         otherThread = activeQueries[i][2];
         break;
      }
   }
   pthread_mutex_unlock(&csmutex);
   if(i != MAX)//Failed
   {      
      char otherQuery[20];
      if(otherQueryType == INQUIRE)
         strcpy(otherQuery, "inquire");
      else if(otherQueryType == BOOK)
         strcpy(otherQuery, "booking");
      else
         strcpy(otherQuery, "cancellation");

      printf("\nThread %d : Booking Query on train %d failed since thread %d is making %s query\n", no, trainNumber, otherThread, otherQuery);
      return;
   }

   //Fill an empty entry
   int entry;
   pthread_mutex_lock(&csmutex);//activeQueries is shared table
   for(i=0;i<MAX;++i)
   {
      if(activeQueries[i][0] == -1)//Empty entry
      {
         entry = i;
         activeQueries[i][0] = trainNumber;
         activeQueries[i][1] = BOOK;
         activeQueries[i][2] = no;
         break;
      }
   }
   pthread_mutex_unlock(&csmutex);

   int k = 5 + rand()%6;//Number of seats to book[5,10]
   if(k > trains[trainNumber])
   {
      printf("\nThread %d : %d tickets not available for train %d\n", no, k, trainNumber);
      return;
   }


   printf("\nThread %d : Begin Booking %d tickets in train %d\n", no, k, trainNumber);
   sleep(SLEEPTIME);

   //Modify data
   if(isBooked[trainNumber] == 0)//First booking
   {
      trainsBooked[*numTrainsBooked] = trainNumber;
      (*numTrainsBooked)++;
   }
   isBooked[trainNumber] += k;//k tickets more booked
   trains[trainNumber] -= k;//Reduce number of available tickets

   printf("\nThread %d : End Booking %d tickets in train %d\n", no, k, trainNumber);

   pthread_mutex_lock(&csmutex);
   if(entry >= MAX || entry <= -1*MAX)
      entry = rand()%MAX;
   activeQueries[entry][0] = -1;//Empty the corresponding entry
   pthread_mutex_unlock(&csmutex);
}

void cancellation(int no, int isBooked[], int trainsBooked[], int * numTrainsBooked)
{
   int i, otherThread, otherQueryType;
   if((*numTrainsBooked) == 0)//This thread has not yet booked any ticket
   {
      printf("\nThread %d : Cannot Cancel Ticket : Has not booked any tickets yet\n", no);
      return;
   }
   
   int ticketToCancel = rand()%(*numTrainsBooked);//Choose a train index to cancel
   int trainNumber = trainsBooked[ticketToCancel];
   while(isBooked[trainNumber] == 0)//All cancelled
   {
      ticketToCancel = rand()%(*numTrainsBooked);
      trainNumber = trainsBooked[ticketToCancel];
   }
   
   pthread_mutex_lock(&csmutex);//active Queries is shared table
   for(i=0;i<MAX;++i)
   {
      if(activeQueries[i][0]==trainNumber)//Another query on same train
      {         
         otherQueryType = activeQueries[i][1];
         otherThread = activeQueries[i][2];
         break;
      }
   }
   pthread_mutex_unlock(&csmutex);
   if(i != MAX)//Failed
   {      
      char otherQuery[20];
      if(otherQueryType == INQUIRE)
         strcpy(otherQuery, "inquire");
      else if(otherQueryType == BOOK)
         strcpy(otherQuery, "booking");
      else
         strcpy(otherQuery, "cancellation");

      printf("\nThread %d : Cancellation Query on train %d failed since thread %d is making %s query\n", no, trainNumber, otherThread, otherQuery);
      return;
   }

   //Fill an empty entry
   int entry;
   pthread_mutex_lock(&csmutex);//activeQueries is shared table
   for(i=0;i<MAX;++i)
   {
      //printf("thread %d : inside queries = %d\n", no, numActiveQueries);
      if(activeQueries[i][0] == -1)//Empty entry
      {
         entry = i;
         activeQueries[i][0] = trainNumber;
         activeQueries[i][1] = CANCEL;
         activeQueries[i][2] = no;
         break;
      }
   }
   pthread_mutex_unlock(&csmutex);


   printf("\nThread %d : Begin Cancellation of ticket from train %d\n", no, trainNumber);
   sleep(SLEEPTIME);

   //Modify data
   isBooked[trainNumber]--;//cancel a ticket
   trains[trainNumber]++;//Increase number of available tickets

   printf("\nThread %d : End Cancellation of ticket from train %d\n", no, trainNumber);
   
   pthread_mutex_lock(&csmutex);
   if(entry >= MAX || entry <= -1*MAX)
      entry = rand()%MAX;
   activeQueries[entry][0] = -1;//Empty the corresponding entry
   pthread_mutex_unlock(&csmutex);
}


/* This is the main function for a worker thread */
/* A worker thread receives a number and a 4-letter name via targ */
void * tmain(void * targ)
{
   /* Local variables are not shared with other threads */
   int no, i;
   char name[5];
   pthread_t tid;
   int isBooked[t];//Tells how many times train i has been booked by this thread.
   memset(isBooked, 0, sizeof(isBooked));
   int trainsBooked[t];//Tells which all trains have been booked by this thread
   int numTrainsBooked = 0;//Count of the number of trains booked by this thread
   
   /* Retrieve my number and name from the parameter passed */
   no = ((tinfo *)targ) -> tno;
   strcpy(name,((tinfo *)targ) -> tname);

   /* Retrieve my thread id */
   tid = pthread_self();

   while (1) {
      /* Check for termination condition */

      pthread_mutex_lock(&donemutex);

      /* if the master thread is done */
      if(mdone)
      {
         pthread_mutex_unlock(&donemutex);
         pthread_barrier_wait(&barrier);         
         pthread_exit(NULL);
      }
      /* The master thread is still sleeping, so I continue to work */
      pthread_mutex_unlock(&donemutex);


      //Check if number of active queries is less than MAX
      pthread_mutex_lock(&querymutex);
      if(numActiveQueries == MAX)
      {
         printf("\nThread %d : Blocked since active queries = MAX\n", no);
         pthread_cond_wait(&querycond, &querymutex);
      }      
      numActiveQueries++;
      pthread_mutex_unlock(&querymutex);        

      int a123;
      //Start query
      int queryType = 1 + rand()%3;
      if(queryType == INQUIRE)
      {
         inquiry(no);
      }
      else if(queryType == BOOK)
      {
         booking(no, isBooked, trainsBooked, &numTrainsBooked);
      }
      else
      {
         cancellation(no, isBooked, trainsBooked, &numTrainsBooked);
      }

      pthread_mutex_lock(&querymutex);
      numActiveQueries--;
      if(numActiveQueries == (MAX-1))//wake up a waiting query
         pthread_cond_signal(&querycond);
      pthread_mutex_unlock(&querymutex);
      
      sleep(SLEEPTIME);
   }
}

/* Function to create s worker threads. Their thread ids are stored in tid.
   The parameters (numbers and names) are stored in param. */
void create_workers(pthread_t *tid, tinfo *param)
{
   pthread_attr_t attr;
   int i, j;

   /* Set attributes for creating joinable threads */
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   /* Create worker threads in a loop */
   for (i=0; i<s; ++i) {
      /* Set the number of the thread */
      param[i].tno = i + 1;

      /* Set a random 4-letter name for the thread */
      for (j=0; j<4; ++j) param[i].tname[j] = 'A' + rand() % 26;
      param[i].tname[4] = '\0';

      /* Create thread with number and name passed as parameters */
      if  (pthread_create(tid + i, &attr, tmain, (void *)(param+i))) {
         fprintf(stderr, "Master thread: Unable to create thread\n");
         pthread_attr_destroy(&attr);
         exit(1);
      }
  }

   /* Wait for a while to insure that all worker threads get the chance to be
      scheduled and read the correct local values defined in this function */
   sleep(1);

   pthread_attr_destroy(&attr);
}

void create_mutex ()
{
   pthread_barrier_init (&barrier, NULL, s+1);

   pthread_mutex_init(&csmutex, NULL);
   pthread_mutex_init(&donemutex, NULL);
   pthread_mutex_init(&querymutex, NULL);
     
   pthread_mutex_trylock(&csmutex);   /* Try to lock mutex (non-blocking) */
   pthread_mutex_unlock(&csmutex);    /* Now, unlock the mutex */

   pthread_mutex_trylock(&querymutex);   /* Try to lock mutex (non-blocking) */
   pthread_mutex_unlock(&querymutex);    /* Now, unlock the mutex */

   pthread_mutex_trylock(&donemutex); /* Try to lock mutex (non-blocking) */
   pthread_mutex_unlock(&donemutex);  /* Now, unlock the mutex */

   /* Initialize condition variable */
   pthread_cond_init(&donecond, NULL);
   pthread_cond_init(&querycond, NULL);
}

void do_work(pthread_t *tid, tinfo *param)
{
   int i;

   /*Server runs for T seconds */
   sleep(T);

   /* At the end of work, the master thread sets the mdone flag */
   pthread_mutex_lock(&donemutex);
   mdone = 1;
   pthread_mutex_unlock(&donemutex);

   pthread_barrier_wait(&barrier);

   for (i=0; i<s; ++i)
   {
      if (pthread_join(tid[i],NULL))
      {
         fprintf(stderr, "Unable to wait for thread [%lu]\n", tid[i]);
      }
   }
   printf("\n");

   printf("Reservation status : \n");
   for(i=0;i<t;++i)
      printf("Train %d : %d\n", i,trains[i]);

}

void wind_up ()
{
   /* Destroy mutex and condition variables */
   printf("\nWinding up\n\n");
   pthread_mutex_destroy(&csmutex);
   pthread_mutex_destroy(&querymutex);
   pthread_mutex_destroy(&donemutex);
   pthread_cond_destroy(&querycond);
   pthread_cond_destroy(&donecond);
}

int main ()
{
   srand((unsigned int)time(NULL));
   int i;
   //Creating and intializing trains
   trains = (int *)malloc(t*sizeof(int));
   for(i=0;i<t;++i)
      trains[i] = c;

   //Initialise all activeQueries train number to -1
   for(i=0;i<MAX;++i)
      activeQueries[i][0] = -1;

   pthread_t tid[s];
   tinfo param[s];

   create_mutex();
   create_workers(tid,param);
   do_work(tid,param);
   wind_up();
   exit(0);
}
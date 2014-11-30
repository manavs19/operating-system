/*
Manav Sethi : 11CS30044
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>
#include <sys/shm.h>
#include <signal.h>
#include <stdlib.h>

#define N 5
#define LEFT i
#define RIGHT (i+1)%N
#define MAXTIME 5

typedef int semaphore;
struct sembuf pop, vop ;

semaphore* semFork;
semaphore * mutex;
int mutexID;
int semForkID;
// int * resourceGraph;
int resourceGraphID;

/*
resourceGraph[i][j] = 1  ==>  Philosopher i has fork j
*/   

struct sembuf pop, vop ;

#define P(s) semop(s, &pop, 1)  /* pop is the structure we pass for doing the P(s) operation */
#define V(s) semop(s, &vop, 1)  /* vop is the structure we pass for doing the V(s) operation */

void remove_all();
void philosopher(int i);
void take_left_fork(int i, int * resourceGraph);
int take_right_fork(int i, int * resourceGraph);
void put_forks(int i, int * resourceGraph);

//Removes shared memory and semaphores on Ctrl+C
void remove_all()
{
	int i;
	//Removing shared memory and semaphores alloted
	for(i=0;i<N;++i)
	{
		semctl(semFork[i], 0, IPC_RMID, 0);
	}
	shmctl(semForkID, IPC_RMID, 0);
	semctl(*mutex, 0, IPC_RMID, 0);
	shmctl(mutexID, IPC_RMID, 0);
	shmctl(resourceGraphID, IPC_RMID, 0);

	exit(0);
}

//Simulates behaviour of philosopher i
void philosopher(int i)
{
	int * resourceGraph = shmat(resourceGraphID, 0, 0);

	int thinkTime = rand()%MAXTIME+1;//Thinking Time
	int eatTime = rand()%MAXTIME+1;//Eating Time
	int pickLag = rand()%MAXTIME+1;//Lag between left and right forks

	while(1)
	{
		sleep(thinkTime);  //thinking

		int hasBothForks = 0;
		while(1)//For Rolling Back
		{
			take_left_fork(i, resourceGraph);
			sleep(pickLag);
	
			P(*mutex);//Shared memory accessed
			if(resourceGraph[i*N+i]==0)//Has been preempted by parent, So Roll Back
			{
				V(*mutex);
				continue;//Roll Back
			}
			V(*mutex);

			int hasBothForks = take_right_fork(i, resourceGraph);
			// printf("philosopher %d hasBothForks = %d\n", i,hasBothForks);
			if(hasBothForks==1)
				break;
		}

		// printf("Outside\n");
		printf("Philosopher %d starts eating\n",i);
		sleep(eatTime);  //eating
		printf("Philosopher %d ends eating and releases forks %d (left) and %d (right)\n",i,LEFT,RIGHT);
		printf("Philosopher %d starts thinking\n",i);

		put_forks(i, resourceGraph);
	}

}

//Gives left fork to philosopher i
void take_left_fork(int i, int * resourceGraph)
{
	P(semFork[LEFT]);
	P(*mutex);
	resourceGraph[i*N+i] = 1;
	V(*mutex);
	printf("Philosopher %d grabs fork %d (left)\n",i,LEFT);
}

//Gives right fork to philosopher i
//Returns 1 if philosopher i has got both forks, otherwise 0
int take_right_fork(int i, int * resourceGraph)
{
	P(semFork[RIGHT]);

	P(*mutex);//Shared memory accessed
	if(resourceGraph[i*N+i]==0)//Does not have left fork, So Roll Back
	{
		V(semFork[RIGHT]);//Release right fork
		V(*mutex);
		return 0;//Roll Back
	}
	else
	{
		resourceGraph[RIGHT*N + RIGHT] = 0;
		resourceGraph[i*N+(RIGHT)] = 1;
		V(*mutex);
		printf("Philosopher %d grabs fork %d (right)\n",i,RIGHT);
		return 1;
	}
}


//Philosopher puts down both left and right forks
void put_forks(int i, int * resourceGraph)
{
	V(semFork[LEFT]);
	P(*mutex);
	resourceGraph[i*N+i] = 0;
	V(*mutex);
	
	V(semFork[RIGHT]);
	P(*mutex);
	resourceGraph[i*N+RIGHT] = 0;
	V(*mutex);	
}

void main()
{
	signal(SIGINT, remove_all);
	srand((unsigned int)time(NULL));
	int i, j;

	//getting shared memory and attaching it
	resourceGraphID = shmget(IPC_PRIVATE, N*N*sizeof(int), 0777|IPC_CREAT);
	int * resourceGraph = shmat(resourceGraphID, 0, 0);

	semForkID = shmget(IPC_PRIVATE, N*sizeof(semaphore), 0777|IPC_CREAT);
	semFork = (semaphore *)shmat(semForkID, 0, 0);

	mutexID = shmget(IPC_PRIVATE, 1*sizeof(semaphore), 0777|IPC_CREAT);
	mutex = (semaphore *)shmat(mutexID, 0, 0);	

	//Initialise the resource Allocation Graph
	for(i=0;i<N;++i)
	{
		for(j=0;j<N;++j)
		{
			resourceGraph[i*N+j] = 0;
		}
	}

	for(i=0;i<N;i++)
	{
		//Getting the required semaphore variables
		semFork[i] = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
		semctl(semFork[i], 0, SETVAL, 1);
	}

	*mutex = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	semctl(*mutex, 0, SETVAL, 1);

	pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1 ; vop.sem_op = 1 ;

	for(i=0;i<N;i++)
	{
		if(fork()==0)
		{
			printf("Philosopher %d starts thinking\n",i);
			philosopher(i);
			exit(0);
		}
	}

	//Parent process
	while(1)
	{
		P(*mutex);
		for(i=0;i<N;++i)
		{
			if(resourceGraph[i*N+i]==0)//Philosopher i does not have left fork, So no deadlock
				break;
		}
		
		if(i==N)//All philosophers holding left forks, So deadlock detected
		{
			printf("Parent detects deadlock, going to initiate recovery\n");
			int toPreempt = rand()%N;
			printf("Parent preempts Philosopher %d\n", toPreempt);
			resourceGraph[toPreempt*N+toPreempt] = 0;
			V(semFork[toPreempt]);	
		}
		V(*mutex);
		sleep(1);
	}

	int status;
	wait(&status);

	//Removing shared memory and semaphores alloted
	for(i=0;i<N;++i)
	{
		semctl(semFork[i], 0, IPC_RMID, 0);
	}
	shmctl(semForkID, IPC_RMID, 0);
	semctl(*mutex, 0, IPC_RMID, 0);
	shmctl(mutexID, IPC_RMID, 0);
	shmctl(resourceGraphID, IPC_RMID, 0);

	exit(0);	
}
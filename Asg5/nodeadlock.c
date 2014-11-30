/*
Manav Sethi : 11CS30044
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>	/* Include this to use semaphores */

#define N 5
#define LEFT (i+N-1)%N
#define RIGHT (i+1)%N
#define THINKING 0
#define HUNGRY 1
#define EATING 2
#define MAXTIME 5

#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

typedef int semaphore;

struct sembuf pop, vop;
int shm_mutex, shm_state, shm_sem_arr;
semaphore * mutex, * state, * sem_arr;

//Removes shared memory and semaphores on Ctrl+C
void kill_all()
{
	int i;
	//Removing shared memory and semaphores alloted
	for(i=0;i<N;++i)
	{
		semctl(sem_arr[i], 0, IPC_RMID, 0);
	}
	semctl(*mutex, 0, IPC_RMID, 0);
	shmctl(shm_mutex, IPC_RMID, 0);
	shmctl(shm_state, IPC_RMID, 0);
	shmctl(shm_sem_arr, IPC_RMID, 0);

	exit(0);
}

//Checks whether forks are available
void test(int i)
{
	if(state[i]==HUNGRY && state[LEFT]!=EATING && state[RIGHT]!=EATING)
	{
		state[i] = EATING;
		printf("Philosopher %d grabs fork %d (left)\n", i,i);
		printf("Philosopher %d grabs fork %d (right)\n", i,RIGHT);
		V(sem_arr[i]);		
	}
}

//Gives both left and right forks to philosopher i
void take_forks(int i)
{
	P(*mutex);
	state[i] = HUNGRY;
	test(i);	
	V(*mutex);
	P(sem_arr[i]);
	printf("Philosopher %d starts eating\n", i);
}

//Philosopher i puts back both left and right forks
void put_forks(int i)
{
	P(*mutex);
	printf("Philosopher %d ends eating and releases forks %d (left) and %d (right)\n", i,i,RIGHT);
	state[i] = THINKING;
	printf("Philosopher %d starts thinking\n", i);
	test(LEFT);
	test(RIGHT);
	V(*mutex);
}

//Simulates behaviour of philosopher i
void philosopher(int i)
{
	while(1)
	{
		int thinkTime = rand()%MAXTIME;//Thinking time
		sleep(thinkTime);
		take_forks(i);
		
		int eatTime = rand()%MAXTIME;//Eating time
		sleep(eatTime);

		put_forks(i);		
	}
}

void main()
{
	signal(SIGINT, kill_all);
	srand((unsigned int)time(NULL));
	int i,j, status;
	//Shared memory allocated
	shm_mutex = shmget(IPC_PRIVATE, 1*sizeof(int), 0777|IPC_CREAT);
	shm_state = shmget(IPC_PRIVATE, N*sizeof(int), 0777|IPC_CREAT);
	shm_sem_arr = shmget(IPC_PRIVATE, N*sizeof(int), 0777|IPC_CREAT);

	//Shared memory attached
	mutex = (int *)shmat(shm_mutex, 0, 0);
	state = (int *)shmat(shm_state, 0, 0);
	sem_arr = (int *)shmat(shm_sem_arr, 0, 0);

	pop.sem_num = 0;
	vop.sem_num = 0;
	pop.sem_flg = 0;
	vop.sem_flg = 0;
	pop.sem_op = -1;
	vop.sem_op = 1;

	for(i=0;i<N;++i)
	{
		//Getting the desired semaphores
		sem_arr[i] = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
		semctl(sem_arr[i], 0, SETVAL, 0);
		state[i] = THINKING;
	}	

	(*mutex) = semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	semctl(*mutex, 0, SETVAL, 1);

	for(i=0;i<N;++i)
	{
		if(fork()==0)
		{
			printf("Philosopher %d starts thinking\n", i);
			philosopher(i);
			exit(0);
		}
	}	
	
	wait(&status);

	//Removing shared memory and semaphores alloted
	for(i=0;i<N;++i)
	{
		semctl(sem_arr[i], 0, IPC_RMID, 0);
	}
	shmctl(shm_mutex, IPC_RMID, 0);
	shmctl(shm_state, IPC_RMID, 0);
	shmctl(shm_sem_arr, IPC_RMID, 0);

	exit(0);	
}
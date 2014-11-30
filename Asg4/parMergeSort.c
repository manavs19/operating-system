#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/shm.h>

int ML;
int ms;
#define SIZE 10
#define MAX 1000000
#define MIN -1

int shmid;

//Selection sort
void sort(int * arr, int n)
{
	int i, j;
	for(i=0;i<n;++i)
	{
		int min = arr[i];
		int pos = i;
		for(j=i+1;j<n;++j)
		{
			if(arr[j] < min)
			{
				pos = j;
				min = arr[j];
			}
		}
		int temp = arr[pos];
		arr[pos] = arr[i];
		arr[i] = temp;
	}
}

//Sorts arr from i to j in shared memory
void mergesort(int * arr, int i, int j, int l)
{
	if(l > ML || (j-i+1) < ms)//Cutoff subarray
	{
		sort(arr+i,j-i+1);
		return;
	}

	char buf[SIZE];	
	int status1, status2;

	int m = (i+j)/2;//Mid
	int totalIter = (j-i+1)/2;//Total iterations in merging
	int cpid1, cpid2;
	int fd1[2], fd2[2];
	pipe(fd1);
	pipe(fd2);
	if((cpid1=fork()) == 0)//Left child
	{
		close(fd1[0]);
		close(fd2[1]);
		l++;
		int k;					

		mergesort(arr, i, m, l);

		int * brr = (int *)malloc((m-i+1)*sizeof(int));//Local copy of shared memory from i to m
		
		//Initiate merging process
		strcpy(buf, "Done");
		write(fd1[1], buf, SIZE);
		read(fd2[0], buf, SIZE);
		while(strcmp(buf, "Done")!=0)
		{
			read(fd2[0], buf, SIZE);		
		}

		int front = i, rear = m;//For arr
		int writeFront = 0;//For brr

		int idx;
		for(idx=1;idx<=totalIter;++idx)
		{	
			int a,b;
			//Front values
			if(front > rear)//All elements finished
			{
				a = MAX;
			}
			else
			{
				a = arr[front];
			}

			sprintf(buf, "%d", a);
			write(fd1[1], buf, SIZE);

			read(fd2[0], buf, SIZE);
			b = atoi(buf);

			if(a <= b)
			{
				brr[writeFront++] = a;
				front++;
			}
			else
			{
				brr[writeFront++] = b;
			}
		
			//Rear values

			if(front > rear)//All elements finished
			{
				a = MIN;
			}
			else
			{
				a = arr[rear];
			}
			sprintf(buf, "%d", a);
			write(fd1[1], buf, SIZE);			
			
			read(fd2[0], buf, SIZE);
			b = atoi(buf);

			if(b < a)
			{
				rear--;
			}
		}

		if((j-i+1)%2==1)//odd
			brr[writeFront] = arr[front];

		//Merge on shared arr
		strcpy(buf, "Done");
		write(fd1[1], buf, SIZE);
		read(fd2[0], buf, SIZE);
		while(strcmp(buf, "Done")!=0)
		{
			read(fd2[0], buf, SIZE);		
		}

		for(k=i;k<=m;++k)
		{
			arr[k] = brr[k-i];
		}

		free(brr);
		exit(0);
	}
	else if((cpid2 = fork()) == 0)//Right child
	{
		close(fd1[1]);
		close(fd2[0]);
		l++;
		int k;

		mergesort(arr, m+1, j, l);		
		
		int * brr = (int *)malloc((j-m)*sizeof(int));//Local copy of shared memory from m+1 to j
		
		read(fd1[0], buf, SIZE);
		while(strcmp(buf, "Done")!=0)
		{
			read(fd1[0], buf, SIZE);		
		}
		write(fd2[1], buf, SIZE);

		int front = m+1, rear = j;//for arr
		int writeBack = j-m-1;//For brr
		
		int idx;
		for(idx=1;idx<=totalIter;++idx)
		{
			int a,b;

			//Front values
			read(fd1[0], buf, SIZE);
			a = atoi(buf);

			if(front > rear)
			{
				b = MAX;
			}
			else
			{
				b = arr[front];
			}
			sprintf(buf, "%d", b);
			write(fd2[1], buf, SIZE);

			if(a>b)
			{
				front++;
			} 

			//Rear values
			read(fd1[0], buf, SIZE);
			a = atoi(buf);

			if(front > rear)
			{
				b = MIN;
			}
			else
			{
				b = arr[rear];
			}
			sprintf(buf, "%d", b);
			write(fd2[1], buf, SIZE);

			if(b >= a)
			{
				brr[writeBack--] = b;
				rear--;
			}
			else
			{
				brr[writeBack--] = a;
			}
		}

		//Merge on shared arr
		read(fd1[0], buf, SIZE);
		while(strcmp(buf, "Done")!=0)
		{
			read(fd1[0], buf, SIZE);		
		}
		write(fd2[1], buf, SIZE);

		for(k=m+1;k<=j;++k)
		{
			arr[k] = brr[k-m-1];
		}

		free(brr);
		exit(0);
	}
	else//Parent
	{
		close(fd1[0]);
		close(fd1[1]);
		close(fd2[0]);
		close(fd2[1]);
		//Waiting for children to exit
		waitpid(cpid1, &status1, 0);
		waitpid(cpid2, &status2, 0);		

		return;
	}
}

int main()
{
	srand((unsigned int)time(NULL));
	int status;
	int * arr, * b;
	int i;

	int n;
	printf("Enter size n : ");
	scanf("%d", &n);

	printf("Enter Maximum levels ML : ");
	scanf("%d",&ML);
	printf("Enter minimum size ms : ");
	scanf("%d",&ms);

	key_t shmkey = ftok("/usr/local/lib/", 100);//Generate key using ftok()
	
	if((shmid = shmget(shmkey, n*sizeof(int), 0777|IPC_CREAT))==-1)
	{
		printf("Could not allocate memory\n");
		exit(0);
	}

	arr = (int *)shmat(shmid, 0, 0);
	//Randomly populate shared memory
	for(i=0;i<n;++i)
	{
		arr[i] = rand()%MAX;
	}

	printf("original array is : ");
	for (i = 0; i < n; ++i)
	{
		printf("%d ", arr[i]);
	}
	printf("\n\n\n");

	mergesort(arr, 0, n-1, 0);
	
	if(n<10000)
	{
		printf("Sort at end : ");
		for (i = 0; i < n; ++i)
		{
			printf("%d ", arr[i]);
		}
		printf("\n");
	}
	else//Write to file
	{
		FILE * fp = fopen("sorted.txt", "w");
		for (i = 0; i < n; ++i)
		{
			fprintf(fp, "%d ", arr[i]);
		}
		printf("Output written to file sorted.txt\n");		
	}

	shmdt(arr);//Detach shared memory
	shmctl(shmid, IPC_RMID, 0);//Remove shared memory

	return 0;
}
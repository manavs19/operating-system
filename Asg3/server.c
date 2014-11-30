//Relevant headers
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
 
#define SIZE_BUF 256

//Structure for message
struct MyMessage
{
	long type;//Type or process
	char msg[SIZE_BUF];//Actual Message
};

//Gets us the current time in the required format
char * getTime()
{
    char * s = (char *)malloc(100 * sizeof(char));
    time_t temp;
    struct tm *timeptr;

    temp = time(NULL);
    timeptr = localtime(&temp);

    strftime(s,sizeof(s),"Msg received at time: %a %b %d %T %Z %Y", timeptr);
    
    return s;
}

int main()
{
	key_t UPkey, DOWNkey;
	char s[100];
   	time_t temp1;
   	struct tm *timeptr;

	UPkey = 100;//Key for UP queue
	DOWNkey = 200;//Key for DOWN queue
	int UPid = msgget(UPkey, IPC_CREAT | 0666);//Creating UP queue
	int DOWNid = msgget(DOWNkey, IPC_CREAT | 0666);//Creating DOWN queue
	
	while(1)
	{
		struct MyMessage receive, result;
		msgrcv(UPid, &receive, SIZE_BUF, 0, 0);//server receives a message from UP queue

     	temp1 = time(NULL);
     	timeptr = localtime(&temp1);

     	//Print current time
     	strftime(s,sizeof(s),"Msg received at time: %a %b %d %T %Z %Y", timeptr);
     	printf("%s\n",s);

     	//Process the message
		char temp[SIZE_BUF];
		int i;
		for(i=0;i<strlen(receive.msg);++i)
		{
			char ch = receive.msg[i];
			if(ch>=65 && ch<=90)//Uppercase
			{
				temp[i] = ch+32;
			}
			else if(ch>=97 && ch<=122)
			{
				temp[i] = ch-32;
			}
			else
			{
				temp[i] = ch;
			}
		}
		temp[i] = '\0';
		strcpy(result.msg, temp);
		result.type = receive.type;//Type is process id of client process
		msgsnd(DOWNid, &result, SIZE_BUF, 0);//Send the message through DOWN queue.
	}

	return 0;
}
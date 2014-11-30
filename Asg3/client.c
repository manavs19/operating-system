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

//Structure for Message
struct MyMessage
{
	long type;//Type or Process
	char msg[SIZE_BUF];//Actual Message
};

int main()
{
	key_t UPkey, DOWNkey;
	UPkey = 100;//Key for UP queue
	DOWNkey = 200;//Key for down queue
	int UPid = msgget(UPkey, IPC_CREAT | 0666);//Create UP queue
	int DOWNid = msgget(DOWNkey, IPC_CREAT | 0666);//Create DOWN queue

	struct MyMessage send, result;
	printf("Insert message to send to server: ");
	scanf("%[^\n]", send.msg);
	send.type = getpid();//Type is Process id of current process

	msgsnd(UPid, &send, SIZE_BUF, 0);//Send the Message to server

	msgrcv(DOWNid, &result, SIZE_BUF, getpid(), 0);//Receive Message from server

	printf("Processed msg from server: %s\n", result.msg);

	return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char * argv[])
{
	//Check for valid command line argument
	if(argc != 2)
	{
		printf("Invalid arguments!!\n");
		exit(0);	
	}

	//Making the command line arguments for execvp()
	char * arguments[] = { "xterm","-e", "./parmax", argv[1], NULL };
	execvp("xterm", arguments);//exec the xterm

	exit(0);
}
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <signal.h>

//function to make the time passed as a string for convenient output
void makeDate(time_t temp, char * date)
{
   struct tm *ts;   //data structure for handling time elements
   ts = localtime(&temp);

   //for getting month name
   switch(ts->tm_mon)
   {
      case 0:
         strcpy(date, "Jan");
         break;
      case 1:
         strcpy(date, "Feb");
         break;
      case 2:
         strcpy(date, "Mar");
         break;
      case 3:
         strcpy(date, "Apr");
         break;
      case 4:
         strcpy(date, "May");
         break;
      case 5:
         strcpy(date, "Jun");
         break;
      case 6:
         strcpy(date, "Jul");
         break;
      case 7:
         strcpy(date, "Aug");
         break;
      case 8:
         strcpy(date, "Sep");
         break;
      case 9:
         strcpy(date, "Oct");
         break;
      case 10:
         strcpy(date, "Nov");
         break;
      case 11:
         strcpy(date, "Dec");
         break;         
   }

   char day[100];
   //for rest of the time elements
   sprintf(day, " %d %d:%d", ts->tm_mday, ts->tm_hour, ts->tm_min);
   strcat(date, day);   
}

//main function
int main ()
{
   char * line = (char *)malloc(1000*sizeof(char));  //to get the instructions written in shell
   char * pch;
   int status;

   while(1)
   {
      printf("myshell> ");  //start of each shell line
      gets(line);  //getting line
      
      pch = strtok(line, " \n\r");   //to get first token of line
      
      //if command is cd, to change directory
      if(strcmp(pch, "cd")==0)
      {
         pch = strtok(NULL, "\n\r");
         status = chdir(pch);
         if(status == -1)
            printf("Could not change directory\n");
      }
      //if command is pwd, to get directory
      else if(strcmp(pch, "pwd")==0)
      {
         char path[100];
         getcwd(path, 100);
         printf("%s\n", path);
      }
      //mkdir, to make directory
      else if(strcmp(pch, "mkdir")==0)
      {
         pch  = strtok(NULL, "\n\r");
         status = mkdir(pch, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
         if(status == -1)
            printf("Could not create directory\n");        
      }
      //rmdir, to remove directory
      else if(strcmp(pch, "rmdir")==0)
      {
         pch  = strtok(NULL, "\n\r");
         status = rmdir(pch);
         if(status == -1)
            printf("Could not remove directory\n");
      }
      //ls and ls-l, to get files' information in directory
      else if(strcmp(pch, "ls")==0)
      {
         pch = strtok(NULL, " \r\n");
         char path[100];
         getcwd(path, 100);
         
         DIR *dir;
         struct dirent *ent;
         if ((dir = opendir (path)) != NULL)
         {
            /* print all the files and directories within directory */
            while ((ent = readdir (dir)) != NULL)
            {
               if(pch!=NULL && strcmp(pch, "-l")==0)  //case of ls -l
               {
                  struct  stat filestats;
                  if(strcmp(ent->d_name, ".")!=0 && strcmp(ent->d_name, "..")!=0)
                  {
                     status = stat(ent->d_name, &filestats);
                     char date[100];
                     makeDate(filestats.st_atime, date);
                     struct passwd *pw = getpwuid(filestats.st_uid);
                     struct group  *gr = getgrgid(filestats.st_gid);
                     
                     //Printing File Permissions
                     printf( (S_ISDIR(filestats.st_mode)) ? "d" : "-");
                     printf( (filestats.st_mode & S_IRUSR) ? "r" : "-");
                     printf( (filestats.st_mode & S_IWUSR) ? "w" : "-");
                     printf( (filestats.st_mode & S_IXUSR) ? "x" : "-");
                     printf( (filestats.st_mode & S_IRGRP) ? "r" : "-");
                     printf( (filestats.st_mode & S_IWGRP) ? "w" : "-");
                     printf( (filestats.st_mode & S_IXGRP) ? "x" : "-");
                     printf( (filestats.st_mode & S_IROTH) ? "r" : "-");
                     printf( (filestats.st_mode & S_IWOTH) ? "w" : "-");
                     printf( (filestats.st_mode & S_IXOTH) ? "x" : "-");
                     
                     //printing
                     printf(" %lu %s %s %lld %s %s\r\n",(unsigned long ) filestats.st_nlink, pw->pw_name, gr->gr_name, (unsigned long long ) filestats.st_size, date, ent->d_name);
                     
                  }
               }
               else  //case of only ls
               {
                  if(strcmp(ent->d_name, ".")!=0 && strcmp(ent->d_name, "..")!=0)
                     printf ("%s\t", ent->d_name);
               }
            }
            printf("\n");
            closedir (dir);
         }
         else
         {
            /* could not open directory */
            perror ("");
            return EXIT_FAILURE;
         }
      }
      //exit command
      else if(strcmp(pch, "exit")==0)
      {
         exit(0);
      }
      else  //program execution taking arguments
      {
         //execvp(executable, arguments);
         char executable[100];  //to get executable name
         char * arguments[100];  //to store arguments
         strcpy(executable, pch);
         arguments[0] =  pch;
         int k = 1;
         while(pch!=NULL)
         {
            pch = strtok(NULL, " \r\n");
            arguments[k++] =  pch;
         }

         int pid = fork();
         if(pid == 0)//Child
         {
            execv(executable, arguments);  //executing
            char* pPath;
            pPath = getenv ("PATH");  //getting path environment variable

            char tempPath[1000];
            strcpy(tempPath, pPath);

            pch = strtok(tempPath, ":\r\n");
            while(pch!=NULL)  //using all the paths present in the PATH environment variable
            {
               char path[1000];
               strcpy(path, pch);
               strcat(path, "/");
               strcat(path, executable);
               execv(path, arguments);
               pch = strtok(NULL, ":\r\n");
            }
            printf("Could not open file\n");
         }
         else
         {
            waitpid(pid, &status, 0);  //waiting
         }

      }

   }

   exit(0);
}
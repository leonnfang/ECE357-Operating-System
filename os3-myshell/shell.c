#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#define _GNU_SOURCE
//assume format pwd >out.txt <-there is no space between arrow and the output file
// if cd and pwd have some errors, the exit number will be set to -1
int latestExit = 0;
int numCommand = 0;
void goto_fork();
int main(int argc, char *argv[]){
	FILE *filept = stdin;
	if(argc >= 2){
		filept = fopen(argv[1],"r");	
	}
	while(1){
		char *lineptr;
		size_t n = 0;
		char *command[4096];
		numCommand = 0;
		int bytesRead = getline(&lineptr,&n,filept);
		if(bytesRead == -1){
			fprintf(stderr,"Error:%s",strerror(errno));
			continue;
		}
		if(*lineptr == '#' || bytesRead == 0){
			continue;
		}
		char *token = strtok(lineptr," ");

		while(token != NULL){
			command[numCommand] = malloc(4096);
			strcpy(command[numCommand],token);
			numCommand++;
			token = strtok(NULL," ");
		}
		int len = strlen(command[numCommand-1]);
		command[numCommand-1][len-1] = 0;
		if(strcmp(command[0],"cd") == 0){
			if(numCommand == 1){
				command[1] = malloc(4096);
				char *env = malloc(4096);
				env = getenv("HOME");
				if(env == NULL){
					fprintf(stderr,"for cd command, cannot find the match for the enviroment. Error:%s",strerror(errno));
					latestExit = -1;
				}else{
					strcpy(command[1],env);
				}
			
			}
			int hasError = chdir(command[1]);
			if(hasError == -1){
				fprintf(stderr,"cannot change the dir. Error:%s\n",strerror(errno));
				latestExit = -1;
			}
		}else if(strcmp(command[0],"pwd") == 0){
			char *buf = malloc(4096);
			char *newpath = getcwd(buf,4096);
			if(numCommand == 1 && newpath != NULL){
				printf("%s\n",newpath);
				free(buf);
			}else if(numCommand > 1 && newpath != NULL){
				goto_fork(command);
			}
			if(newpath == NULL){
				fprintf(stderr,"cannot get the curr dir. Error:%s\n",strerror(errno));
				latestExit = -1;
			}
		}else if(strcmp(command[0],"exit") == 0){
			int return_val;
			if(numCommand == 1){
				return_val = latestExit;
			}else{
				return_val = atoi(command[1]);
			}
			exit(return_val);
		}else{
			goto_fork(command);

		}
		// free command && free other free()
		int k;
		for(k=0;k<numCommand;k++){
			command[k] = NULL;
		}
	}
	return 0;
}
void goto_fork(char *command[]){
	struct timeval tv1;
	struct timeval tv2;
	int end_time;
	int start_time = gettimeofday(&tv1,NULL);
	int fd,j = 1;
	int flag = O_CREAT|O_RDWR;
	int io_start = 0;
	if(start_time == -1){
		fprintf(stderr,"Error:%s",strerror(errno));
	}
	pid_t pid = fork();
	int status = 0;;
	pid_t res_pid;
	struct rusage usage;
	switch(pid){
		case -1:
			fprintf(stderr,"the child process cannot be created. Error:%s",strerror(errno));
			break;
		case 0:
			while(j < numCommand){
				char *io1 = strstr(command[j],">");
				char *io2 = strstr(command[j],"<");
				char *io1_2 = strstr(command[j],"2>");
				char *pathname;
				if(io1 != NULL){
					// find the beginning of the io redirection
					if(io_start == 0){
						io_start = j;
					}
					if(*(io1+1) == '>'){
						flag = flag|O_APPEND;
						pathname = io1+2;
					}else if(io1_2 != NULL){
						flag = flag|O_APPEND;
						if(*((io1_2)+2) == '>'){
							pathname = io1_2+3;
						}else{
							pathname = io1_2+2;
						}
					}else{
						flag = flag|O_TRUNC;
						pathname = io1+1;
					}
					fd = open(pathname,flag,0666);
					if(fd  == -1){
						fprintf(stderr,"cannot open the i/o file. Error:%s\n",strerror(errno));
						latestExit = 1;
						exit(1);
						
					}
					int new_fd = dup2(fd,1);
					if(new_fd == -1){
						fprintf(stderr,"cannot dup the fd. Error:%s\n",strerror(errno));
						latestExit = 1;
						exit(1);
					}
					int close_error = close(fd);
					if(close_error == -1){
						fprintf(stderr,"cannot close the fd. Error:%s\n",strerror(errno));
						latestExit = 1;
						exit(1);
					}
				}
				if(io2 != NULL){
					if(io_start == 0){
						io_start = j;
					}
					pathname = io2+1;
					fd = open(pathname,flag,0666);
					if(fd  == -1){
						fprintf(stderr,"cannot open the i/o file. Error:%s\n",strerror(errno));
						latestExit = 1;
						exit(1);
						
					}
					int new_fd = dup2(fd,0);
					if(new_fd == -1){
						fprintf(stderr,"cannot dup the fd. Error:%s\n",strerror(errno));
						latestExit = 1;
						exit(1);
					}
					int close_error = close(fd);
					if(close_error == -1){
						fprintf(stderr,"cannot close the fd. Error:%s\n",strerror(errno));
						latestExit = 1;
						exit(1);
					}
				}
				j++;
			}
			// get the command for the exec
			if(io_start != 0){
				command[io_start] = NULL; 
			}
			int execvp_error = execvp(command[0],command);
			if(execvp_error == -1){
				fprintf(stderr,"cannot excute. Error:%s\n",strerror(errno));
				latestExit = 127;
				exit(127);
			}
			break;
		default:
			// need to add the core dump check
			res_pid = wait3(&status,0,&usage);
			if(res_pid == -1){
				fprintf(stderr,"Error:%s",strerror(errno));
			}
			end_time = gettimeofday(&tv2,NULL);
			if(end_time == -1){
				fprintf(stderr,"Error:%s",strerror(errno));
			}
			if(status != 0){
				if(WIFSIGNALED(status)){
					fprintf(stderr,"Child process %d exited with signal %d\n",res_pid,WTERMSIG(status));
				}else{
					fprintf(stderr,"Child process exited with non-zero return value %d\n",res_pid,WEXITSTATUS(status));
				}
				latestExit = WEXITSTATUS(status);
			}else{
				printf("Child process %d exited normally\n",res_pid);
			}
			printf("Real:%ld.%06lds\t",tv2.tv_sec-tv1.tv_sec,tv2.tv_usec-tv1.tv_usec);
			printf("User:%ld.%06lds\t",usage.ru_utime.tv_sec,usage.ru_utime.tv_usec);
			printf("Sys:%ld.%06lds\n",usage.ru_stime.tv_sec,usage.ru_stime.tv_usec);
			break;
	}
}

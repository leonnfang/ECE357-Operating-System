#include <unistd.h>
#include <string.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <setjmp.h>
#define _GNU_SOURCE
int pipegrep[2];
int pipemore[2];
int BUFSIZE = 4096;
int flag = -1;
char buf[4096];
char *pattern, *filename;
void err(char *syscall,char *file);
void forkit(char *file);
void randw(int inputfd,int outfd);
void sighandler();
int numfiles,Byets,fdin = 0;
jmp_buf env;
int main(int argc, char *argv[]){
	signal(SIGINT,sighandler);
	signal(SIGPIPE,sighandler);
	if(argc <= 2){
		fprintf(stderr, "Not enough input arguments\n");
		fprintf(stderr, "Usage: ./pp <pattern> infile1 [...infile2...]\n");
		exit(1);
	}
	pattern = malloc(4096);
	strcpy(pattern,argv[1]);
	int i;
	for(i = 2;i < argc;i++){
		if(sigsetjmp(env,2) == 0){

		}else{
			close(pipemore[1]);
			close(pipemore[0]);
			continue;
		}

		if(pipe(pipegrep) == -1){err("pipe()","pipegrep");	exit(1);}
		if(pipe(pipemore) == -1){err("pipe()","pipemore");	exit(1);}
		numfiles++;
		filename = argv[i];
		forkit(argv[i]);
	}
	exit(0);
} 
void err(char *syscall,char *file){
	fprintf(stderr,"Cannot call:%s for %s. Error:%s\n",syscall,file,strerror(errno));
}
void sighandler(int num){
	if(num == SIGINT){
		fprintf(stderr,"Total number of file: %d\n",numfiles);
		fprintf(stderr, "Total number of bytes transferred: %d\n",Byets);
	}
	if(flag == -1){
		if(close(fdin) == -1 || close(pipegrep[1])){
			err("close()","fdin or pipegrep[1] in the sighandler");
			exit(1);
		}
	}
	flag = -1;
	siglongjmp(env,2);
}
void randw(int inputfd,int outputfd){
	int readBytes, writeBytes,counter = 0;
	int infd = inputfd;
	int outfd = outputfd;
	while( (readBytes = read(infd,buf,BUFSIZE)) != 0){
				if(readBytes == -1){
					err("read()",filename);
					exit(1);
				}else{
					int j = 0;
					int isBinary = -1;
					for(;j < readBytes && isBinary == -1; j++){
						if(!(isprint(buf[j])||isspace(buf[j]))){
							isBinary = 1;
							//counter = 1;
							break; 
						}
					}
					if(isBinary == 1 && counter == 0){
						fprintf(stderr, "A Binary file:%s\n",filename);
						counter++;
					}
					writeBytes = write(outfd,buf,readBytes);
					if(writeBytes == -1){err("write()","write into the pipe");	exit(1);}
					Byets += writeBytes;
				}
				while(writeBytes < readBytes && writeBytes > 0){
					//partial write
					int w = write(outfd,buf,readBytes-writeBytes);
					if(w == -1){err("write()","partial write"); exit(1);};
					Byets += w;
					writeBytes += w;
				}
	}
}
void forkit(char *file){
	int cid,cid2,readBytes,writeBytes = 0;
	char *argu[] = {"grep",pattern,NULL};
	char *argu2[] = {"more",NULL};
	switch((cid = fork())){
		case -1:
			err("fork()",file);
			break;
		case 0:
			//in the child process
			if(close(pipegrep[1]) == -1){err("close()","file");	exit(1);}
			if(dup2(pipegrep[0],0) == -1){err("dup2","dup stdin to pipegrep[0]");	exit(1);}
			if(dup2(pipemore[1],1) == -1){err("dup2","pipemore[1]");	exit(1);}
			if(close(pipemore[0]) == -1){err("close()","pipemore[0] in child1");	exit(1);}
			if(close(pipemore[1]) == -1){err("close()","pipemore[1] in child1");	exit(1);}
			if(close(pipegrep[0]) == -1){err("close()","pipegrep[0] in child1");	exit(1);}
			if(execvp("grep",argu) == -1){
				err("execvp","grep function");
				exit(1);
			}
			break;
		default:
			if((cid2 = fork()) == -1){
				err("fork","child process for MORE");
				exit(1);
			}else if(cid2 == 0){
				if(close(pipegrep[0]) == -1){err("close()","pipegrep[0]");	exit(1);}
				if(close(pipegrep[1]) == -1){err("close()","pipegrep[1]");	exit(1);}
				if(dup2(pipemore[0],0) == -1){
					err("dup2","cannot dup stdin as pipemore[0]");
					exit(1);
				}
				if(close(pipemore[0]) == -1){err("close()","pipemore[0]"); exit(1);}
				if(close(pipemore[1]) == -1){err("close()","pipemore[1]"); exit(1);}
				if(execvp("more",argu2) == -1){
					err("execvp","MORE");
					exit(1);
				}
			}else{
				if(close(pipegrep[0]) == -1){err("clsoe()","pipegrep[0]");	exit(1);}
				if(close(pipemore[0]) == -1){err("close()","pipemore[0]");	exit(1);}
				if(close(pipemore[1])){err("close()","pipemore[1]");	exit(1);}
				fdin = open(file,O_RDONLY,0666);
				if(fdin == -1){err("open()",file);	exit(1);}
				randw(fdin,pipegrep[1]);
				if(close(fdin) == -1){err("close",file);	exit(1);}
				if(close(pipegrep[1])){err("close","pipegrep[1]");	exit(1);}
				flag = 1;
				if(waitpid(cid,NULL,0) == -1){err("waitpid","wait the child process");	exit(1);}
				if(waitpid(cid2,NULL,0) == -1){err("waitpid","wait the child process");	exit(1);}
			}
			break;
	}
}
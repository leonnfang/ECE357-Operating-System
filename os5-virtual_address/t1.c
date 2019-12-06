#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
char *filename = "test.txt";
void sighandler(int signum);
void filemake();
void prompt();
int fd,wb;
char *pmap;
char curchar,curr;
char buf[4096];
struct stat mystat;
char *input = "A";
int main(int argc, char *argv[]){
	int i;
	for(i = 1; i <= 31; i++){
		if(i == 9 || i == 19){
			continue;
		}
		if(signal(i,sighandler) == SIG_ERR){
			fprintf(stderr, "Cannot call signal(). Error:%s\n",strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	if(argc <= 1){
		fprintf(stderr, "Please specify a test case number");
		exit(EXIT_FAILURE);
	}
	int testnumber = atoi(argv[1]);
	filemake();
	if(fstat(fd,&mystat) < 0){
		fprintf(stderr, "Cannot call fstat() for:%s. Error:%s\n",filename,strerror(errno));
		if(close(fd) == -1){fprintf(stderr, "Cannot close:%s. Error:%s\n",filename,strerror(errno));}
		exit(EXIT_FAILURE);
	}
	switch(testnumber){
		case 1:
			pmap = mmap(NULL,mystat.st_size,PROT_READ,MAP_SHARED,fd,0);
			if(pmap == MAP_FAILED){
				fprintf(stderr, "Cannot call mmap() for:%s. Error:%s\n",filename,strerror(errno));
				exit(EXIT_FAILURE);
			}
			fprintf(stderr,"Executing Test #1 (write to r/o mmap):\n");
			prompt();

			*(pmap+3) = 'B';

			if(*(pmap+3) == 'B'){
				return 0;
			}else{
				return 255;
			}
			break;

		case 2:
			pmap = mmap(NULL,mystat.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
			if(pmap == MAP_FAILED){
				fprintf(stderr, "Cannot call mmap() for:%s. Error:%s\n",filename,strerror(errno));
				exit(EXIT_FAILURE);
			}
			fprintf(stderr,"Executing Test #2 (write to MAP_SHARED):\n");
			prompt();
			*(pmap+3) = 'B';
			if(read(fd,buf,4) < 0){
				fprintf(stderr, "Cannot call read(). Error:%s\n",strerror(errno));
				exit(EXIT_FAILURE);
			}
			if(buf[3] == 'B'){
				return 0;
			}else{
				return 1;
			}
			break;
		case 3:
			pmap = mmap(NULL,mystat.st_size,PROT_READ|PROT_WRITE,MAP_PRIVATE,fd,0);
			if(pmap == MAP_FAILED){
				fprintf(stderr, "Cannot call mmap() for:%s. Error:%s\n",filename,strerror(errno));
				exit(EXIT_FAILURE);
			}
			fprintf(stderr,"Executing Test #3 (write to MAP_PRIVATE):\n");
			prompt();
			*(pmap+3) = 'B';
			if(read(fd,buf,4) < 0){
				fprintf(stderr, "Cannot call read(). Error:%s\n",strerror(errno));
				exit(EXIT_FAILURE);
			}
			if(buf[3] == 'B'){
				return 0;
			}else{
				return 1;
			}
			break;
		case 4:
			fprintf(stderr, "Executing Test #4 (writing into a hole)\n");
			pmap = mmap(NULL,mystat.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
			curr = *(pmap+mystat.st_size-1);
			fprintf(stderr, "map[size-1] = '%c'\n",curr);
			fprintf(stderr, "writing a B to the end of the file\n");
			pmap[mystat.st_size] = 'B';
			if(lseek(fd,mystat.st_size+16,SEEK_SET) == -1){
				fprintf(stderr, "Cannot call lseek() for file for %s. Error:%s\n",filename,strerror(errno));
				exit(EXIT_FAILURE);
			}
			fprintf(stderr, "writing a B to the file(offset by 16)\n");
			if(write(fd,"B",1) < 0){
				fprintf(stderr, "Cannot call write() for file:%s. Error:%s\n",filename,strerror(errno));
				exit(EXIT_FAILURE);
			}
			if(lseek(fd,mystat.st_size,SEEK_SET) == -1){
				fprintf(stderr, "Cannot call lseek() for file:%s. Error:%s\n",filename,strerror(errno));
				exit(EXIT_FAILURE);
			}
			if(read(fd,buf,4) < 0){
				fprintf(stderr, "Cannot call read() for %s. Error:%s\n",filename,strerror(errno));
				exit(EXIT_FAILURE);
			}
			if(buf[0] == 'B'){
				return 0;
			}else{
				return 1;
			}
			break;
	}
	int size = mystat.st_size;
	if(testnumber == 4){
		size += 16;
	}
	if(munmap(pmap,size) == -1){
		fprintf(stderr, "Fail to call munmap() for file:%s. Error:%s\n",filename,strerror(errno));
		exit(EXIT_FAILURE);
	}
	if(close(fd) == -1){
		fprintf(stderr, "Cannot close fd for file:%s. Error:%s\n",filename,strerror(errno));
		exit(EXIT_FAILURE);
	}
	return 0;
}
void prompt(){
	curchar = *(pmap+3);
	fprintf(stderr,"map[3] = '%c'\n",curchar);
	fprintf(stderr,"writing a 'B' to map[3]\n");
}
void sighandler(int signum){
	fprintf(stderr, "Received a Signal:%d. Signal:%s\n",signum,strsignal(signum));
	exit(signum);
}
void filemake(){
	fd = open(filename,O_CREAT|O_RDWR|O_TRUNC,0666);
	if(fd == -1){
		fprintf(stderr, "Cannot opne file:%s. Error:%s\n",filename,strerror(errno));
		exit(EXIT_FAILURE);
	}
	int i = 0;
	for(i=0;i < 4000;i++){
		int writebytes = write(fd,input,1);
		if(writebytes < 0){
			fprintf(stderr, "Cannot call read() for %s. Error:%s\n",filename,strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	if(lseek(fd,0,SEEK_SET) == -1){
			fprintf(stderr, "Cannot call lseek() for %s. Error:%s\n",filename,strerror(errno));
			exit(EXIT_FAILURE);
	}
}


#include <fcntl.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main(int argc, char* argv[]){
	int BUFFSIZE = 4069;	
	char buf[BUFFSIZE]; 
	char* outfilename = "default_outfile";
	int out;
	int sum_syscall = 0;
	while((out=getopt(argc,argv,"o:")) != -1){	
		switch(out){
			case 'o':
				outfilename = optarg;
				break;
		}

	}
	sum_syscall++;
	int fdout;
	if(outfilename == "default_outfile" ){
		fdout = 0;
	}else{
		fdout = open(outfilename,O_RDWR|O_TRUNC|O_CREAT,0666);
		if(fdout < 0){
			fprintf(stderr,"something wrong happened(position: open the output file): %s \n", strerror(errno));
			exit(-1);		
		}
	}
	sum_syscall++;
	if(optind == argc){
		int readSTDinput, outSTDoutput;
		sum_syscall++;
		while((readSTDinput = read(1,buf,4069)) != 0){
			if(readSTDinput == -1){
				fprintf(stderr,"something wrong happened(position: STDinput): %s \n", strerror(errno));
				exit(-1);		
			}
			outSTDoutput = write(fdout,buf,readSTDinput);
			if(outSTDoutput == -1){
				fprintf(stderr,"something wrong happened(position: STDoutput): %s \n", strerror(errno));
				exit(-1);		
			}
			
		}
		fprintf(stderr,"(warning: no input file, only STDinput)the number of bytes transfered is : %d \n",outSTDoutput);
		exit(0);	
	}
	int i;
	int sum = 0;
	for(i = optind;i < argc;i++){
		int fd, readSTDinput, outSTDoutput;		
		char *ptr_hypen = "-";
		int counter = 0;
		int strcmpRes = strcmp(argv[i],ptr_hypen);
		if(strcmpRes == 0){
			while((readSTDinput = read(1,buf,4069)) != 0){
				sum_syscall++;				
				if(readSTDinput == -1){
					fprintf(stderr,"something wrong happened(position: STDinput): %s \n", strerror(errno));
					exit(-1);		
				}
				outSTDoutput = write(fdout,buf,readSTDinput);
				if(outSTDoutput == -1){
					fprintf(stderr,"something wrong happened(position: STDoutput): %s \n", strerror(errno));
					exit(-1);		
				}
				sum += outSTDoutput;			
			}		
			continue;
		}
		sum_syscall++;
		fd = open(argv[i],O_RDONLY);
		if(fd < 0){
			fprintf(stderr,"input file error, cannot open->(position: %s): %s \n",argv[i],strerror(errno));
			exit(-1);
		}
		int isend = -1;
		int isBinary = -1;
		while(isend != 0){
			int bytesRead = read(fd,buf,BUFFSIZE);
			sum_syscall++;
			if(bytesRead == -1){
				fprintf(stderr,"input file read, cannot read->(position: %s): %s \n",argv[i] ,strerror(errno));
				exit(-1);
			}
			if(bytesRead == 0){
				isend = 0;			
			}
			int j = 0;
			for(j = 0; j < bytesRead ; j++){
				if(!(isprint(buf[j])||isspace(buf[j]))){
					
					isBinary = 1;
					continue;		
				}
			}
			
			if(isBinary != -1 && counter == 0){
				fprintf(stderr,"u got a binary file. the filename is: %s \n",argv[i]);
				counter++;
							
			}
			int wd = write(fdout,buf,bytesRead);
			sum_syscall++;
			if(wd == -1){
				fprintf(stderr,"output file write, cannot write from %s to %s: %s \n",argv[i],outfilename,strerror(errno));
				exit(-1);
			}
			sum += wd;

		}	
	}
	int k = optind+1;
	for(;k < argc; k++){
		strcat(argv[optind],argv[k]);
	}
	fprintf(stderr,"the number of bytes transfered is: %d, number of syscall is: %d, and the file is transfered from %s to %s\n",sum,sum_syscall,argv[optind],outfilename);
	exit(0);
	
}

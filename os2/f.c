#include <getopt.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>
int v,m,u,depth,input_uid,read_user,read_group = 0;
char *input, *user;
int rootvol = 0;
char p_list[10];
void traversal(char* name);
void printinfo(char* pathname);
void convert();
dev_t devnum;
void errorReport();
char* s = "/";
int main(int argc, char* argv[]){
	int i = 0;
	int opt;
	while((opt=getopt(argc,argv,"m:vu:"))!=-1){
		switch(opt){
			case 'v':
				v = 1;
				break;
			case 'm':
				m = atoi(optarg);
				break;
			case 'u':
				u = 1;
				if(isdigit(*optarg) != 0){
					input_uid = atoi(optarg);
				}else{
					user = optarg;
				}
				break;
			case ':':
				fprintf(stderr,"missing flag in the arguement: %s",strerror(errno));
				exit(1);

			default:
				fprintf(stderr,"no flag set: %s",argv[0]);
				exit(1);
		}
	}	
	input = malloc(4096);
	if(optind == argc){
		fprintf(stdout,"need to input the pathname after all flags set\n");
		exit(1);
	}
	strcpy(input, argv[optind]);
	traversal(input);
	free(input);
	return 0;
}
void traversal(char* name){
	if(depth > 1023){
		printf("too deep\n");
		return;
	}	
	DIR *dir = opendir(name);
	if(dir == NULL){
		fprintf(stderr,"the path: %s cannot be open. The ERROR is: %s \n",name,strerror(errno));
		return;
	}
	struct dirent* curstr;
	int self = 0;
	while((curstr=readdir(dir))!= NULL){
		char currname[4096];
		strcpy(currname,name);// save as currpathname
		if(strcmp(currname, "./folder1/folder2/t.txt") == 0){
			printf("In folder 2");
		}
		struct stat buf;
		stat(name,&buf);
		if(v == 1 && rootvol == 1 && buf.st_rdev != devnum){
			continue;
		}		
		if(curstr->d_name[0] == '.'&& self == 0){
			printinfo(name);
			rootvol = 1;
			self++;
		}else if(curstr->d_name[0] == '.'){
			continue;	
		}else{
			if(curstr->d_type == DT_DIR){
				strcat(currname,s); // change the pathname by appending "/"
				strcat(currname,curstr->d_name);// change the pathname
				traversal(currname); // call recursion
				depth++;
			}else{
				strcat(currname,s);				
				strcat(currname,curstr->d_name);
				printinfo(currname);
			}
		}
	}
	int closed = closedir(dir);
	if(closed == -1){
		fprintf(stderr,"cannot close the dir:%s. Error:%s",name,strerror(errno));
	}
}
void printinfo(char* pathname){
	struct stat buf;
	struct passwd *pwd;
	struct group *gpd;
	int flag = lstat(pathname,&buf);
	if(flag == -1)
	{
		fprintf(stderr,"the information of the current path(%s) cannot be open. error:%s",pathname,strerror(errno));
	}
	pwd = getpwuid(buf.st_uid);
	gpd = getgrgid(buf.st_gid);
	if(u == 1){
		gid_t *groups;
		int ngroup = 0;
		getgrouplist(pwd->pw_name,pwd->pw_gid,groups,&ngroup);
		groups = malloc(ngroup*sizeof(gid_t *));
		getgrouplist(pwd->pw_name,pwd->pw_gid,groups,&ngroup);
		int group_p = 0;
		int e = 0;
		for(;e < ngroup;e++){
			if(groups[e] == buf.st_gid){
				group_p = 1;
				break;
			}
		}
		read_user = buf.st_mode & 00700?1:0;
		read_user = buf.st_mode & 00400?1:0;
		read_group = buf.st_mode & 00070?1:0;
		read_group = buf.st_mode & 00040?1:0;
		printf("%d",buf.st_uid);
		if((strcmp(pwd->pw_name,user)!=0 || read_user != 1) && (group_p != 1 || read_group != 1) && input_uid != buf.st_uid){
					return;
		}
		free(groups);
	}
	time_t rawtime,filetime,diff;
	filetime = buf.st_mtime;
	rawtime = time(NULL);
	diff = rawtime - filetime;// rawtime - filetime
	if(m > 0 && diff <= m){
			return;
	}
	if(m < 0 && diff > (-1)*m){
			return;
	}
	if(rootvol == 0){
		devnum = buf.st_rdev;// set dev_number
	}
	printf("%d\t",buf.st_ino);
	printf("%lld\t",(long long)buf.st_blocks/2);
	convert(buf.st_mode);
	printf("%s\t",p_list);
	if(pwd == NULL){
		fprintf(stderr,"Either Cannot find the correct struct(cannot find the uid) or Error: %s\n",strerror(errno));
	}else{
		printf("%s\t",pwd->pw_name);
	}
	if(gpd == NULL){
		fprintf(stderr,"Either Cannot find the correct struct(cannot find the gid) or Error: %s\n",strerror(errno));
	}else{
		printf("%s\t",gpd->gr_name);
	}
	printf("%lld\t",(long long)buf.st_size);
	printf("%s\t",ctime(&buf.st_mtime));
	printf("%s\t",pathname);
	char* buf_path = malloc(4096);
	char* p = "->";
	if(S_ISLNK(buf.st_mode)){
		int hasError = readlink(pathname,buf_path,4096);
		if(hasError == -1)
		{
			fprintf(stderr,"the symlink cannot be explored.(path:%s)Error:%s\n",pathname,strerror(errno));
		}else{
			printf("it is a symlink. the explored path is:%s%s%s\n",pathname,p,buf_path);
		}
	}else{
		printf("not crossing mount point\n");
	}
	free(buf_path);
}
void convert(mode_t m){
    p_list[0] = '-';
    if(S_ISDIR(m)){
    	p_list[0] = 'd';
    }else if(S_ISCHR(m)){
    	p_list[0] = 'c';
    }else if(S_ISBLK(m)){
    	p_list[0] = 'b';
    }else if(S_ISFIFO(m)){
    	p_list[0] = 'p';
    }else if(S_ISLNK(m)){
    	p_list[0] = 'l';
    }else if(S_ISSOCK(m)){
    	p_list[0] = 's';
    }
    p_list[1] = m&00400?'r':'-';
    p_list[2] = m&00200?'w':'-';
    p_list[3] = m&00100?'x':'-';
    p_list[4] = m&00040?'r':'-';
    p_list[5] = m&00020?'w':'-';
    p_list[6] = m&00010?'x':'-';
    p_list[7] = m&00004?'r':'-';
    p_list[8] = m&00002?'w':'-';
    p_list[9] = m&00001?'x':'-';
    if(m&01000){
    	if(p_list[9] == 'x'){
    		p_list[9] = 't';
    	}else{
    		p_list[9] = 'T';
    	}
    }
    if(m&04000){
    	if(p_list[3] == 'x'){
    		p_list[3] = 's';
    	}else{
    		p_list[3] = 'S';
    	}
    }
    if(m&02000){
    	if(p_list[6] == 'x'){
    		p_list[6] = 's';
    	}else{
    		p_list[6] = 'S';
    	}
    }
    
}

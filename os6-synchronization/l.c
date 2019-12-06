#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#define NSLOTS 100
#define cores 8
#define itera 1000000
struct dll
{
	char lock;
	int value;
	struct dll *fwd,*rev;
	int count;
};
struct slab
{
	char lock;
	char freemap[NSLOTS];
	struct dll slots[NSLOTS];
};
struct numtry
{
	char lock;
	int counter;	
};
int* p1;
char* p2;
void spin_lock(char *lock);
void spin_unlock(char *lock);
void testnolock();
void testhaslock();
void write_seqlock(struct dll *s);
void write_sequnlock(struct dll *s);
int read_seqbegin(struct dll *s);
bool read_seqretry(struct dll *s,int orig);
void *slab_alloc(struct slab *slab);
int slab_dealloc(struct slab *slab, void* object);
void problem1();
void problem5();
void problem6();
struct dll* dll_insert(struct dll *anchor, int value,struct slab *slab);
struct dll* dll_insert1(struct dll *anchor, int value,struct slab *slab);
void dll_delete(struct dll *anchor,struct dll *node, struct slab *slab);
void dll_delete1(struct dll *anchor,struct dll *node, struct slab *slab);
struct dll* dll_find1(struct dll *anchor, int value);
struct dll* dll_find(struct dll *anchor, int value);
void spin_lock(char *lock){
	while(tas(lock));
}
void spin_unlock(char *lock){
	*lock = 0;
}

int main(int argc, char *args[]){
	problem1();
	fprintf(stderr, "NSLOTS:%d\n",NSLOTS );
	fprintf(stderr, "Number of process:%d\n",cores);
	fprintf(stderr, "Number of iteration:%d\n",itera);
	problem5();
	problem6();
	return 0;
}
struct numtry* numbertry;
void problem6(){
	struct slab *p;
	void *v = mmap(NULL,sizeof(struct slab),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	p = (struct slab*)v;
	p->lock = 0;
	struct dll* anchor;
	if((anchor = slab_alloc(p)) == NULL){
		exit(EXIT_FAILURE);
	}
	void *v2 = mmap(NULL,sizeof(struct numtry),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	numbertry = (struct numtry*)v2;
	anchor->value = 0;
	anchor->rev = anchor;
	anchor->fwd = anchor;
	anchor->lock = 0; 
	anchor->count = 0;
	int i,j;
	struct timeval tv1;
	struct timeval tv2;
	struct timeval res;
	gettimeofday(&tv1,NULL);
	for(i = 0;i < cores; i++){
		pid_t id;
		if((id = fork()) == -1){
			fprintf(stderr, "Cannot fork(). Error:%s\n",strerror(errno));
		}else if(id == 0){
			srand(time(NULL));
			for(j = 0; j < itera;j ++){
				int val = (rand() % 100) + 1;
				int type = (rand() % (3)) + 1;
				if(type == 1){
					if(dll_insert(anchor,val,p) == NULL){
						//fprintf(stderr, "Cannot Perform insertion (in pid %d for %d)\n",getpid(),j);
					}
				}else if(type == 2){
					struct dll *node = &(p->slots[val%(NSLOTS-1)+1]);
					dll_delete(anchor,node,p);
				}else{
					if(dll_find(anchor,val) == NULL){
						//fprintf(stderr, "Cannot find the target node (in pid %d for %d)\n",getpid(),j);
					}
				}
			}
			exit(0);
		}
	}
	for(i = 0;i < cores; i++){
		wait(NULL);
	}
	gettimeofday(&tv2,NULL);
	struct dll* tem = anchor->fwd;
	while(tem!=anchor){
		fprintf(stderr, "%d\t",tem->value);
		tem = tem->fwd;
	}
	fprintf(stderr, "\n");
	timersub(&tv2,&tv1,&res);
	fprintf(stderr, "Time taken: %ld.%06ld\n",res.tv_sec,res.tv_usec);
	fprintf(stderr, "The number of retrys:%d\n",numbertry->counter);
	if(munmap(p,sizeof(struct slab)) == -1){
		fprintf(stderr, "Cannot call munmap. Error:%s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
}
void problem5(){
	struct slab *p;
	void *v = mmap(NULL,sizeof(struct slab),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	p = (struct slab*)v;
	p->lock = 0;
	struct dll* anchor;
	if((anchor = slab_alloc(p)) == NULL){
		exit(EXIT_FAILURE);
	}
	anchor->value = 0;
	anchor->rev = anchor;
	anchor->fwd = anchor;
	anchor->lock = 0; 
	anchor->count = 0;
	int i,j;
	struct timeval tv1;
	struct timeval tv2;
	struct timeval res;
	gettimeofday(&tv1,NULL);
	for(i = 0;i < cores; i++){
		pid_t id;
		if((id = fork()) == -1){
			fprintf(stderr, "Cannot fork(). Error:%s\n",strerror(errno));
		}else if(id == 0){
			srand(time(NULL));
			for(j = 0; j < itera;j ++){
				int val = (rand() % 100) + 1;
				int type = (rand() % (3)) + 1;
				if(type == 1){
					if(dll_insert1(anchor,val,p) == NULL){
						//fprintf(stderr, "Cannot Perform insertion (in pid %d for %d)\n",getpid(),j);
					}
				}else if(type == 2){
					struct dll *node = &(p->slots[val%(NSLOTS-1)+1]);
					dll_delete1(anchor,node,p);
				}else{
					if(dll_find1(anchor,val) == NULL){
						//fprintf(stderr, "Cannot find the target node (in pid %d for %d)\n",getpid(),j);
					}
				}
			}
			exit(0);
		}
	}
	for(i = 0;i < cores; i++){
		wait(NULL);
	}
	gettimeofday(&tv2,NULL);
	struct dll* tem = anchor->fwd;
	while(tem!=anchor){
		fprintf(stderr, "%d\t",tem->value);
		tem = tem->fwd;
	}
	fprintf(stderr, "\n" );
	timersub(&tv2,&tv1,&res);
	fprintf(stderr, "Time taken: %ld.%06ld\n",res.tv_sec,res.tv_usec);
	if(munmap(p,sizeof(struct slab)) == -1){
		fprintf(stderr, "Cannot call munmap. Error:%s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
}
void write_seqlock(struct dll *s){
	spin_lock(&s->lock);
	s->count++;
}
void write_sequnlock(struct dll *s){
	s->count++;
	spin_unlock(&s->lock);
}
int read_seqbegin(struct dll *s){
	int a;
	while(((a = s->count)%2) == 1){
		if(sched_yield() == -1){
			fprintf(stderr, "Cannot yield the process. Error:%s\n",strerror(errno));
		}
	}
	return a;
	// a should be 0
}
bool read_seqretry(struct dll *s,int orig){
	return s->count != orig;
}
void *slab_alloc(struct slab *slab){
	int i;
	spin_lock(&(slab->lock));
	for(i = 0; i < NSLOTS; i++){
		if(slab->freemap[i] == 0){
			slab->freemap[i] = 1;
			spin_unlock(&(slab->lock));
			return &slab->slots[i];
		}
	}
	spin_unlock(&(slab->lock));
	return NULL;
}
int slab_dealloc(struct slab *slab, void *object){
	int i;
	spin_lock(&(slab->lock));
	for(i = 0;i < NSLOTS; i++){
		if(&(slab->slots[i]) == object){
			if(slab->freemap[i] == 0){
				spin_unlock(&(slab->lock));
				return -1;
			}else{
				slab->freemap[i] = 0;
				spin_unlock(&(slab->lock));
				return 1;
			}
		}
	}
	spin_unlock(&(slab->lock));
	return -1;
}
struct dll* dll_insert1(struct dll *anchor, int value,struct slab *slab){
	spin_lock(&(anchor->lock));
	struct dll* node;
	if( (node = slab_alloc(slab) )== NULL){
		spin_unlock(&(anchor->lock));
		return NULL;
	}
	struct dll* pre = anchor;
	struct dll* index = anchor->fwd;
	node->value = value;
	while(index != anchor){
		if(index->value < value){
			index = index->fwd;
			pre = pre->fwd;
			continue;
		}else{
			break;
		}
	}
	node->rev = pre;
	pre->fwd = node;
	node->fwd = index;
	index->rev = node;
	spin_unlock(&anchor->lock);
	return node;
}

void dll_delete1(struct dll *anchor,struct dll *node, struct slab *slab){
	spin_lock(&(anchor->lock));
	if(node == NULL){
		spin_unlock(&anchor->lock);
		return;
	}
	if(slab_dealloc(slab,node) == -1){
		spin_unlock(&(anchor->lock));
		return;
	}
	struct dll* target = node;
	target->rev->fwd = target->fwd;
	target->fwd->rev = target->rev;
	spin_unlock(&(anchor->lock));

}
struct dll* dll_find1(struct dll *anchor, int value){
	spin_lock(&(anchor->lock));
	struct dll* index = anchor->fwd;
	while(index != anchor){
		if(index->value == value){
			spin_unlock(&(anchor->lock));
			return index;
		}
		index = index->fwd;
	}
	spin_unlock(&(anchor->lock));
	return NULL;
}
struct dll* dll_insert(struct dll *anchor, int value,struct slab *slab){
	write_seqlock(anchor);
	struct dll* node;
	if( (node = slab_alloc(slab) )== NULL){
		write_sequnlock(anchor);
		return NULL;
	}
	struct dll* pre = anchor;
	struct dll* index = anchor->fwd;
	node->value = value;
	while(index != anchor){
		if(index->value < value){
			index = index->fwd;
			pre = pre->fwd;
			continue;
		}else{
			break;
		}
	}
	node->rev = pre;
	pre->fwd = node;
	node->fwd = index;
	index->rev = node;
	write_sequnlock(anchor);
	return node;
}

void dll_delete(struct dll *anchor,struct dll *node, struct slab *slab){
	write_seqlock(anchor);
	if(node == NULL){
		write_sequnlock(anchor);
		return;
	}
	if(slab_dealloc(slab,node) == -1){
		write_sequnlock(anchor);
		return;
	}
	struct dll* target = node;
	target->rev->fwd = target->fwd;
	target->fwd->rev = target->rev;
	write_sequnlock(anchor);

}
struct dll* dll_find(struct dll *anchor, int value){
	beginning: 
	read_seqbegin(anchor);// if the return value is even
	struct dll* index = anchor->fwd;
	while(index != anchor){
		if(index->value == value){
			if(!read_seqretry(anchor,anchor->count)){
				return index;
			}else{
				goto beginning;
			}
		}
		index = index->fwd;
	}
	if(!read_seqretry(anchor,anchor->count)){
		return NULL;
	}else{
		spin_lock(&numbertry->lock);
		numbertry->counter++;
		spin_unlock(&numbertry->lock);
		goto beginning;
	}
	
}
void problem1(){
	p1 = mmap(NULL,sizeof(int*),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	if(p1 == MAP_FAILED){
		fprintf(stderr, "Cannot call mmap for p1. Error:%s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	p2 = mmap(NULL,sizeof(char*),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0);
	if(p2 == MAP_FAILED){
		fprintf(stderr, "Cannot call mmap for p2. Error:%s\n",strerror(errno));
		exit(EXIT_FAILURE);
	}
	*p1 = 0;
	testnolock();
	*p1 = 0;
	testhaslock();
}
void testnolock(){
	fprintf(stderr, "Testing without lock\n");
	fprintf(stderr, "The value should be 8000000\n");
	int i,j;
	for(i = 0; i < cores; i++){
		pid_t pid;
		if((pid = fork()) == -1){
			fprintf(stderr, "Cannot call fork(). Error:%s\n",strerror(errno));
			continue;
		}else if(pid == 0){
			for(j = 0; j < 1000000; j++){
				*p1 = *p1+1;
			}
			exit(EXIT_SUCCESS);
 		}

	}
	for(i = 0; i < cores; i++){
		wait(NULL);
	}
	fprintf(stderr, "The result is %d\n",*p1);

}
void testhaslock(){
	fprintf(stderr, "Testing with lock\n");
	fprintf(stderr, "The resulting value is 8000000\n");
	int i,j;
	for(i = 0; i < cores; i++){
		pid_t pid;
		if((pid = fork()) == -1){
			fprintf(stderr, "Cannot call fork(). Error:%s\n",strerror(errno));
			continue;
		}else if(pid == 0){
			for(j = 0; j < 1000000; j++){
				spin_lock(p2);
				*p1 = *p1+1;
				spin_unlock(p2);
			}
			exit(EXIT_SUCCESS);
 		}

	}
	for(i = 0; i < cores; i++){
		wait(NULL);
	}
	fprintf(stderr, "The result is %d\n",*p1); 
}




#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/mman.h>

#define SIZE 1

int main(){
	
	//shm setup for hotel manager

	int shm_fd=-1;
	void * ptr=NULL;
	char *name="admin";

	shm_fd=shm_open(name, O_CREAT|O_RDWR, 0666);
	if(shm_fd==-1){
		printf("segemnt not created yet, terminating");
		exit(1);
	}
	ftruncate(shm_fd, SIZE);
	ptr=mmap(0,SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if(ptr==(void*)-1){
		printf("Error while mapping shm, terminating");
		exit(1);
	}
	
	*(int*)ptr=0;

	while(1){
		char decision;
		char waste;
        
		printf("Do you want to close the hotel? Enter Y for Yes and N for NO: ");
		scanf("%c",&decision);	
		scanf("%c",&waste); //to clear input buffer
		
		if(decision=='Y'||decision=='y'){
			*(int*)ptr=1;
			break;
		}
		else if(decision=='N'||decision=='n'){
			continue;
		}
		else{
			printf("Invalid input. Please enter Y or N\n");
		}		
	}

	sleep(2); //wait for hotel manager to read
	shm_unlink(name);
	return 0;
}

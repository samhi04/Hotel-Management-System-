#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <limits.h>

#define SIZE 1024
#define BUFFER_SIZE 1024
#define READ_END 0
#define WRITE_END 1

int main(){
	
	char table_number;
	printf("Enter Table Number: ");
	scanf("%c",&table_number);
	
	char name[11]="/segment_"; //name for the shared memory segement
	name[9]=table_number;
	
	//shared memory object setup

	int shm_fd;
	void *ptr,*ptr_store;
	shm_fd=shm_open(name, O_CREAT|O_RDWR, 0666);
	if(shm_fd==-1){
		printf("Couldnt find/create segement, terminating\n");
		exit(1);
	}
	ftruncate(shm_fd, SIZE);
	ptr=mmap(0,SIZE,PROT_WRITE|PROT_READ,MAP_SHARED,shm_fd,0);
	if(ptr==(void*)-1){
		printf("error while mapping shm object terminating\n");
		exit(1);
	}
	ptr_store=ptr;
	*(int*)ptr=-1; //to prevent auto execution;
	
	int hotel_close =0; //changed by data from waiter
	int customer_count=0;
	printf("Enter Number of Customers at Table (maximum no. of customers can be 5):  ");
	scanf("%d",&customer_count);
	int run_count=0;
	do{

		//creating pipes 

		int *pipes[customer_count];
		for(int i=0;i<customer_count;i++){
			int *fd=(int*)malloc(sizeof(int)*2);
			if(pipe(fd)==-1){
				printf("Pipe creation failed, exiting");
				exit(1);
			}
			pipes[i]=fd;
		}

		//file io

		FILE *fptr;
		fptr=fopen("menu.txt","r");
		if(fptr==NULL){
			printf("File not found\n");
			return 1;
		}
		char menu_item[50];
		while(fgets(menu_item,50,fptr)!= 0){
			int i=0;
			while(menu_item[i]!='\0'){
				printf("%c",menu_item[i++]);
			}
		}
		fclose(fptr);

		//creating and executing child processes

		pid_t pid=getpid();
		for(int i=0;i<customer_count;i++){
			pid=fork();
			if(pid==0){
				//sleep to ensure the parent completes its tasks
				
				sleep(0.1);

				printf("Enter the serial number(s) of the item(s) to order from the menu. Enter -1 when done: \n");
				int write_msg[BUFFER_SIZE];
				int count=0;
				int temp=0;
				int j=0;
				while(temp!=-1){
					scanf("%d",&temp);
					write_msg[j++]=temp;
				}
				//IPC using pipe
				close(pipes[i][READ_END]);
				write(pipes[i][WRITE_END],write_msg,sizeof(int)*j);
				close(pipes[i][WRITE_END]);
				return 0;
			}
			if(pid==0) break;
			int status;
			pid_t exited_child = wait(&status);
		}
		
		//IPC using pipe
		for(int i=0;i<customer_count;i++){
			close(pipes[i][WRITE_END]);
			int read_msg[BUFFER_SIZE];
			int b=read(pipes[i][READ_END], read_msg, BUFFER_SIZE);
			close(pipes[i][READ_END]);
			//doing stuff to received data
			for(int i=0;read_msg[i]!=-1;i++){
				*(int*)ptr=read_msg[i];
				ptr+=sizeof(int); 
			}
		}
		*(int*)ptr=INT_MIN; //check for customer orders end;
		sleep(1); //wait for waiter to complete calculation
		ptr+=sizeof(int);
		//setup of flag to ensure writing is done before proceding
		if(run_count<1){
			*(int*)ptr=-100;
			while(*(int*)ptr==-100){
				sleep(0.1);
			}	
		}
		int bill=*(int*)ptr;

		printf("Thank you for dining with us. Here is your bill: %d\n", bill);

		// free the pointers used to store the file descriptors
		for(int i=0;i<customer_count;i++){
			free(pipes[i]);
		}
		sleep(2); //to ensure conversation between hotel manager and the waiter is complete	
		ptr_store+=sizeof(int)*250;
		if(*(int*)ptr_store!=1){
			printf("Enter Number of Customers at Table (maximum no. of customers can be 5):  ");
			scanf("%d",&customer_count);
			if(customer_count==-1){
				hotel_close=1;
				ptr_store+=sizeof(int);
				*(int*)ptr_store=1; //implies that the table is now closing, and the waiter must terminate too
				ptr_store-=sizeof(int);
				ptr_store-=sizeof(int)*250;
				ptr_store+=sizeof(int)*240;
				*(int*)ptr_store=1;
				ptr_store-=sizeof(int)*240;
				ptr_store+=sizeof(int)*250;
			}
			else{
				ptr_store+=sizeof(int);
				*(int*)ptr_store=2; //implies that the table is repeating task, the waiter must do too
				ptr_store-=sizeof(int);
				ptr_store-=sizeof(int)*250;
				ptr_store+=sizeof(int)*240;
				*(int*)ptr_store=1;
				ptr_store-=sizeof(int)*240;
				ptr=ptr_store; //reset pointer for repeating tasks
				*(int*)ptr==-1; //to prevent auto execution of waiter
			}
		}
		else{
			hotel_close=1;
			ptr_store+=sizeof(int);
			*(int*)ptr_store=1; //implies that the table is now closing, and the waiter must terminate too
			ptr_store-=sizeof(int)*251;
			ptr_store+=sizeof(int)*240;
			*(int*)ptr_store=1;
			ptr_store-=sizeof(int)*240;
		}
		sleep(1);
		run_count++;
		ptr_store+=sizeof(int)*240;
		*(int*)ptr_store=0;
		ptr_store-=sizeof(int)*240;
	}while(hotel_close==0);
	sleep(1);// to ensure waiter reads updates before program terminates
	return 0;	
}

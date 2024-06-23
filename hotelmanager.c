#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <stdbool.h>

#define SIZE 128
#define MAX_TABLES 100
#define EARNINGS_FILE "earnings.txt"
#define DEF_NAME "waiter_"

int hotel_open=1;

typedef struct waiter_info{
	int shm_fd;
	char *segment_name;
	void *ptr;
	} w;
    
int main() 
    {
        printf("Enter the number of tables: ");
	int table_count;
	scanf("%d",&table_count);
	w *waiter=(w *)malloc(sizeof(w)*table_count);
	
	//initialising shared memory for waiter-hotelmanager
	int i;
	for(i=0;i<table_count;++i){
		
		waiter[i].segment_name=(char*)malloc(sizeof(char)*9);
		waiter[i].segment_name[0]='w';
		waiter[i].segment_name[1]='a';
		waiter[i].segment_name[2]='i';
		waiter[i].segment_name[3]='t';
		waiter[i].segment_name[4]='e';
		waiter[i].segment_name[5]='r';
		waiter[i].segment_name[6]='_';
		waiter[i].segment_name[7]=i+'0'+1;
		waiter[i].segment_name[8]='\0';
		waiter[i].shm_fd=shm_open(waiter[i].segment_name, O_CREAT|O_RDWR, 0666);
		ftruncate(waiter[i].shm_fd,SIZE);
		waiter[i].ptr=mmap(0, SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, waiter[i].shm_fd, 0);
	}
		
	for(int i=0;i<table_count;i++){
		void *p=waiter[i].ptr;
		*(int*)p=-1;
	}

	int j=0;
	int totalEarnings = 0;

   	//opening file for IO 

	FILE *earningsFile = fopen(EARNINGS_FILE, "w");
	if(earningsFile==NULL){
		printf("error while creating or accessing file, terminating");
		exit(1);
	}

	//shared memory setup for admin-hotelmanager comms

	int admin_shm_fd=-1;
	
	while(admin_shm_fd==-1){
		admin_shm_fd=shm_open("admin", O_RDWR, 0666);
		if(admin_shm_fd==-1){
			printf("Admin is not active, retrying");
		}
	}
	ftruncate(admin_shm_fd,1);
	void* admin_ptr=mmap(0, 1, PROT_READ|PROT_WRITE, MAP_SHARED, admin_shm_fd, 0);
	if(admin_ptr==(void*)-1){
		printf("cannot map shared memory object, terminating");
		exit(1);
	}
		
	while(hotel_open==1){
		for(i=0;i<table_count;i++){
			if(*(int*)(waiter[i].ptr) != -1){
				int money=*(int*)(waiter[i].ptr);
				printf("Bill: %d\n",money);
				fprintf(earningsFile, "Earning from Table %d: %d INR\n", i+1,money);
				totalEarnings += money;
				printf("Total: %d\n",totalEarnings);
				void *p=waiter[i].ptr;
				*(int*)p=-1;
			}
		}
		if(*(int*)admin_ptr==1) {
			hotel_open=0;
			for(int k=0;k<table_count;k++){
				waiter[k].ptr+=sizeof(int)*2;
				void*p=waiter[k].ptr;
				*(int*)(p)=1; //1 here means the hotel is closing and more customers will not be accepted by the table
				waiter[k].ptr-=sizeof(int)*2;
			}
			for(int k=0;k<table_count;k++){
				waiter[k].ptr+=sizeof(int);
				void*q=waiter[k].ptr;
				if(*(int*)q==1){ //check if customers are there
					while(*(int*)q==1) sleep(0.1);
				}
				waiter[k].ptr-=sizeof(int);
				sleep(0.3); //wait for waiter to complete write
				//add final entry for this table into the earnings file
				void *r=waiter[k].ptr;
				if(*(int*)(r) != -1){
					int money=*(int*)(r);
					printf("Bill: %d\n",money);
					fprintf(earningsFile, "Earning from Table %d: %d INR\n", k+1,money);
					totalEarnings += money;
					printf("Total: %d\n",totalEarnings);
					void *s=waiter[k].ptr;
					*(int*)s=-1; //resetting bill to -1
				}
			}
		}
	}
	
        int totalWages = totalEarnings * 0.4;
        int totalProfit = totalEarnings - totalWages;

        printf("Total Earnings of Hotel: %d INR\n", totalEarnings);
        printf("Total Wages of Waiters: %d INR\n", totalWages);
        printf("Total Profit: %d INR\n", totalProfit);
            
        fprintf(earningsFile, "Total Earnings of Hotel: %d INR\n", totalEarnings);
        fprintf(earningsFile, "Total Wages of Waiters: %d INR\n", totalWages);
        fprintf(earningsFile, "Total Profit: %d INR\n", totalProfit);
   
        fclose(earningsFile);	
         
        printf("Thank you for visiting the Hotel!\n");
        
	for(int i=0;i<table_count;i++){
		shm_unlink(waiter[i].segment_name);
	}
	
	return 0;
    }
        

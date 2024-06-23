#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define HM_SIZE 128
#define SIZE 1024
#define BUFFER_SIZE 1024

typedef struct menu_items{
	int item_number;
	int price;
} mi;

int main(){
	
	//name setup for shared mem

	printf("Enter waiter ID: ");
	char id;
	scanf("%c",&id);
	//name for table shared memory
	char name[11]="/segment_";
	name[9]=id;
	//name for hotel manager shared memory
	char waiter_name[9]="waiter_";
	waiter_name[7]=id;
	
	//file io

	FILE *fptr=NULL;
	fptr=fopen("menu.txt","r");
	if(fptr==NULL){
		printf("File not found, terminating\n");
		exit(1);
	}
	char menu_item[50];
	mi menu[100];
	int i=0;  // number of menu items
	char item_id[2]; //can handle menu items upto 10
	item_id[1]='\0';
	char item_price[4]; //can handle prices upto 999
	for(int j=0;j<4;j++) item_price[j]='\0';
	//read data from the file, max 100 menu_items can be allowed.
	while(fgets(menu_item,50,fptr)!=0){
		item_id[0]=menu_item[0];
		//get menu_item number and store it in array of structures
		menu[i].item_number=atoi(item_id);
		int num_count=0;
		int size=strlen(menu_item);
		//loop through the menu_item array till a number other than the one aldready encountered is found, then store it in array of structures
		for(int j=0;j<size;j++){
			if(menu_item[j]>=48 && menu_item[j]<=57){
				num_count++;
				if(num_count>1&&num_count<5){
					item_price[num_count-2]=menu_item[j];
				}
			}
		}
		menu[i].price=atoi(item_price);
		i++;
	}

	//shared memory setup for table

	int shm_fd=-1;
	void *ptr,*ptr_store;

	shm_fd=shm_open(name, O_RDWR, 0666);
	if(shm_fd==-1){
		printf("Segement doesn't exist yet, terminating\n");
		exit(1);
	}
	ptr=mmap(0, SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
	ptr_store = ptr; //used to reset ptr if needed
	if(ptr==(void*)-1){
		printf("Unalbe to map shared memory object, terminating\n");
		exit(1);
	}
	
	//shared memory setup for hotel manager

	int shm_fd_hm=-1;
	void *hm_ptr=NULL;

	shm_fd_hm=shm_open(waiter_name, O_CREAT|O_RDWR, 0666);
	if(shm_fd_hm==-1){
		printf("hotel segment not created yet, terminating");
		exit(1);
	}

	hm_ptr=mmap(0, HM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd_hm, 0);

	if(hm_ptr==(void*)-1){
		printf("Unable to map hotel manager segment, terminating");
		exit(1);
	}
	
	int customers_present=1;
	
	do{
		hm_ptr+=sizeof(int);
		*(int*)hm_ptr=1; //1 means there are customers aka waiter is active
		hm_ptr-=sizeof(int);
		// reading data from table shm
		while(*(int*)ptr==-1) sleep(0.1);
		int *order_list=(int*)malloc(sizeof(int)*i);
		for(int j=0;j<i;j++){
			order_list[j]=0;
		}

		while(*(int*)ptr!=INT_MIN){
			int temp = *(int*)ptr;
			if(temp<=i){
				order_list[temp-1]++;
			}
			else printf("invalid entry: %d\n",temp);
			ptr+=sizeof(int);
		}
		
		ptr+=sizeof(int);
		
		//calculation of bill

		int bill=0;
		for(int j=0;j<i;j++){
			bill+=(order_list[j]*(menu[j].price));
		}
		
		*(int*)ptr=bill;
		ptr_store+=sizeof(int)*240;
		while(*(int*)ptr_store!=1) sleep(0.1);
		ptr_store-=sizeof(int)*240;
		ptr_store+=sizeof(int)*251;
		if(*(int*)ptr_store==1){
			customers_present=0;
			hm_ptr+=sizeof(int);
			*(int*)hm_ptr=0; //1 means there are customers aka waiter is active
			hm_ptr-=sizeof(int);
		}else if(*(int*)ptr_store==2){ //means that the table is repeating, and the waiter must too
			ptr_store-=sizeof(int)*251;
			ptr=ptr_store;
			*(int*)ptr=-1; //set to stop auto execution
			printf("Table still active\n");
		}
		*(int*)hm_ptr=bill;
		printf("Bill Amount for Table %d: %d \n",id-'0',bill);

		
		//reading data from hotel manager shared memeory
		
		
		hm_ptr+=sizeof(int)*2;
		if(*(int*)hm_ptr==1){ //means that the hotel is closing
			ptr_store+=sizeof(int)*250;
			*(int*)ptr_store=1; //1 at the 1000-1003 int means that the hotel is closing and the table must not accept any new guests
			ptr_store-=sizeof(int)*250;
		}
		hm_ptr-=sizeof(int)*2;
	}while(customers_present==1);
	
	hm_ptr+=sizeof(int);
	*(int*)hm_ptr=0;
	sleep(2);  //to ensure that table and hotel manager read it before unlinking
	shm_unlink(waiter_name); //donot unlink this to prevent deletion of shared memory
	shm_unlink(name);
	

	return 0;
}

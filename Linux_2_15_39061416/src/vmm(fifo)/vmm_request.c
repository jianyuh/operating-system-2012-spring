#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include "vmm_global.h"

void welcomeVMMRequest()
{
        printf("\n//////////////////////////////////////////////\n");
        printf("\tWelcome to VMM 1.0 Request Part\n");
        printf("\tAuthor:39061416\tHuang Jianyu\n");
        printf("\tWebsite:www.huangjy.info\n");
        printf("\tMobile Phone:15210965935\n");
        printf("\tEmail:hjyahead@gmail.com\n");
        printf("///////////////////////////////////////////////\n");
        printf("\n\n");
}

void do_request(MemoryAccessRequestPtr ptr_memAccRequest)
{
	//int randnum;
	//randnum = rand();
	//printf("%d\n",randnum);
	ptr_memAccRequest->virtual_Addr= rand() % VIRTUAL_MEMORY_SIZE;
	
	
	//printf("%lu\n",ptr_memAccRequest->virtual_Addr);

	switch (rand()%3)
	{
	case 0:			//read
		ptr_memAccRequest->request_type=READ;
		printf("Generate request:\nAddress:%lu\tType:read\n",ptr_memAccRequest->virtual_Addr);
		break;
	case 1:			//write
		ptr_memAccRequest->request_type=WRITE;
		ptr_memAccRequest->value=rand()%MAX_VALUE;
		printf("Generate request:\nAddress:%lu\tType:write\tvalue:%02X\n",ptr_memAccRequest->virtual_Addr,ptr_memAccRequest->value);
		break;
	case 2:			//execute
		ptr_memAccRequest->request_type=EXECUTE;
		printf("Generate request:\nAddress:%lu\tType:execute\n",ptr_memAccRequest->virtual_Addr);
		break;
	default:
		break;
	}	
}


int main()
{
	int write_num,i=0,n=0;
	char c;
	FILE *fp;
	welcomeVMMRequest();
	printf("Please input the number of the requests:\n");
	scanf("%d",&n);
	srand(time(NULL));
	MemoryAccessRequestPtr ptr_memAccRequest0;
	ptr_memAccRequest0=(MemoryAccessRequestPtr)malloc(sizeof(MemoryAccessRequest));
	//mkfifo(FIFO,S_IFIFO|0666,0);
	
	if((mkfifo(FIFO,S_IFIFO|0666))==-1)//Žò¿ªFIFO£¬¶ÁÈ¡ÊýŸÝ
	{
		//printf("can not mkfifo %s\n",FIFO);
	}
	
	
	while(i<n)
	{
		if((fp=fopen(FIFO,"a+"))==NULL)
		{
			//handle_error(FILE_OPEN_FAILED);
			exit(1);
		}
		do_request(ptr_memAccRequest0);
		printf("Success request : %d\n",++i);
		write_num=fwrite(ptr_memAccRequest0,sizeof(MemoryAccessRequest),1,fp);
		fflush(fp);
	}
	fclose(fp);
	return 0;
}

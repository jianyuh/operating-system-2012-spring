#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include "vmm_global.h"
#include "vmm_pagereplace.h"

void welcomeVMMResponse()
{
        printf("\n//////////////////////////////////////////////\n");
        printf("\tWelcome to VMM 1.0 Response Part\n");
        printf("\tAuthor:39061416\tHuang Jianyu\n");
        printf("\tWebsite:www.huangjy.info\n");
        printf("\tMobile Phone:15210965935\n");
        printf("\tEmail:hjyahead@gmail.com\n");
        printf("///////////////////////////////////////////////\n");
        printf("\n\n");
}

void init()
{
	int i,j,n;
	srand(time(NULL));
	lengthOfSeq=0;
	exec_times=0;
	for(n=0;n<OUTER_PAGE_TOTAL;n++)
	{
		outerpagetable[n].page_num=n;
		outerpagetable[n].index_num=n*INNER_PAGE_TABLE_SIZE;

		for(i=n*PAGE_SIZE;i<(n+1)*PAGE_SIZE&&i<PAGE_TOTAL;i++)
		{
			pagetable[i].page_num=i;
			pagetable[i].filled=FALSE;
			pagetable[i].changed=FALSE;
			pagetable[i].count=0;

			//set the protect type with the random number
			switch (rand()%7)
			{
			case 0:
				pagetable[i].pro_type=READABLE;
				break;
			case 1:
				pagetable[i].pro_type=WRITABLE;
				break;
			case 2:
				pagetable[i].pro_type=EXECUTABLE;
				break;
			case 3:
				pagetable[i].pro_type=READABLE|WRITABLE;
				break;
			case 4:
				pagetable[i].pro_type=READABLE|EXECUTABLE;
				break;
			case 5:
				pagetable[i].pro_type=WRITABLE|EXECUTABLE;
				break;
			case 6:
				pagetable[i].pro_type=READABLE|WRITABLE|EXECUTABLE;
				break;
			default:
				break;
			}
			pagetable[i].virtual_Addr=i*PAGE_SIZE*2;//the disk address mapping the page table	
		}
	}
	for (j=0;j<BLOCK_TOTAL;j++)
	{
		//select some physical blocks to move the page from the disk to the actual memory randomly
		if(rand()%2==0)
		{
			do_page_in(&pagetable[j],j);
			pagetable[j].block_num=j;
			pagetable[j].filled=TRUE;
			block_status[j]=TRUE;
			FIFOSeq[lengthOfSeq++]=pagetable[j].page_num;
		}
		else
			block_status[j]=FALSE;
	}

	//set process id and related page table number

	
	pcb[0].pid=1;
	pcb[0].start=0;
	pcb[0].end=3;
	pcb[1].pid=2;
	pcb[1].start=4;
	pcb[1].end=7;
	
	pcb[2].pid=3;
	pcb[2].start=8;
	pcb[2].end=11;
	pcb[3].pid=4;
	pcb[3].start=12;
	pcb[3].end=15;
	
	
}

void do_response()
{
	PageTablePtr ptable;
	unsigned int outer_page_num,in_page_offset,offset,in_page_num,actual_address,i;

	//whether over boundary
	if(ptr_memAccRequest->virtual_Addr<0||ptr_memAccRequest->virtual_Addr>VIRTUAL_MEMORY_SIZE)
	{
		handle_error(OVER_BOUNDARY);
		return;
	}

	outer_page_num=(ptr_memAccRequest->virtual_Addr/PAGE_SIZE)/INNER_PAGE_TABLE_SIZE;
	in_page_offset=(ptr_memAccRequest->virtual_Addr/PAGE_SIZE)%INNER_PAGE_TABLE_SIZE;
	offset=ptr_memAccRequest->virtual_Addr%PAGE_SIZE;

	for(i=0;i<PID_NUM;++i)
	{
		if(outer_page_num>=pcb[i].start && outer_page_num<=pcb[i].end)
		{
			process_id=pcb[i].pid;
		}
	}

	//actually, in_page_num == page num == (ptr_memAccRequest->virtual_Addr/PAGE_SIZE)
	in_page_num=outerpagetable[outer_page_num].index_num+in_page_offset;
	printf("The process id: %u\nThe page(outer) number:%u\tThe page(inner) number:%u\tThe offset:%u\n",process_id,outer_page_num,in_page_num,offset);

	ptable=&pagetable[in_page_num];

	//whether to produce the page fault according to the related flag
	if(!ptable->filled)
	{
		do_page_fault(ptable);
	}

	ptable->no_use=exec_times++;				//for the LRU algorithom:page exchange
	actual_address=ptable->block_num*PAGE_SIZE+offset;
	printf("The actual address is: %u\n",actual_address);

	//check the page access rights and deal with memory access request
	switch (ptr_memAccRequest->request_type)
	{
	case READ:
		{
			ptable->count++;
			if(!(ptable->pro_type&READABLE))	//can not be read
			{
				handle_error(READ_DENY);
				return;
			}
			//read from the actual memory
			printf("Success to read: The value is %02X\n",actual_memory[actual_address]);
			break;
		}
	case WRITE:
		{
			ptable->count++;
			if(!(ptable->pro_type&WRITABLE))	// can not be written
			{
				handle_error(WRITE_DENY);	
				return;
			}
			//write the requested data to the actual memory
			actual_memory[actual_address]=ptr_memAccRequest->value;
			ptable->changed=TRUE;			
			printf("Success to write!\n");
			break;
		}
	case EXECUTE:	
		{
			ptable->count++;
			if(!(ptable->pro_type&EXECUTABLE))	//can not be executed
			{
				handle_error(EXECUTE_DENY);
				return;
			}			
			printf("Success to execute!\n");
			break;
		}
	default:									//illegal memory access request
		{	
			handle_error(INVALID_REQUEST);
			return;
		}
	}
}

//deal with the page fault
void do_page_fault(PageTablePtr ptable)
{
	unsigned int i;
	char c;
	for(i=0;i<BLOCK_TOTAL;i++)
	{
		if(!block_status[i])
		{
			//read from the disk, and write to the actual memory
			do_page_in(ptable, i);

			//refresh the page table
			ptable->block_num = i;
			ptable->filled = TRUE;
			ptable->changed = FALSE;
			ptable->count = 0;

			block_status[i] = TRUE;
			return;
		}
	}

	//no free physical block, do page exchange
	printf("Please choose a method to do the page exchange algorithm,and press '1' for FIFO, '2' for LFU, '3' for LRU...\n");
	while(c=getchar())
	{
		if(c=='1')
		{
			do_FIFO(ptable);
			break;
		}
		else if(c=='2')
		{
			do_LFU(ptable);
			break;
		}
		else if(c=='3')
		{
			do_LRU(ptable);
			break;
		}
	}
}

//move the contents in the disk into the actual memory
void do_page_in(PageTablePtr ptable,unsigned int block_num)
{
	unsigned int read_num;
	if(fseek(auxmem_ptr,ptable->virtual_Addr,SEEK_SET)<0)
	{
#ifdef DEBUG
		printf("DEBUG:auxAddr=%u\tftell=%u\n",ptable->virtual_Addr,ftell(auxmem_ptr));
#endif
		handle_error(FILE_SEEK_FAILED);
		exit(1);
	}
	if((read_num=fread(&actual_memory[block_num*PAGE_SIZE],sizeof(unsigned char),PAGE_SIZE,auxmem_ptr))<PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG:auxAddr=%u\tftell=%u\n",ptable->virtual_Addr,ftell(auxmem_ptr));
		printf("DEBUG:blockNum=%u\treadNum=%u\n",block_num,read_num);
		printf("DEBUG:feof= %d\tferror=%d\n",feof(auxmem_ptr),ferror(auxmem_ptr));
#endif
		handle_error(FILE_READ_FAILED);
		exit(1);
	}
	printf("Read page success: auxiliary memory address %lu -->> actual block %u\n",ptable->virtual_Addr,block_num);
}

//move the contents in the actual memory out to the disk
void do_page_out(PageTablePtr ptable)
{
	unsigned int write_num;
	if(fseek(auxmem_ptr,ptable->virtual_Addr,SEEK_SET)<0)
	{
#ifdef DEBUG
		printf("DEBUG:auxAddr=%u\tftell=%u\n",ptable->virtual_Addr,ftell(auxmem_ptr));
#endif
		handle_error(FILE_SEEK_FAILED);
		exit(1);
	}
	if((write_num=fwrite(&actual_memory[ptable->block_num*PAGE_SIZE],sizeof(unsigned char),PAGE_SIZE,auxmem_ptr))<PAGE_SIZE)
	{
#ifdef DEBUG
		printf("DEBUG:auxAddr=%u\tftell=%u\n",ptable->virtual_Addr,ftell(auxmem_ptr));
		printf("DEBUG:readNum=%u\n",write_num);
		printf("DEBUG:feof= %d\tferror=%d\n",feof(auxmem_ptr),ferror(auxmem_ptr));
#endif
		handle_error(FILE_WRITE_FAILED);
		exit(1);
	}
	printf("Write back success: actual block %lu -->> auxiliary address %03X\n",ptable->virtual_Addr,ptable->block_num);
}

void FIFOSeq_change(unsigned int num)
{
	int i;
	for(i=0;i<lengthOfSeq-1;++i)
		FIFOSeq[i]=FIFOSeq[i+1];
	FIFOSeq[lengthOfSeq]=num;
}

void handle_error(ErrorType error_type)
{
	switch (error_type)
	{
	case READ_DENY:
		printf("Access error: the address can not be read!\n");
		break;
	case WRITE_DENY:
		printf("Access error: the address can not be written!\n");
		break;
	case EXECUTE_DENY:
		printf("Access error: the address can not be executed!\n");
		break;
	case INVALID_REQUEST:
		printf("Access error: illegal request!\n");
		break;
	case OVER_BOUNDARY:
		printf("Access error: the address is over boundary!\n");
		break;
	case FILE_OPEN_FAILED:
		printf("System error: fail to open the file!\n");
		break;
	case FILE_CLOSE_FAILED:
		printf("System error: fail to close the file!\n");
		break;
	case FILE_SEEK_FAILED:
		printf("System error: fail to seek the file!\n");
		break;
	case FILE_READ_FAILED:
		printf("System error: fail to read the file!\n");
		break;
	case FILE_WRITE_FAILED:
		printf("System error: fail to write the file!\n");
		break;
	default:
		printf("Unknown error: no such error code!\n");
		break;
	}
}

//print the page table information
void print_pageinfo()
{
	unsigned int i,j,m;
	unsigned char str[4];
	printf(
                "---------------------------------------------------------------------------\n");
	printf("| %7s | %7s | %7s | %6s | %7s | %7s | %3s | %7s |\n","oPageNO","iPageNO","blockNO","Filled","Changed","ProType","Cnt","Aux mem");
	
	
	printf(
                "---------------------------------------------------------------------------\n");
	for(i=0;i<OUTER_PAGE_TOTAL;++i)
	{
		for(j=0;j<INNER_PAGE_TABLE_SIZE;++j)
		{
			m=outerpagetable[i].index_num+j;
			
			
			printf("| %7u | %7u | %7u | %6u | %7u | %7s | %3lu | %7lu |\n", i, pagetable[m].page_num,pagetable[m].block_num, pagetable[m].filled, 
				pagetable[m].changed, get_protype_str(str, pagetable[m].pro_type), 
				pagetable[m].count, pagetable[m].virtual_Addr);
		}
	}
	printf(
                "---------------------------------------------------------------------------\n");
}

//get the protect type string
char *get_protype_str(char *str,unsigned char type)
{
	if (type & READABLE)
		str[0] = 'r';
	else
		str[0] = '-';
	if (type & WRITABLE)
		str[1] = 'w';
	else
		str[1] = '-';
	if (type & EXECUTABLE)
		str[2] = 'x';
	else
		str[2] = '-';
	str[3] = '\0';
	return str;
}



int main()
{
	char c;
	int i,read_num=0;
	FILE *fp;

	if(!(auxmem_ptr=fopen(AUXILIARY_SPACE_FILE,"rw+")))
	{
		handle_error(FILE_OPEN_FAILED);
		exit(1);
	}

	welcomeVMMResponse();
	init();
	print_pageinfo();
	ptr_memAccRequest=(MemoryAccessRequestPtr)malloc(sizeof(MemoryAccessRequest));
	umask(0);									//file mask
	//mkfifo(FIFO,S_IFIFO|0666);				//open fifo and read data
	//if((mkfifo(FIFO,S_IFIFO|0666))==-1)//Žò¿ªFIFO£¬¶ÁÈ¡ÊýŸÝ
	//{
	//	printf("can not mkfifo %s\n",FIFO);
	//}
	
	if((fp=fopen(FIFO,"r"))==NULL)//open FIFO
	{
		
		printf("\nCannot open %s, you should run request part first\n\n",FIFO);
		exit(errno);				
	}

	//simulate the memory access request and response
	while(TRUE)
	{
		//do_request();

		if((read_num=fread(ptr_memAccRequest,sizeof(MemoryAccessRequest),1,fp))==0)
		{
			printf("Please generate memory access request...\n");
			printf("(Press 'C' to cancel, Press others to continue...)\n");
			if((c=getchar())=='c'||c=='C')
				break;
			else
				continue;
		}
		if(ptr_memAccRequest->request_type==READ)
			printf("The request I get from IPC(file):\nAddress:%lu\tType:read\n",ptr_memAccRequest->virtual_Addr);
		else if (ptr_memAccRequest->request_type==WRITE)
			printf("The request I get from IPC(file):\nAddress:%lu\tType:write\tvalue:%02X\n",ptr_memAccRequest->virtual_Addr,ptr_memAccRequest->value);
		else if(ptr_memAccRequest->request_type==EXECUTE)
			printf("The request I get from IPC(file):\nAddress:%lu\tType:execute\n",ptr_memAccRequest->virtual_Addr);		
		
		do_response();
		printf("Press 'P' to print the page-table...\n");
		if((c=getchar())=='p'||c=='P')
			print_pageinfo();
		while(c!='\n')
			c=getchar();
		printf("Press 'C' to cancel, Press others to continue...\n");
		if((c=getchar())=='c'||c=='C')
			break;
		while(c!='\n')
			c=getchar();
	}

	fclose(fp);
	if(fclose(auxmem_ptr)==EOF)
	{
		handle_error(FILE_CLOSE_FAILED);
		exit(1);
	}
	return 0;
}

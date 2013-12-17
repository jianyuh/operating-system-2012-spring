#include <stdio.h>
#include <stdlib.h>
#include "vmm_global.h"

//page change:Least Frequency Use
void do_LFU(PageTablePtr ptable)
{
	unsigned int i,min_use,page;
	printf("There is no idle block.Do LFU:\n ");
	min_use=0xFFFFFFFF;							//init the least frequency
	page=0;
	for(i=0;i<PAGE_TOTAL;i++)
	{
		if(pagetable[i].count<min_use)
		{
			min_use=pagetable[i].count;
			page=pagetable[i].page_num;
		}
	}
	printf("Replace the %uth page.\n",page);
	if(pagetable[page].changed)
	{
		//there are some changes, which need to write back to the disk
		printf("The page to be replaced has been changed:Write back.\n");
		do_page_out(&pagetable[page]);
	}
	pagetable[page].changed=FALSE;
	pagetable[page].count=0;
	pagetable[page].filled=FALSE;


	do_page_in(ptable,pagetable[page].block_num);

	//refresh the page table
	ptable->block_num=pagetable[page].block_num;
	ptable->changed=FALSE;
	ptable->count=0;
	ptable->filled=TRUE;

	printf("Page exchange Success!\n");
}

//page change:Fisrt in, First out
void do_FIFO(PageTablePtr ptable)
{
	unsigned int firstcome;

	//FIFOSeq[PAGE_TOTAL]  ---->>>> the page fifo of the actual memory, each is page number and the first is the first do_page_in page. 
	firstcome=FIFOSeq[0];
	printf("There is no idle block.Do FIFO:\n ");
	printf("Replace the %uth page.\n",firstcome);
	if(pagetable[firstcome].changed)
	{
		printf("The page to be replaced has been changed:Write back.\n");
		do_page_out(&pagetable[firstcome]);
	}
	pagetable[firstcome].changed=FALSE;
	pagetable[firstcome].count=0;
	pagetable[firstcome].filled=FALSE;

	do_page_in(ptable,pagetable[firstcome].block_num);


	ptable->block_num=pagetable[firstcome].block_num;
	ptable->changed=FALSE;
	ptable->count=0;
	ptable->filled=TRUE;

	FIFOSeq_change(ptable->page_num);

	printf("Page exchange Success!\n");
}

//page change:LRU
void do_LRU(PageTablePtr ptable)
{
	unsigned int i,min_use,page;
	printf("There is no idle block.Do LRU:\n ");
	min_use=0xFFFFFFFF;							//init the least frequency
	page=0;
	for(i=0;i<PAGE_TOTAL;i++)
	{
		if(min_use>pagetable[i].no_use)
		{
			min_use=pagetable[i].no_use;
			page=pagetable[i].page_num;
		}
	}
	printf("Replace the %uth page.\n",page);
	if(pagetable[page].changed)
	{
		printf("The page to be replaced has been changed:Write back.\n");
		do_page_out(&pagetable[page]);
	}
	pagetable[page].changed=FALSE;
	pagetable[page].count=0;
	pagetable[page].no_use=0;
	pagetable[page].filled=FALSE;

	do_page_in(ptable,pagetable[page].block_num);

	ptable->block_num=pagetable[page].block_num;
	ptable->changed=FALSE;

	ptable->count=0;
	ptable->no_use=0;
	ptable->filled=TRUE;
	printf("Page exchange Success!\n");

}

#ifndef VMM_GLOBAL_H_
#define VMM_GLOBAL_H_


//global data declaration

#define uchar unsigned char
#define uint unsigned int
#define ulong unsigned long

#define READABLE 0x01u			
#define WRITABLE 0x02u		
#define EXECUTABLE 0x04u

#define MAX_VALUE 0xFFu

#define AUXILIARY_SPACE_FILE "aux_space"//simulate the path of the files in the disk, not in the memory

#define ACTUAL_MEMORY_SIZE (32*4)						//actual memory size
#define VIRTUAL_MEMORY_SIZE	(64*4)					//virtual memory size

#define PAGE_SIZE 4								//page size
#define INNER_PAGE_TABLE_SIZE 4					//inner page table size
#define PAGE_TOTAL (VIRTUAL_MEMORY_SIZE/PAGE_SIZE)		//total virtual page number(inner)
#define OUTER_PAGE_TOTAL (PAGE_TOTAL/INNER_PAGE_TABLE_SIZE)		//total virtual page number(outer)
#define BLOCK_TOTAL (ACTUAL_MEMORY_SIZE/PAGE_SIZE)		//total physical blocks number

#define PID_NUM 4


#define FIFO "vmm_fifo"

//global enum type declaration

//error type enum
typedef enum
{
	READ_DENY,					
	WRITE_DENY,			
	EXECUTE_DENY,									
	INVALID_REQUEST,							
	OVER_BOUNDARY,							
	FILE_OPEN_FAILED,							
	FILE_CLOSE_FAILED,							
	FILE_SEEK_FAILED,							
	FILE_READ_FAILED,							
	FILE_WRITE_FAILED							
}ErrorType;

typedef enum
{
	TRUE=1,
	FALSE=0
}BOOL;


//type of  request of the memory access
typedef enum
{
	READ,
	WRITE,
	EXECUTE 
}RequestType;


//global structure declaration

//PCB(simplified:Process Control Block)
typedef struct
{
	unsigned int pid;								//process id
	unsigned int start;							//start outer page number
	unsigned int end;								//end outer page number
}PCB;

//the page table(outer)
typedef struct
{
	unsigned int page_num;							//outer page number
	unsigned int index_num;							//inner page number
}OuterPageTableItem,*OuterPageTablePtr;

//the page table(inner)
typedef struct
{
	unsigned int page_num;							//inner page number
	unsigned int block_num;							//physical block number
	BOOL filled;									//whether the page is filled
	BOOL changed;									//whether the page is changed
	unsigned char pro_type;							//protect type
	unsigned long virtual_Addr;					//virtual memory address
	unsigned long count;							//count the frequency of the page used
	unsigned int no_use;							//find the most recent page that is not used
}PageTableItem,*PageTablePtr;


//struct of request of the memory access
typedef struct
{
	RequestType request_type;						//type of  request of the memory access
	unsigned long virtual_Addr;					//virtual address
	unsigned char value;							//the value of write request
}MemoryAccessRequest,*MemoryAccessRequestPtr;

//global variable declaration

unsigned char actual_memory[ACTUAL_MEMORY_SIZE];		//actual memory space
FILE *auxmem_ptr;										//simulate the disk with the file
PageTableItem pagetable[PAGE_TOTAL];					//page table(inner)
OuterPageTableItem outerpagetable[OUTER_PAGE_TOTAL];	//page table(outer)
BOOL block_status[BLOCK_TOTAL];							//whether the physical block(actual memory) has been used
MemoryAccessRequestPtr ptr_memAccRequest;						//memory access request
unsigned int FIFOSeq[PAGE_TOTAL];						//for fifo, record the page sequence of the actual memory, each is page number and the first is the first do_page_in page. 
PCB pcb[PID_NUM];										//record the beginning and end page number of each process
unsigned int exec_times;								//execute times: for LRU algorithms
int lengthOfSeq;										//the length of fifo sequence FIFOSeq[]
int process_id;											//process id, for different process to request the memory access


//global function declaration

//welcome
void welcomeVMMRequest();
void welcomeVMMResponse();

//init the environment
void init();

//memory access
void do_request(MemoryAccessRequestPtr);				//produce the memory access request randomly
void do_response();								//response to the memory access request

//deal with the page
void do_page_in(PageTablePtr,unsigned int);			//move the contents in the disk into the actual memory
void do_page_out(PageTablePtr);						//move the contents in the actual memory out to the disk
void do_page_fault(PageTablePtr);					//deal with the page fault

//page exchange algorithm
void do_LFU(PageTablePtr);							//least frequency use algorithm
void do_FIFO(PageTablePtr);							//first in, first out
void do_LRU(PageTablePtr);							//least recently use

//print the page table information
void print_pageinfo();

//error handler
void handle_error(ErrorType);

//FIFO sequence array(for fifo) handler
void FIFOSeq_change(unsigned int);

//get the protect type string
char *get_protype_str(char *str,unsigned char type);



#endif /* VMM_GLOBAL_H_ */

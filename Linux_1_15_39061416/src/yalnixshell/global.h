#ifndef _global_H
#define _global_H

#define HISTORY_LEN 10

#define MAXPIPE 16

//not used;  arg -----20
#define MAXARG 15

#define FOREGROUND 'F'
#define BACKGROUND 'B'
#define SUSPENDED 'S'
#define WAITING_INPUT 'W'

#define BY_PROCESS_ID 1
#define BY_JOB_ID 2
#define BY_JOB_STATUS 3

#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <stdlib.h>

typedef struct Command
{
	char *args[MAXARG];					//command and parameters
	char *input;					//input redirect
	char *output;					//output redirect
}Command;

typedef struct History
{
	int start;						//srart position
	int end;						//end position
	char cmds [HISTORY_LEN][100];	//history command
}History;


typedef struct Job
{
	int id;
	//int pid[MAXPIPE+1];				
	int pid;						//pid number
	int groupid;					//groupid
	char cmd[100];					//command str
	char state[10];					//state
	int status;						//status
	struct Job *next;				//next point

}Job;

char inputBuff[100],inputBuffCopy[100];
Command cmd[MAXPIPE+1];
int commandDone,isBackground;
int cmd_num,pipe_num,jobNo;
static Job* jobHead = NULL; 	    // linked list of active processes
static int numActiveJobs = 0; 
int nowstatus;

void init();
void addHistory(char *history);
void execute();

#endif


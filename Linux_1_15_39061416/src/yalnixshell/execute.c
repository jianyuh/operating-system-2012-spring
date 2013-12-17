#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include "global.h"
#include "jobs.h"

char *envPath[10], cmdBuff[40];		//the environment path,the command buffer
History history;					//history record
//Job *curjob;							//current Job;

//shell pid, pgid, default terminal, terminal modes
pid_t SH_PID;
pid_t SH_PGID;
int SH_TERMINAL, SH_IS_INTERACTIVE;
struct termios SH_TMODES;

//for debug : present the information of the running process
void pr_ids(char *name)
{
	printf("%s: pid = %d, ppid = %d, pgrp = %d,tpgrp = %d\n",name,getpid(),getppid(),getpgrp(),tcgetpgrp(STDIN_FILENO));
	fflush(stdout);
}


//for error handler, print the error information
void errorPrint(char *s)
{
	printf("Error:%s\n",s);
}

void welcomeShell()
{
        printf("\n//////////////////////////////////////////////\n");
        printf("\tWelcome to shell version 1.0\n");
        printf("\tAuthor:39061416\tHuang Jianyu\n");
        printf("\tWebsite:www.huangjy.info\n");
        printf("\tMobile Phone:15210965935\n");
        printf("\tEmail:hjyahead@gmail.com\n");
        printf("///////////////////////////////////////////////\n");
        printf("\n\n");
}

//signal handler for SIGCHLD
void signalHandler_child(int p)
{
        pid_t pid,pgid;
        int terminationStatus;
        Job* job;
        
        //printf("come into the signalHandler_child part!\n");
        
        pid = waitpid(WAIT_ANY, &terminationStatus, WUNTRACED | WNOHANG); // intercept the process that sends the signal
        if (pid > 0) {  // if there are information about it
        		pgid = tcgetpgrp(STDIN_FILENO);
        		
        		if(pgid == SH_PGID) job = getJob(pid, BY_PROCESS_ID);
        		else job = getJob(pgid, BY_PROCESS_ID);// get the job from the list
				//MAKE SURE the pgid is right,the the above change to getJob(pgid...)
				//after the ^C(WIFSIGNALED(terminationStatus)):kill(-groupid);after the ^Z(),kill(-groupid);
                if (job == NULL)
                {
                		//printf("job is NULL!\n");
                        return;
                 }
                        
               // printf("job->id:%d,job->name:%s,pid:%d,groupid:%d,status:%d\n",job->id,job->cmd,job->pid,job->groupid,job->status);
                
                if (WIFEXITED(terminationStatus)) {  // case the process exits normally
                		//printf("the process exits normally\n");
                        if (job->status == BACKGROUND) {  // child in background terminates normally
                                printf("\n[%d]+  Done\t   %s\n", job->id, job->cmd); // inform the user
                                jobHead = delJob(job);   // delete it from the list
                        }
                } else if (WIFSIGNALED(terminationStatus)) { // the job dies because of a signal
                		 //printf("the job dies because of a signal\n");	
                        printf("\n[%d]+  KILLED\t   %s\n", job->id, job->cmd); // inform the user
                        jobHead = delJob(job);// delete the job from the list
                } else if (WIFSTOPPED(terminationStatus)) {  // a job receives a SIGSTP signal
                        
                       // printf("a job receives a SIGSTP signal\n");
                        
                       
                        
                        if (job->status == BACKGROUND) { // the job is in bg
                                tcsetpgrp(SH_TERMINAL, SH_PGID);
                                changeJobStatus(pid, WAITING_INPUT); // change its status to "waiting for input"
                                printf("\n[%d]+   suspended [wants input]\t   %s\n",
                                       numActiveJobs, job->cmd); // inform the user
                        } else {  // otherwise, the job is going to be suspended
                               // tcsetpgrp(SH_TERMINAL, job->groupid);
                                
                                tcsetpgrp(SH_TERMINAL, SH_PGID);				//?????????????????
                                
                                //kill(- job->groupid,SIGTSTP);
                                
                                changeJobStatus(pid, SUSPENDED);// we modify the status
                                
                                
                                nowstatus == SUSPENDED;
                                
                                printf("\n[%d]+   stopped\t   %s\n", numActiveJobs, job->cmd); // and inform the user
                                
                                //goto waitend;
                        }
                        return;
                } else {
                		//printf("a job dies because of other reason\n");
                        if (job->status == BACKGROUND) {  // otherwise, delete the job from the list
                                jobHead = delJob(job);
                        }
                }
                tcsetpgrp(SH_TERMINAL, SH_PGID);
        }
}


//waits for a job, blocking unless it has been suspended.
//Deletes the job after it has been executed
void waitJob(Job* job)
{
        int terminationStatus;
        while (waitpid(job->pid, &terminationStatus, WNOHANG) == 0) {      // while there are child to be waited
                if (job->status == SUSPENDED)                              // exit if the job has been set to be stopped
                        return;
        }
		//usleep(20000);
        jobHead = delJob(job);                                            // delete the job
}

//puts a job in foreground. If continueJob = TRUE, sends the process group
//a SIGCONT signal to wake it up. After the job is waited successfully, it
//restores the control of the terminal to the shell
void putJobForeground(Job* job, int continueJob)
{
        job->status = FOREGROUND;                  // set its status in the list as FOREGROUND
        tcsetpgrp(SH_TERMINAL, job->groupid);      // give it the control of the terminal
        if (continueJob) {                        // continue the job (if desired)
                if (kill(-job->groupid, SIGCONT) < 0) // by sending it a SIGCONT signal
                        perror("kill (SIGCONT)");
        }

        waitJob(job);                            // wait for the job
        tcsetpgrp(SH_TERMINAL, SH_PGID);        // give the shell control of the terminal
}

//puts a job in background, and sends the job a continue signal, if continueJob = TRUE
//puts the shell in foreground
void putJobBackground(Job* job, int continueJob)
{
        if (job == NULL)
                return;

        if (continueJob && job->status != WAITING_INPUT)
                job->status = WAITING_INPUT;	// fixes another synchronization bug: if the child process launches
		// a SIGCHLD and is set to WAITING_INPUT before this point has been
        // reached, then it would be set to BACKGROUND again

        if (continueJob)                        // if desired, continue the job
                if (kill(-job->groupid, SIGCONT) < 0)
                        perror("kill (SIGCONT)");

        tcsetpgrp(SH_TERMINAL, SH_PGID);       // paranoia: give the shell control of terminal
}


//Judge whether the command exists
int exists(char *cmdFile)
{
	//^^^^^^^
    //printf("Come to the function exists\n");
    
	int i=0;
	if((strcmp(cmd->args[0],"exit")==0)||(strcmp(cmd->args[0],"history")==0)||(strcmp(cmd->args[0],"jobs")==0)||(strcmp(cmd->args[0],"cd")==0)||(strcmp(cmd->args[0],"fg")==0)||(strcmp(cmd->args[0],"bg") == 0))
		return 1;
	if((cmdFile[0]=='/'||cmdFile[0]=='.')&&access(cmdFile,F_OK)==0)//have access to the command file in the present path
	{
		strcpy(cmdBuff, cmdFile);
		return 1;
	}
	else//have access to the command file in the envPath
	{
		while(envPath[i] != NULL)
		{
			strcpy(cmdBuff,envPath[i]);
			strcat(cmdBuff,cmdFile);
			if(access(cmdBuff,F_OK) == 0)
			{
				return 1;
			}
			i++;
		}
	}
	//^^^^^^^
    //printf("Come Out of the function exists\n");
	return 0;
}

//Transform the String to Number
int str2Pid(char *str,int start,int end)
{
	int i,j;
	char chs[20];

	for(i=start,j=0;i<end;i++,j++)
	{
		if(str[i]<'0'||str[j]>'9')
		{
			printf("number < 0 or number > 9\n");
			return -1;
		}
		else
			chs[j] = str[i];
	}
	chs[j] = '\0';

	return atoi(chs);
}

//Change the form of some outer commands
void justArgs(char *str)
{
	int i,j,len;
	len = strlen(str);

	for (i=0,j=-1;i<len;i++)
	{
		if(str[i] == '/')
			j=i;
	}

	if(j!=i)//find the last '/' of the command
	{
		for (i = 0 ,j++;j<len;i++,j++)
		{
			str[i]=str[j];
		}
		str[i]='\0';
	}
}

//fg command
void fg_exec(int No)
{
	Job* job = getJob(No, BY_JOB_ID);
	if (job == NULL)
		return;
	if (job->status == SUSPENDED || job->status == WAITING_INPUT)
		putJobForeground(job, TRUE);
	else                                                                                                // status = BACKGROUND
		putJobForeground(job, FALSE);
}

//bg command
void bg_exec(int No)
{
	Job* job = getJob(No, BY_JOB_ID);
	if (job == NULL)
		return;
	if(job->status == FOREGROUND)
		putJobBackground(job,FALSE);
	else
		putJobBackground(job,TRUE);
}

//add HistHISory
void addHistory(char *cmd)
{
	if(history.end == -1)//第一次使用history
	{
		//printf("第一次使用history循环数组\n");
		history.end = 0;
		strcpy(history.cmds[history.end],cmd);
		return;
	}
	history.end = (history.end + 1)%HISTORY_LEN;//end在循环数组环形区域中向前1位
	strcpy(history.cmds[history.end],cmd);
	if(history.end==history.start)
	{
		history.start = (history.start + 1)%HISTORY_LEN;
	}
}

//get the environment path
void getEnvPath(int len,char *buf)
{
	int i,j,last = 0, pathIndex = 0,temp;
	char path[40];
	
	for(i=0,j=0;i<len;i++)
	{
		if(buf[i]==':')
		{
			if(path[j-1]!='/')
				path[j++]='/';
			path[j] = '\0';
			j=0;
			
			temp = strlen(path);
			envPath[pathIndex] = (char*)malloc(sizeof(char)*(temp+1));
			strcpy(envPath[pathIndex],path);
			
			pathIndex++;
		}
		else
		{
			path[j++]=buf[i];
		}
	}
	envPath[pathIndex] = NULL;
	//^^^^^^^
	//for (i = 0;i<pathIndex;i++)
    //	printf("envPath[%d]:%s\n",i,envPath[i]);
}

//initial
void init()
{
	int fd,n,len;
	char c,buf[80];
	
	//open the file "cmdenv.conf"
	if( (fd = open("cmdenv.conf",O_RDONLY,660))==-1)
	{
		perror("init environment failed\n");
		exit(1);
	}
	
	//init the history
	history.end = -1;
	history.start = 0;
	jobNo = 0;
	//too important!!!!!!!!!!!!!!
	len=0;
	while(read(fd,&c,1)!=0)
	{
		buf[len++]=c;
	}
	
	buf[len]='\0';

	//get the environment path
	getEnvPath(len,buf);

	SH_PID = getpid();                                                             // retrieve the pid of the shell
    SH_TERMINAL = STDIN_FILENO;                                       // terminal = STDIN
    SH_IS_INTERACTIVE = isatty(SH_TERMINAL);            // the shell is interactive if STDIN is the terminal

    if (SH_IS_INTERACTIVE)
	{                                                 // is the shell interactive?
		while (tcgetpgrp(SH_TERMINAL) != (SH_PGID = getpgrp()))
			kill(SH_PID, SIGTTIN);                                                    // make sure we are in the foreground
               //ignore all the job control stop signals and install custom signal handlers 
                signal(SIGQUIT, SIG_IGN);
                signal(SIGTTOU, SIG_IGN);
                signal(SIGTTIN, SIG_IGN);
                signal(SIGTSTP, SIG_IGN);
                signal(SIGINT, SIG_IGN);
                signal(SIGCHLD, &signalHandler_child);

                setpgid(SH_PID, SH_PID);                                         // we make the shell process as new process group leader
                SH_PGID = getpgrp();
                if (SH_PID != SH_PGID) {
                        printf("Error, the shell is not process group leader");
                        exit(EXIT_FAILURE);
                }
                if (tcsetpgrp(SH_TERMINAL, SH_PGID) == -1)      // if bdsh cannot grab control of the terminal
                    tcgetattr(SH_TERMINAL, &SH_TMODES);             // we save default terminal attributes for shell.
                //currentDirectory = (char*) calloc(1024, sizeof(char));
                
     }
	else 
	{
                printf("Could not make shell interactive. Exiting..\n");
                exit(EXIT_FAILURE);
	}
        
}


// Command parser
int handleCommandStr(char *buf,int cmdnum)
{
	int begin,end;
	int i,j,k;
	int fileFinished;	//record whether the command has been parsed
	char c,buff[20][40],inputFile[30],outputFile[30],*temp = NULL;
	//Command *cmd = (Command*)malloc(sizeof(Command));
	begin = 0;end = strlen(buf);
	
	//^^^^^^^
    //printf("Come To the handleCommandStrPart\n");
	
	//default: not background
	cmd[cmdnum].input = cmd[cmdnum].output = NULL;
	
	//init the related variables
	for(i=begin;i<20;i++)
	{
		buff[i][0]='\0';
	}
	inputFile[0] = '\0';
	outputFile[0] = '\0';
	
	i=begin;
	//skip the blank
	while(i<end && (buf[i] == ' '||buf[i] == '\t'))
	{
		i++;
	}
	
	k=0;//buff ID refer to string
	j=0;//temp ID, refer to char
	fileFinished = 0;
	
	temp = buff[k];//temp refer to buffer[i]
	while(i<end)
	{
		switch(buf[i])
		{
			case ' ':
			case '\t':
				temp[j]='\0';
				j = 0;
				if(!fileFinished)
				{
					k++;
					temp = buff[k];
				}
				break;
			case '<':////redirect of the input
				if(j!=0)
				{
					temp[j] ='\0';
					j = 0;
					if(!fileFinished)
					{
						k++;
						temp = buff[k];
					}
				}
				temp = inputFile;
				fileFinished ++;
				i++;
				break;
				
			case '>'://redirect of the output
				if(j!=0)
				{
					temp[j]='\0';
					j=0;
					if(!fileFinished)
					{
						k++;
						temp = buff[k];
					}
				}
				temp = outputFile;
				fileFinished ++;
				i++;
				break;
				
			case '&':
				if(j!=0)
				{
					temp[j] ='\0';
					j = 0;
					if(!fileFinished)
					{
						k++;
						temp = buff[k];
					}
				}
				isBackground = 1;
				fileFinished ++;
				i++;
				break;
			default:
				temp[j++]=buf[i++];
				continue;
		}
		while(i<end &&(buf[i]==' '||buf[i]=='\t'))
		{
			i++;
		}
	}
	

	if(buf[end-1]!=' '&&buf[end-1]!='\t'&&buf[end-1]!='&')
	{
		temp[j] = '\0';
		if(!fileFinished)
		{
			k++;
		}
	}

	cmd[cmdnum].args[k] = NULL;
	for(i = 0;i<k;i++)
	{
		j = strlen (buff[i]);
		cmd[cmdnum].args[i] = (char*)malloc(sizeof(char)*(j+1));
		strcpy(cmd[cmdnum].args[i],buff[i]);
		
		//^^^^^^^
    	//printf("cmd[%d].args[%d]:%s\n",cmdnum,i,cmd[cmdnum].args[i]);
	}
	
	if(strlen(inputFile)!=0)
	{
		j=strlen(inputFile);		
		cmd[cmdnum].input = (char*)malloc(sizeof(char)*(j+1));
		strcpy(cmd[cmdnum].input,inputFile);
		
		//^^^^^^^
    	printf("inputFile:%s\n",inputFile);
	}
	
	if(strlen(outputFile)!=0)
	{
		j=strlen(outputFile);
		cmd[cmdnum].output = (char*)malloc (sizeof(char)*(j+1));
		strcpy(cmd[cmdnum].output,outputFile);
		//^^^^^^^
    	printf("outputFile:%s\n",outputFile);
	}
	//^^^^^^^
    //printf("Come Out of the handleCommandStr\n");
	return 0;
}


//exit Command
void exitCmd(Command *cmd)
{
	if(cmd->args[1]!=NULL)
	{
		errorPrint("wrong exit command!\n");
		return;
	}
	//release();
	printf("\nByeBye!\n\n");
	
	exit(1);
}
	
//history Command
void historyCmd(Command *cmd)
{
	int i;
	if(cmd->args[1]!=NULL)
	{
		errorPrint("wrong history command!\n");
		return;
	}
	
	if(history.end == -1)
		{
			printf("no command ever\n");
			return;
		}
		i=history.start;
		do
		{
			printf("%s\n",history.cmds[i]);
			i = (i+1)%HISTORY_LEN;
		}while(i != (history.end+1)%HISTORY_LEN);
		
	//scanf printf redirect?????????????????????????????????????????????????
}


//jobs Command
void jobsCmd(Command *cmd)
{
	int i;
	Job *now;
	if(cmd->args[1]!=NULL)
	{
		errorPrint("wrong jobs command!\n");
		return;
	}
	
	if(jobHead == NULL)
	{
		printf("No jobs\n");
	}
	else
	{
	
		printJobs();
		//printf("index\tpid\tstate\t\tcommand\n");
		//for (i=1,now = jobHead;now !=NULL; now = now->next,i++)
		//{
		//	printf("%d\t%d\t%s\t\t%s\n",i,now->pid,now->state,now->cmd);
		//}
	}
}

//cd Command
void cdCmd(Command *cmd)
{
	char *temp;
	if(cmd->args[2]!=NULL)
	{
		errorPrint("wrong cd command!\n");
		return;
	}
	
	temp = cmd->args[1];
	if(temp!=NULL)			//temp==NULL????????????????????????????????????????????????
	{
		if(chdir(temp)<0)
		{
			printf("cd;%s wrong file name\n",temp);
		}
	}
	else
	{
		if(chdir(getenv("HOME"))<0)
		{
			printf("cd;HOME wrong file name\n");
		}
		
	}
}


//fg Command
void fgCmd(Command *cmd)
{
	char *temp;
	int No;
	if(cmd->args[2]!=NULL)
	{
		errorPrint("wrong fg command!\n");
		return;
	}
	
	temp = cmd->args[1];
	if(temp !=NULL &&temp[0] == '%')
	{
		No = str2Pid(temp,1,strlen(temp));
		if(No != -1)
		{
			fg_exec(No);
		}
	}
	else
	{
		printf("fg;wrong parameter.standard format:fg %%<int>\n");
	}	

}


// bg Command
void bgCmd(Command *cmd)
{
	char *temp;
	int No;
	if(cmd->args[2]!=NULL)
	{
		errorPrint("wrong bg command!\n");
		return;
	}
	temp = cmd->args[1];
	if(temp !=NULL&&temp[0] =='%')
	{
		No = str2Pid(temp,1,strlen(temp));
		if(No != -1)
		{
			bg_exec(No);
		}
	}
	else
	{
		printf("bg;wrong parameter.standard format:bg %%<int>\n");
	}
}


//execute outer command
void execOuterCmd(Command *cmd,int executionMode)
{
	pid_t pid;
	int pipeIn,pipeOut;
	
	//sigset_t mask;
	
	//^^^^^^^
    //printf("Come To execOutercmd\n");
    
	
	if(exists(cmd->args[0]))	//command exists
	{ 	
		if((pid = fork())<0)
		{
			perror("fork failed");
		}	
		if(pid ==0)			//child process
		{
			//^^^^^^^
    		//printf("I'm the child Process\n");
    		
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			signal(SIGCHLD, &signalHandler_child);
			signal(SIGTTIN, SIG_DFL);
			usleep(20000);             // fixes a synchronization bug. Needed for short commands like ls
			setpgrp();           

			if (executionMode == FOREGROUND)
				tcsetpgrp(SH_TERMINAL, getpid());  // if we want the process to be in foreground
			if (executionMode == BACKGROUND)
				printf("\n[%d] %d\n", ++numActiveJobs, (int) getpid()); // inform the user about the new job in bg

			if(cmd->input !=NULL)
			{
				if((pipeIn = open(cmd->input,O_RDONLY,S_IRUSR|S_IWUSR))==-1)
				{
					printf("can not open %s!\n",cmd->input);
					return;
				}
				if(dup2(pipeIn,0)==-1)
				{
					printf("input redirect error!\n");
					return;
				}
			}

			if(cmd->output !=NULL)
			{
				if((pipeOut = open(cmd->output,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR))==-1)
				{
					printf("can not open %s!\n",cmd->output);
					return;
				}
				if(dup2(pipeOut,1)==-1)
				{
					printf("output redirect error\n");
					return;
				}
			}
			
			
			if(innerCmd(cmd)==0)
			{
				justArgs(cmd->args[0]);


				if(execv(cmdBuff,cmd->args)<0)
				{
					perror("Shell(execv)");
					return;
				}
			}	

			
			exit(EXIT_SUCCESS);
		}
		else//parent process
		{
			//^^^^^^^
    		//printf("I'm the parent Process\n");
    		
			setpgid(pid, pid);    // make the child a new process group leader from here
			// to avoid race conditions
			jobHead = insertJob(pid, pid,inputBuffCopy,(int)executionMode); // insert the job in the list

			Job* job = getJob(pid, BY_PROCESS_ID);  // and get it as job object

			if (executionMode == FOREGROUND) 
			{
				putJobForeground(job, FALSE);  // put the job in foreground (if desired)
			}
			if (executionMode == BACKGROUND)
			{
				putJobBackground(job, FALSE);  // put the job in background (if desired)
				
			}
		}
	}
	else//command not exists
	{
		printf("The command does not exist!%s\n",inputBuff);
	}
	
	
	//^^^^^^^
    //printf("Come Out of execOutercmd\n");
}

int innerCmd(Command *cmd)
{
	/*
	int pipeIn,pipeOut;
	if(cmd->input !=NULL)
	{
		if((pipeIn = open(cmd->input,O_RDONLY,S_IRUSR|S_IWUSR))==-1)
		{
			printf("can not open %s!\n",cmd->input);
			return;
		}
		if(dup2(pipeIn,0)==-1)
		{
			printf("input redirect error!\n");
			return;
		}
	}
	if(cmd->output !=NULL)
	{
		if((pipeOut = open(cmd->output,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR))==-1)
		{
			printf("can not open %s!\n",cmd->output);
			return;
		}
		if(dup2(pipeOut,1)==-1)
		{
			printf("output redirect error\n");
			return;
		}
	}
	*/
	
	if(strcmp(cmd->args[0],"exit")==0)//exit Command
	{
		exitCmd(cmd);
		return 1;
	}
	else if(strcmp(cmd->args[0],"history")==0)//history Command
	{
		historyCmd(cmd);
		//dup2(1,1);
		//dup2(0,0);
		//close(pipeIn);
		//close(pipeOut);

		return 1;
	}
	else if(strcmp(cmd->args[0],"jobs")==0)//jobs Command
	{
		jobsCmd(cmd);
		return 1;
	}
	else if(strcmp(cmd->args[0],"cd")==0)//cd Command
	{
		cdCmd(cmd);
		return 1;
	}
	else if(strcmp(cmd->args[0],"fg")==0)//bg Command
	{
		fgCmd(cmd);
		return 1;
	}
	else if(strcmp(cmd->args[0],"bg") == 0)//bg Command
	{
		bgCmd(cmd);
		return 1;
	}
	else
		return 0;

}

//execute the simple command
void execCommand(Command *cmd)
{
	int i;
	//^^^^^^^
    //printf("Come To execCommand\n");
	
	if(strcmp(cmd->args[0],"history")==0||innerCmd(cmd)==0)			//history command will be done in the execOuterCmd, in case of redirect
	{
		if(isBackground)
		{
			execOuterCmd(cmd,BACKGROUND);
		}
		else
		{
			execOuterCmd(cmd,FOREGROUND);
		}
	}
	
	//^^^^^^^
    //printf("Come Out of execCommand\n");
	
	//free the command array
	for(i=0;cmd->args[i]!=NULL;i++)
	{
		free(cmd->args[i]);
	}
		free(cmd->input);
		free(cmd->output);
	
}


//execute the pipe command
void execPipeCmd(int executionMode)
{
	int i,j,pipeIn,pipeOut,groupid;//ppid;
	pid_t pid;
	int pfd[MAXPIPE][2];
	Job* job;
	int allpid[MAXPIPE+1];
	int pidcnt = 0;
	int suspendflag =0;
	int terminationStatus;
	
	for (i = 0;i<pipe_num;i++)
	{
		if(  pipe(pfd[i])==-1 )
		{
			perror("pipe failed!\n");
			exit(errno);	
		}
	}

    for(i = 0;i<cmd_num;i++)
    {
    	if( (pid=fork())==0 )
    	{
    	
			signal(SIGINT, SIG_DFL);
			signal(SIGQUIT, SIG_DFL);
			signal(SIGTSTP, SIG_DFL);
			signal(SIGCHLD, &signalHandler_child);
			signal(SIGTTIN, SIG_DFL);
			usleep(160000);
			


			if (i ==0)
			{
				groupid = getpid();
				//tcsetpgrp(STDIN_FILENO, groupid);
			}
			setpgid(getpid(),groupid);
			
			//pr_ids(cmd[i].args[0]);
		
    		if(pipe_num)
    		{
    			if(i==0)
    			{
    				dup2(pfd[0][1],STDOUT_FILENO);
    				close(pfd[0][0]);
    				for(j=1;j<pipe_num;j++)
    				{
    					close(pfd[j][0]);
    					close(pfd[j][1]);
    				}
    			}
    			else if(i==pipe_num)
    			{
    				dup2(pfd[i-1][0],STDIN_FILENO);
    				close(pfd[i-1][1]);
    				for(j=0;j<pipe_num-1;j++)
    				{
    					close(pfd[j][0]);
    					close(pfd[j][1]);
    				}
    			}
    			else
    			{
    				dup2(pfd[i-1][0],STDIN_FILENO);
    				close(pfd[i-1][1]);
    				dup2(pfd[i][1],STDOUT_FILENO);
    				close(pfd[i][0]);
    				for(j=0;j<pipe_num;j++)
    				{
    					if(j!=i&&j!=i-1)
    					{
    						close(pfd[j][0]);
    						close(pfd[j][1]);
    					}
    				}
    			}
    		}

			
			if (executionMode == FOREGROUND)
				tcsetpgrp(SH_TERMINAL, getpid());      // if we want the process to be in foreground
			if (executionMode == BACKGROUND)
				printf("[%d] %d\n", ++numActiveJobs, (int) getpid());
			

   		 	if(cmd[i].input !=NULL)
			{
				//^^^^^^^
    			printf("in=%s\n",cmd[i].input);
				if((pipeIn = open(cmd[i].input,O_RDONLY,S_IRUSR|S_IWUSR))==-1)
				{
					printf("can not open %s!\n",cmd[i].input);
					return;
				}
				if(dup2(pipeIn,0)==-1)
				{
					printf("input redirect error!\n");
					return;
					}
			}
	
			if(cmd[i].output !=NULL)
			{
				//^^^^^^^
    			printf("out=%s\n",cmd[i].output);
				if((pipeOut = open(cmd[i].output,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR))==-1)
				{
					printf("can not open %s!\n",cmd[i].output);
					return;
				}
				if(dup2(pipeOut,1)==-1)
				{
					printf("output redirect error!\n");
					return;
				}
			}			
			
			
			/*
			if(innerCmd( &cmd[i])==0)
			{
				justArgs(cmd->args[0]);
				if(execv(cmdBuff,cmd->args)<0)
				{
					perror("Shell(execv)");
					return;
				}
			}
			

			exit(EXIT_SUCCESS);
			*/

			//runCmd(cmd[i]);
		
			//execv(cmdBuff,cmd->args)
		
			//^^^^^^^		
			//printf("execvp started!!!!%s\n",cmd[i].args[0]);

			if(innerCmd( &cmd[i])==0)
			{
				execvp(cmd[i].args[0],cmd[i].args);
				fprintf(stderr,"executing%serror.\n",cmd[i].args[0]);
			}
			exit(0);   					
			break;
    	}
    	
		
		if(i ==0)
		{
			groupid = pid;
		}
		setpgid(pid, groupid);			// to avoid race conditions
		
		if(i == 0)
		{
			groupid =pid;
			//setpgid(pid, pgid);			// to avoid race conditions
			jobHead = insertJob(pid, groupid,inputBuffCopy,(int)executionMode); // insert the job in the list

			job = getJob(pid, BY_PROCESS_ID);   // and get it as job object

		}
		
		allpid[pidcnt++] = pid;

    }	
	//parent
	for (i=0;i<pipe_num;i++)
	{
		close(pfd[i][0]);
		close(pfd[i][1]);
	}
	
	
	/*
	if (executionMode == FOREGROUND) {
		putJobForeground(job, FALSE);   // put the job in foreground (if desired)
	}
	if (executionMode == BACKGROUND)
		putJobBackground(job, FALSE);   // put the job in background (if desired)
	
	
	
	
	//for(i=0;i<cmd_num;i++)
	//	wait(NULL);
	
	//tcsetpgrp(0,getpid());
	*/
	//printf("YES1!\n");
	
	if (executionMode == FOREGROUND) {
		putJobForeground(job, FALSE);   // put the job in foreground (if desired)
	}
	if (executionMode == BACKGROUND)
		putJobBackground(job, FALSE);   // put the job in background (if desired)
	
	//printf("YES2!\n");
	
	
	for(i=1;i<cmd_num;i++)
	{
		//wait(NULL);	
		while (waitpid(allpid[i], &terminationStatus, WNOHANG) == 0)
		{      // while there are child to be waited
			if (nowstatus==SUSPENDED||(job!=NULL&&job->status == SUSPENDED))  // exit if the job has been set to be stopped
            {
            	suspendflag = 1;
                break;
            }
        }
        if(suspendflag == 1) break;
		
	}
	
	//printf("YES3!\n");
	
	
	
	//waitpid(pid,NULL,0);

	//^^^^^^^
    //printf("Come Out of execPipeCmd\n");
    
    //free the command array
    for (j=0;j<cmd_num;j++)
    {
		for(i=0;cmd[j].args[i]!=NULL;i++)
		{
			free(cmd[j].args[i]);
		}
			free(cmd[j].input);
			free(cmd[j].output);
	}
    
}

//the interface for the command
void execute()
{
	//^^^^^^^
    //printf("Come to the Execute Part\n");
    
	int i,j;
	char *nextcmd,*curcmd;
	
	cmd_num = 0;
	
	nextcmd = inputBuff;
	
	strcpy(inputBuffCopy,inputBuff);
	
	while( (curcmd = strsep(&nextcmd,"|")))
	{
		//printf("%c\t\n",curcmd[0]);
		if(handleCommandStr(curcmd,cmd_num++) <0 )
		{
			//^^^^^^^	
			printf("Parse < 0 \n");
			cmd_num--;
			break;
		}
		
		//^^^^^^^	
		//printf("cmd_num in the while:%d\n",cmd_num);
		
		if (cmd_num == MAXPIPE + 1)
		{
			errorPrint("Process in jobs exceeds the max number!\n");
			break;
		}
		
	}
	
	pipe_num = cmd_num - 1;
	
	//^^^^^^^	
	//printf("cmd_num:%d\n",cmd_num);
	
	if (cmd_num == 0)
	{
		commandDone = 0;
		return;
	}
	else if(cmd_num == 1)
	{
		execCommand( &cmd[0]);
	}
	else
	{
		if(isBackground)
		{
			execPipeCmd(BACKGROUND);
		}
		else
		{
			execPipeCmd(FOREGROUND);
		}
	}
	
	commandDone = 1;
	
	//pr_ids("execute");
}













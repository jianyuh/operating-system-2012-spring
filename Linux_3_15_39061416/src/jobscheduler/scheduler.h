#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include<signal.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<time.h>
#include<string.h>
#include<pwd.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<malloc.h>
#include<errno.h>

#define TMPDIR		"/tmp"
#define MYFIFO		"/tmp/myfifo"

#define BUFLEN 100
#define DATALEN	sizeof(struct jobnode)
#define MAXARG 10
#define MAXPATH  25

enum jobstate
{
	READY,RUNNING,DONE
};
enum cmdtype
{
	ENQ=-1,DEQ=-2,STAT=-3
};
struct jobnode
{
	int jid;
	int pid;
	int uid;
	int defpri;
	int curpri;
	int runtime;
	int waittime;
	time_t create_time;
	enum jobstate state;
	enum cmdtype type;
	char data[BUFLEN];
};


struct queue
{
	struct jobnode job;
	struct queue *next;
};


#endif /* SCHEDULER_H_ */

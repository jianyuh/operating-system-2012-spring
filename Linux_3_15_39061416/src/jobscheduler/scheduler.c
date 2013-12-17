#include "scheduler.h"

int fifo;
char *envPath[10];
struct sigaction newact, CHLD_act, VTALRM_act;
struct itimerval newtime,oldtime;
struct timeval interval;
struct queue *head,*current;
int jobid;
void welcome();
void getEnvPath(int len,char *buf);
void init();
void start();
void exit_scheduler();
void sig_handler(int ,siginfo_t *,void *);
void childsig();
void scheduler();
void update();
void newjob();
char * itoa(int);
void do_enq(struct jobnode);
void do_deq(struct jobnode);
void do_stat(struct jobnode);
void selectjob();
void executejob();


int main()
{
	welcome();
	init();
	start();
	while(1);
	exit_scheduler();
	return 1;
}

void welcome()
{
	printf("\n//////////////////////////////////////////////\n");
	printf("\tWelcome to job scheduler version 1.0\n");
	printf("\tAuthor:39061416\tHuang Jianyu\n");
	printf("\tWebsite:www.huangjy.info\n");
	printf("\tMobile Phone:15210965935\n");
	printf("\tEmail:hjyahead@gmail.com\n");
	printf("///////////////////////////////////////////////\n");
	printf("\n\n");
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

/*
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
*/



void init()
{
	int fd,len;
	char c,buf[80];

	//open the file "cmdenv.conf"
	if( (fd = open("cmdenv.conf",O_RDONLY,660))==-1)
	{
		perror("init environment failed\n");
		exit(1);
	}
	//too important!!!!!!!!!!!!!!
	len=0;
	while(read(fd,&c,1)!=0)
	{
		buf[len++]=c;
	}

	buf[len]='\0';

	//get the environment path
	getEnvPath(len,buf);




	//create fifo
	if((mkdir(TMPDIR,0666)!=0)&&errno!=EEXIST){
		printf("cannot create directory!\n");
		exit(0);
	}
	if((mkfifo(MYFIFO,0666)<0)&&errno!=EEXIST){
		printf("cannot create fifo!\n");
		exit(0);
	}
	if((fifo=open(MYFIFO,O_RDONLY|O_NONBLOCK))<0){
		printf("cannot open fifo!\n");
		exit(0);
	}
	
	//set signal
	newact.sa_flags=SA_SIGINFO;
	sigemptyset(&newact.sa_mask);
	newact.sa_sigaction=sig_handler;

	sigaction(SIGCHLD,&newact,&CHLD_act);
	sigaction(SIGVTALRM,&newact,&VTALRM_act);
	
	//init job queue
	head=NULL;
	current=NULL;
	
	//other
	jobid=0;
}
void start()
{

    //set time
	interval.tv_sec=0;
	interval.tv_usec=10;
	newtime.it_interval=interval;
	newtime.it_value=interval;
	setitimer(ITIMER_VIRTUAL,&newtime,&oldtime);
}
void exit_scheduler(){
	close(fifo);
}
void sig_handler(int sig,siginfo_t *info,void *notused){
	switch(sig){
	case SIGVTALRM:scheduler();return ;
	case SIGCHLD:childsig();return ;
	default: return ;
	}
}
void childsig()
{
	int ret;
	int status;
	ret=waitpid(-1,&status,WNOHANG);
	if(ret==-1){
		printf("waitpid error!\n");
		return ;
	}
	if(ret==0)
		return ;
	if(WIFEXITED(status)){
		current->job.state=DONE;
		printf("normal termination\n");
	}
	else if(WIFSIGNALED(status)){
		printf("abnormal termination\n");
	}
	else if(WIFSTOPPED(status)){
		printf("child stopped");
	}
	return ;
}
void scheduler()
{
	update();
	newjob();
	selectjob();
	executejob();
}

void update()
{
	struct queue *p;
	p=head;
	while(p!=NULL){
		if(p->job.state==READY){
			p->job.waittime+=10;
			if(p->job.waittime%100==0||p->job.curpri<3){
				p->job.curpri++;
			}
		}
		else if(p->job.state==RUNNING){
			p->job.runtime+=10;
			p->job.curpri=current->job.defpri;
		}
		p=p->next;
	}
}
void newjob()
{
	struct jobnode temp;
	while(1){
		if(read(fifo,&temp,DATALEN)<=0)
			break;
		switch(temp.type){
		case ENQ: do_enq(temp);break;
		case DEQ: do_deq(temp);break;
		case STAT: do_stat(temp);break;
		default: break;
		}
	}
}
char * itoa(int num)
{
	char * t=(char*)malloc(10);
	char s[10];
	int i=0,j=0;
	if(num==0){
		t[0]='0';
		t[1]=0;
		return t;
	}
	while(num!=0){
		s[i++]=num%10+'0';
		num/=10;
	}
	i--;
	while(i!=-1){
		t[j++]=s[i--];
	}
	t[j]=0;
	return t;
}
void do_enq(struct jobnode temp)
{
	struct queue *newjob,*jobp;
	char *cmdarg[MAXARG];
	int i,j,pid;
	int jobfifo;
	char localpath[MAXPATH];
	char *p;
	char cmdtmp[100];
	
	//get new job info
	newjob=(struct queue *)malloc(sizeof(struct queue));
	newjob->next=NULL;
	newjob->job.create_time=time(NULL);
	newjob->job.defpri=temp.defpri;
	newjob->job.curpri=temp.defpri;
	newjob->job.jid=jobid++;
	newjob->job.runtime=0;
	newjob->job.waittime=0;
	newjob->job.state=READY;
	newjob->job.type=ENQ;
	newjob->job.uid=temp.uid;
	strcpy(newjob->job.data,temp.data);

	//get command and arguments
	p=newjob->job.data;
	i=0;j=0;
	while(*p!=0){
		if(*p==':'){
			cmdtmp[i]=0;
			cmdarg[j]=(char *)malloc(strlen(cmdtmp)+1);
			strcpy(cmdarg[j],cmdtmp);
			i=0;j++;
		}
		else{
			cmdtmp[i++]=*p;
		}
		p++;
	}
	cmdarg[j]=NULL;
	
	
	//if(!exists(cmdarg[0]))
	//{
	//	printf("The command \"%s\"do not exist!",cmdarg[0]);
	//	//return;
	//}
	
	

    //add new jobnode
	jobp=head;
	if(head==NULL){
		head=newjob;
	}
	else{
		while(jobp->next!=NULL){
			jobp=jobp->next;
		}
		jobp->next=newjob;
	}

    //open fifo to output the result
	localpath[0]=0;
	strcat(localpath,TMPDIR);
	strcat(localpath,"/");
	strcat(localpath,itoa(temp.uid));
	
	
	//printf("%s\n",localpath);

	//execute new job

	if((pid=fork())<0){
		printf("fork error!\n");
	}
	else if(pid==0){
		raise(SIGSTOP);
		if((jobfifo=open(localpath,O_WRONLY))<0){
			printf("open localpath error!:%s\n",localpath);
			exit(0);
		}
		if(dup2(jobfifo,1)<0)
			printf("dup2 error!\n");
		if(execvp(cmdarg[0],cmdarg)<0){
			printf("exec failed!\n");
			exit(0);
		}
	}
	else{
		newjob->job.pid=pid;
		for(i=0;i<j;i++){
			free(cmdarg[i]);
		}
	}
}
void do_deq(struct jobnode temp){
	int jid,uid;
	char localpath[MAXPATH];
	struct queue *p,*pre;
	char info1[13]="no such job!\n";
	char info2[16]="not authorized!\n";
	char info3[16]="job terminated!\n";
	int jobfifo;

   //get deq info
	jid=temp.jid;
	uid=temp.uid;
 
    //open fifo to output the result
	localpath[0]=0;
	strcat(localpath,TMPDIR);
	strcat(localpath,"/");
	strcat(localpath,itoa(temp.uid));
	strcat(localpath,"_deq");
	if((jobfifo=open(localpath,O_WRONLY))<0){
		printf("open localpath error!\n");
		exit(0);
	}

   //search the job and kill it
	p=head;
	while(p!=NULL){
		if(p->job.jid==jid){
			break;
		}
		else{
			pre=p;
			p=p->next;
		}
	}
	if(p==NULL){
		write(jobfifo,info1,13);
	}
	else if(p->job.uid!=uid){
		write(jobfifo,info2,16);
	}
	else{
		if(p==head){
			head=head->next;
			kill(p->job.pid,SIGTERM);
			free(p);
		}
		else{
			pre->next=p->next;
			kill(p->job.pid,SIGTERM);
			free(p);
		}
		write(jobfifo,info3,16);
	}
	//close fifo

	close(jobfifo);
}
void do_stat(struct jobnode temp){
	int uid;
	char localpath[MAXPATH];
	struct queue *p;
	char tempbuff[200],*q,*r;
	struct passwd *userinfo;
	int jobfifo;
	//open localfifo
 
	uid=temp.uid;
	localpath[0]=0;
	strcat(localpath,TMPDIR);
	strcat(localpath,"/");
	strcat(localpath,itoa(temp.uid));
	strcat(localpath,"_stat");
	if((jobfifo=open(localpath,O_WRONLY))<0){
		printf("open localpath error!\n");
		exit(0);
	}

    //output job info

	p=head;
	if(p==NULL){
		strcpy(tempbuff,"there are no jobs now.\n");
		write(jobfifo,tempbuff,strlen(tempbuff));
		close(jobfifo);
		return ;
	}
	tempbuff[0]=0;
	strcpy(tempbuff,"--------------------------------------------------------------------------------------------------------------------\n");
	strcat(tempbuff,"jid\tpid\tu_name\tr_time(ms)\tw_time(ms)\tc_time\t\t\t\tstate\t\tcommand\t\t\n");
	write(jobfifo,tempbuff,strlen(tempbuff));
	while(p!=NULL){
		tempbuff[0]=0;
		strcat(tempbuff,itoa(p->job.jid));strcat(tempbuff,"\t");
		strcat(tempbuff,itoa(p->job.pid));strcat(tempbuff,"\t");
		userinfo=getpwuid(p->job.uid);
		strcat(tempbuff,userinfo->pw_name);strcat(tempbuff,"\t");
		strcat(tempbuff,itoa(p->job.runtime));strcat(tempbuff,"\t\t");
		strcat(tempbuff,itoa(p->job.waittime));strcat(tempbuff,"\t\t");
		strcat(tempbuff,ctime(&(p->job.create_time)));tempbuff[strlen(tempbuff)-1]=0;strcat(tempbuff,"\t");
		if(p->job.state==READY){
			strcat(tempbuff,"ready");
		}
		else if(p->job.state==RUNNING){
			strcat(tempbuff,"running");
		}
		else{
			strcat(tempbuff,"done");
		}
		strcat(tempbuff,"\t\t");
		q=p->job.data;r=q;
		while(*q!=0){
			if(*q==':'){
				*q=0;
				strcat(tempbuff,r);
				strcat(tempbuff," ");
				*q=':';
				q++;r=q;
			}
			else{
				q++;
			}
		}
		strcat(tempbuff,"\n");
		strcat(tempbuff,"--------------------------------------------------------------------------------------------------------------------\n");
		write(jobfifo,tempbuff,strlen(tempbuff));
		p=p->next;
	}
	close(jobfifo);
}
void selectjob(){
	struct queue *p,*highest;
	int highpri;
	//current job
	if(current!=NULL){
		if(current->job.state==RUNNING){
			kill(current->job.pid,SIGSTOP);
			current->job.state=READY;
		}
		else{
			p=head;
			if(p==current){
				head=head->next;
			}
			else{
				while(p->next!=current)
					p=p->next;
				p->next=current->next;
			}
			free(current);
		}
		current=NULL;
	}
//search job with highest privilage
	if(head==NULL){
		return ;
	}
	p=head;highpri=-1;
	while(p!=NULL){
		if(p->job.curpri>highpri){
			highpri=p->job.curpri;
			highest=p;
		}
		p=p->next;
	}
	current=highest;
}
void executejob(){

	if(current==NULL)
		return ;
	current->job.state=RUNNING;

	printf("Pid: %d\tPrirority: %d\n",current->job.pid,current->job.curpri);


	kill(current->job.pid,SIGCONT);

}


















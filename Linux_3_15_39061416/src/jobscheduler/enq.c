#include "scheduler.h"
#include<errno.h>

struct jobnode temp;
int fifo;
int localfifo;
char localpath[MAXPATH];

void welcome();
void help();
void transform(int argc, char * argv[]);
char * itoa(int);
void init();
void writetofifo();
void printtoscreen();
void exitfree();

int main(int argc, char *argv[])
{
	welcome();
	transform(argc,argv);
	init();
	writetofifo();
	printtoscreen();
	exitfree();
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
	printf("Usage:\t enq [-p num] e_file args\n");
	printf("///////////////////////////////////////////////\n");
	printf("\n\n");
}

void help(){
	printf("Usage:\t enq [-p num] e_file args\n");
}
void transform(int argc, char *argv[]){
	char tempcmd[BUFLEN];
	int t;
	if(argc==1||((strcmp(argv[1],"-p")==0)&&argc<4)){
		help();
		exit(0);
	}
	temp.type=ENQ;
	temp.uid=getuid();
	//determine the privilage

	if(strcmp(argv[1],"-p")==0){
		if(argc==2){
			t=0;
		}
		else{
			t=atoi(argv[2]);
		}
		if(t<0||t>3){
			t=0;
		}
		temp.defpri=t;
	}
	else{
		t=0;
	}
	//get command

	strcpy(tempcmd,"");
	if(strcmp(argv[1],"-p")==0){
		int i;
		for(i=3;i<argc;i++){
			strcat(tempcmd,argv[i]);
			strcat(tempcmd,":");
		}
	}
	else{
		int i;
		for(i=1;i<argc;i++){
			strcat(tempcmd,argv[i]);
			strcat(tempcmd,":");
		}
	}
	strcpy(temp.data,tempcmd);
}
char * itoa(int num){
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
void init(){
	localpath[0]=0;
	strcat(localpath,TMPDIR);
	strcat(localpath,"/");
	strcat(localpath,itoa(temp.uid));
	if((mkfifo(localpath,0666)<0)&&errno!=EEXIST){
		printf("make localfifo error!\n");
		exit(0);
	}
	if((fifo=open(MYFIFO,O_WRONLY))<0){
		printf("open fifo error!\n");
		exit(0);
	}
}
void writetofifo(){
	write(fifo,&temp,DATALEN);
}
void printtoscreen(){
	int rdnum;
	char buff[100];
	if((localfifo=open(localpath,O_RDONLY))<0){
		printf("open local fifo error!\n");
		exit(0);
	}
	while((rdnum=read(localfifo,buff,100))>0){
		write(1,buff,rdnum);
	}
}
void exitfree(){
	close(fifo);
	close(localfifo);
}

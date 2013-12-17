#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<string.h>
#include<ctype.h>
#include<time.h>
#include "fatsys.h"



void welcome()
{
	printf("\n//////////////////////////////////////////////\n");
	printf("\tWelcome to File System version 1.0\n");
	printf("\tAuthor:39061416\tHuang Jianyu\n");
	printf("\tWebsite:www.huangjy.info\n");
	printf("\tMobile Phone:15210965935\n");
	printf("\tEmail:hjyahead@gmail.com\n");
	printf("///////////////////////////////////////////////\n");
	printf("\n\n");
}


//change time and date
void changeTimeDate(unsigned char Timeinfo[2],unsigned char Dateinfo[2])
{
	time_t rawtime;
	struct tm *curtm;
	
	time(&rawtime);
	curtm = localtime(&rawtime);
	
	Timeinfo[0] = 0;	Timeinfo[1] = 0;
	Timeinfo[0] |= ((curtm->tm_sec >> 2)&0x1f)&0xff;
	Timeinfo[0] |= ((curtm->tm_min << 5)&0x7e0)&0xff;
	Timeinfo[0] |= ((curtm->tm_hour<< 11) & 0xf800)&0xff;

	Timeinfo[1] |= (((curtm->tm_sec >> 2)&0x1f)&0xff00)>>8;
	Timeinfo[1] |= (((curtm->tm_min << 5) & 0x7e0)&0xff00)>>8;
	Timeinfo[1] |= (((curtm->tm_hour << 11) & 0xf800)&0xff00)>>8;
	
	
	Dateinfo[0] = 0;	Dateinfo[1] = 0;
	Dateinfo[0] |= ((curtm->tm_mday) & 0x1F)&0xff;
	Dateinfo[0] |= (((curtm->tm_mon + 1) << 5) & 0x1E0)&0xff;
	Dateinfo[0] |= (((curtm->tm_year - 80) << 9) & 0xFE00)&0xff;
	
	Dateinfo[1] |= (((curtm->tm_mday) & 0x1F)&0xff00)>>8;
	Dateinfo[1] |= ((((curtm->tm_mon + 1) << 5) & 0x1E0)&0xff00)>>8;
	Dateinfo[1] |= ((((curtm->tm_year - 80) << 9) & 0xFE00)&0xff00)>>8;
	
}



//find the date
void transformDate(unsigned short *year,
			  unsigned short *month,
			  unsigned short *day,
			  unsigned char info[2])
{
	int date;
	date = CombineByte(info[0],info[1]);

	*year = ((date & MASK_YEAR)>> 9 )+1980;
	*month = ((date & MASK_MONTH)>> 5);
	*day = (date & MASK_DAY);
}

//find the time
void transformTime(unsigned short *hour,
			  unsigned short *min,
			  unsigned short *sec,
			  unsigned char info[2])
{
	int time;
	time = CombineByte(info[0],info[1]);

	*hour = ((time & MASK_HOUR )>>11);
	*min = (time & MASK_MIN)>> 5;
	*sec = (time & MASK_SEC) * 2;
}

//filename format transform
void TransformFileName(unsigned char *name)
{
	unsigned char *p = name;
	while(*p!='\0')
		p++;
	p--;
	while(*p==' ')
		p--;
	p++;
	*p = '\0';
}

//scan the boot sector:initial part
void initBootSec()
{
	unsigned char buf[SECTOR_SIZE];
	int ret,i;

	if((ret = read(fd,buf,SECTOR_SIZE))<0)
		perror("read boot sector failed");
	for(i = 0; i < 8; i++)
		bdptor.Oem_name[i] = buf[i+0x03];
	bdptor.Oem_name[i] = '\0';

	bdptor.BytesPerSector = CombineByte(buf[0x0b],buf[0x0c]);
	bdptor.SectorsPerCluster = buf[0x0d];
	bdptor.ReservedSectors = CombineByte(buf[0x0e],buf[0x0f]);
	bdptor.FATs = buf[0x10];
	bdptor.RootDirEntries = CombineByte(buf[0x11],buf[0x12]);    
	bdptor.LogicSectors = CombineByte(buf[0x13],buf[0x14]);
	bdptor.MediaType = buf[0x15];
	bdptor.SectorsPerFAT = CombineByte( buf[0x16],buf[0x17] );
	bdptor.SectorsPerTrack = CombineByte(buf[0x18],buf[0x19]);
	bdptor.Heads = CombineByte(buf[0x1a],buf[0x1b]);
	bdptor.HiddenSectors = CombineByte(buf[0x1c],buf[0x1d]);

	printf("-------------------------------------------------\n");
	printf("|\tOem_name \t\t|\t%s\t|\n"
		"|\tBytesPerSector \t\t|\t%d\t|\n"
		"|\tSectorsPerCluster \t|\t%d\t|\n"
		"|\tReservedSector \t\t|\t%d\t|\n"
		"|\tFATs \t\t\t|\t%d\t|\n"
		"|\tRootDirEntries \t\t|\t%d\t|\n"
		"|\tLogicSectors \t\t|\t%d\t|\n"
		"|\tMedioType \t\t|\t%d\t|\n"
		"|\tSectorPerFAT \t\t|\t%d\t|\n"
		"|\tSectorPerTrack \t\t|\t%d\t|\n"
		"|\tHeads \t\t\t|\t%d\t|\n"
		"|\tHiddenSectors \t\t|\t%d\t|\n",
		bdptor.Oem_name,
		bdptor.BytesPerSector,
		bdptor.SectorsPerCluster,
		bdptor.ReservedSectors,
		bdptor.FATs,
		bdptor.RootDirEntries,
		bdptor.LogicSectors,
		bdptor.MediaType,
		bdptor.SectorsPerFAT,
		bdptor.SectorsPerTrack,
		bdptor.Heads,
		bdptor.HiddenSectors);
		printf("-------------------------------------------------\n");
}



//get the directory entry
int GetEntry(struct Entry *pentry)
{
	int ret,i;
	int count = 0;
	unsigned char buf[DIR_ENTRY_SIZE], info[2];

	if( (ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
		perror("read entry failed");
	count += ret;

	if(buf[0]==0xe5 || buf[0]== 0x00)
		return -1*count;
	else
	{
		while (buf[11]== 0x0f) 
		{
			if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
				perror("read root dir failed");
			count += ret;
		}

		for (i=0 ;i<=10;i++)
			pentry->short_name[i] = buf[i];
		pentry->short_name[i] = '\0';

		TransformFileName(pentry->short_name); 



		info[0]=buf[22];
		info[1]=buf[23];
		transformTime(&(pentry->hour),&(pentry->min),&(pentry->sec),info);  

		info[0]=buf[24];
		info[1]=buf[25];
		transformDate(&(pentry->year),&(pentry->month),&(pentry->day),info);

		pentry->FirstCluster = CombineByte(buf[26],buf[27]);
		pentry->size = CombineWord(buf[28],buf[29],buf[30],buf[31]);

		pentry->readonly = (buf[11] & ATTR_READONLY) ?1:0;
		pentry->hidden = (buf[11] & ATTR_HIDDEN) ?1:0;
		pentry->system = (buf[11] & ATTR_SYSTEM) ?1:0;
		pentry->vlabel = (buf[11] & ATTR_VLABEL) ?1:0;
		pentry->subdir = (buf[11] & ATTR_SUBDIR) ?1:0;
		pentry->archive = (buf[11] & ATTR_ARCHIVE) ?1:0;

		return count;
	}
}

//ls command
int ud_ls()
{
	int ret, offset,cluster_addr;
	struct Entry entry;
	unsigned char buf[DIR_ENTRY_SIZE];
	if( (ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
		perror("read entry failed");
	if(curdir==NULL)
		printf("Root_dir\n");
	else
		printf("%s_dir\n",curdir->short_name);
		
	printf("---------------------------------------------------------------------------\n");
	
	printf("| %-12s | %-10s | %-8s | %-7s | %-4s | %-4s |\n","NAME","DATE","TIME","CLUSTER","SIZE","ATTR");
	

	if(curdir==NULL)//root dir
	{

		if((ret= lseek(fd,ROOTDIR_OFFSET,SEEK_SET))<0)
			perror("lseek ROOTDIR_OFFSET failed");

		offset = ROOTDIR_OFFSET;

		while(offset < (DATA_OFFSET))
		{
			ret = GetEntry(&entry);

			offset += abs(ret);
			if(ret > 0)
			{
				printf("| %-12s | %-4d/%-2d/%-2d | %-2d:%-2d:%-2d | %-7d | %-4d | %-4s |\n",
					entry.short_name,
					entry.year,entry.month,entry.day,
					entry.hour,entry.min,entry.sec,
					entry.FirstCluster,
					(entry.subdir) ? 0:entry.size,
					(entry.subdir) ? "dir":"file");
			}
		}
	}

	else
	{
		cluster_addr = DATA_OFFSET + (curdir->FirstCluster-2) * CLUSTER_SIZE ;
		if((ret = lseek(fd,cluster_addr,SEEK_SET))<0)
			perror("lseek cluster_addr failed");

		offset = cluster_addr;

		while(offset<cluster_addr +CLUSTER_SIZE)
		{
			ret = GetEntry(&entry);
			offset += abs(ret);
			if(ret > 0)
			{
					printf("| %-12s | %-4d/%-2d/%-2d | %-2d:%-2d:%-2d | %-7d | %-4d | %-4s |\n",
					entry.short_name,
					entry.year,entry.month,entry.day,
					entry.hour,entry.min,entry.sec,
					entry.FirstCluster,
					(entry.subdir) ? 0:entry.size,
					(entry.subdir) ? "dir":"file");
			}
		}
	}
	
	
	printf("---------------------------------------------------------------------------\n");
	return 0;
} 


//scan the entry
int ScanEntry (char *entryname,struct Entry *pentry,int mode)
{
	int ret,offset,i;
	int cluster_addr;
	char uppername[80];
	for(i=0;i< strlen(entryname);i++)
		uppername[i]= toupper(entryname[i]);
	uppername[i]= '\0';

	if(curdir ==NULL)  
	{
		if((ret = lseek(fd,ROOTDIR_OFFSET,SEEK_SET))<0)
			perror ("lseek ROOTDIR_OFFSET failed");
		offset = ROOTDIR_OFFSET;


		while(offset<DATA_OFFSET)
		{
			ret = GetEntry(pentry);
			offset +=abs(ret);

			if(pentry->subdir == mode &&!strcmp((char*)pentry->short_name,uppername))

				return offset;

		}
		return -1;
	}

	else  
	{
		cluster_addr = DATA_OFFSET + (curdir->FirstCluster -2)*CLUSTER_SIZE;
		if((ret = lseek(fd,cluster_addr,SEEK_SET))<0)
			perror("lseek cluster_addr failed");
		offset= cluster_addr;

		while(offset<cluster_addr + CLUSTER_SIZE)
		{
			ret= GetEntry(pentry);
			offset += abs(ret);
			if(pentry->subdir == mode &&!strcmp((char*)pentry->short_name,uppername))
				return offset;



		}
		return -1;
	}
}


//cd command
int ud_cd(char *dir)
{
	struct Entry *pentry;
	int ret;
	
	char *nextdir,*curcurdir;

	if(!strcmp(dir,"."))
	{
		return 1;
	}
	if(!strcmp(dir,"..") && curdir==NULL)
		return 1;

	if(!strcmp(dir,"..") && curdir!=NULL)
	{
		curdir = fatherdir[dirno];
		dirno--; 
		return 1;
	}
	
	nextdir = dir;
	while( (curcurdir = strsep(&nextdir,"/")))
	{
		pentry = (struct Entry*)malloc(sizeof(struct Entry));
		ret = ScanEntry(curcurdir,pentry,1);
		if(ret < 0)
		{
			printf("no such dir\n");
			free(pentry);
			return -1;
		}
		dirno ++;
		fatherdir[dirno] = curdir;
		curdir = pentry;	
	}
	
	return 1;
}

//get the fat cluster
unsigned short GetFatCluster(unsigned short prev)
{
	unsigned short next;
	int index;

	index = prev * 2;
	next = CombineByte(fatbuf[index],fatbuf[index+1]);

	return next;
}

//clear the fat cluster
void ClearFatCluster(unsigned short cluster)
{
	int index;
	index = cluster * 2;

	fatbuf[index]=0x00;
	fatbuf[index+1]=0x00;

}


//write fat
int WriteFat()
{
	if(lseek(fd,FAT_ONE_OFFSET,SEEK_SET)<0)
	{
		perror("lseek failed");
		return -1;
	}
	if(write(fd,fatbuf,512*250)<0)
	{
		perror("read failed");
		return -1;
	}
	if(lseek(fd,FAT_TWO_OFFSET,SEEK_SET)<0)
	{
		perror("lseek failed");
		return -1;
	}
	if((write(fd,fatbuf,512*250))<0)
	{
		perror("read failed");
		return -1;
	}
	return 1;
}

//read fat
int ReadFat()
{
	if(lseek(fd,FAT_ONE_OFFSET,SEEK_SET)<0)
	{
		perror("lseek failed");
		return -1;
	}
	if(read(fd,fatbuf,512*250)<0)
	{
		perror("read failed");
		return -1;
	}
	return 1;
}

//rmdir command
int ud_rmdir(char *filename)
{
	struct Entry *pentry;
	int ret;
	unsigned char c;
	unsigned short seed,next;

	pentry = (struct Entry*)malloc(sizeof(struct Entry));

	ret = ScanEntry(filename,pentry,1);
	if(ret<0)
	{
		printf("no such file\n");
		free(pentry);
		return -1;
	}

	seed = pentry->FirstCluster;
	while((next = GetFatCluster(seed))!=0xffff)
	{
		ClearFatCluster(seed);
		seed = next;

	}

	ClearFatCluster( seed );

	c=0xe5;


	if(lseek(fd,ret-0x20,SEEK_SET)<0)
		perror("lseek ud_df failed");
	if(write(fd,&c,1)<0)
		perror("write failed");  


	if(lseek(fd,ret-0x40,SEEK_SET)<0)
		perror("lseek ud_df failed");
	if(write(fd,&c,1)<0)
		perror("write failed");

	free(pentry);
	if(WriteFat()<0)
		exit(1);
	return 1;
}


//df command

int ud_df(char *filename)
{
	struct Entry *pentry;
	int ret;
	unsigned char c;
	unsigned short seed,next;

	pentry = (struct Entry*)malloc(sizeof(struct Entry));

	ret = ScanEntry(filename,pentry,0);
	if(ret<0)
	{
		printf("no such file\n");
		free(pentry);
		return -1;
	}

	seed = pentry->FirstCluster;
	while((next = GetFatCluster(seed))!=0xffff)
	{
		ClearFatCluster(seed);
		seed = next;

	}

	ClearFatCluster( seed );

	c=0xe5;


	if(lseek(fd,ret-0x20,SEEK_SET)<0)
		perror("lseek ud_df failed");
	if(write(fd,&c,1)<0)
		perror("write failed");  


	if(lseek(fd,ret-0x40,SEEK_SET)<0)
		perror("lseek ud_df failed");
	if(write(fd,&c,1)<0)
		perror("write failed");

	free(pentry);
	if(WriteFat()<0)
		exit(1);
	return 1;
}


//mkdir command
int ud_mkdir(char *filename)
{
	int size = 100;
	struct Entry *pentry;
	int ret,i=0,cluster_addr,offset;
	unsigned short cluster,clusterno[100];
	unsigned char c[DIR_ENTRY_SIZE];
	int index,clustersize;
	unsigned char buf[DIR_ENTRY_SIZE];
	pentry = (struct Entry*)malloc(sizeof(struct Entry));


	clustersize = (size / (CLUSTER_SIZE));

	if(size % (CLUSTER_SIZE) != 0)
		clustersize ++;

	ret = ScanEntry(filename,pentry,0);
	if (ret<0)
	{
		for(cluster=2;cluster<1000;cluster++)
		{
			index = cluster *2;
			if(fatbuf[index]==0x00&&fatbuf[index+1]==0x00)
			{
				clusterno[i] = cluster;

				i++;
				if(i==clustersize)
					break;

			}

		}

		for(i=0;i<clustersize-1;i++)
		{
			index = clusterno[i]*2;

			fatbuf[index] = (clusterno[i+1] &  0x00ff);
			fatbuf[index+1] = ((clusterno[i+1] & 0xff00)>>8);


		}
		index = clusterno[i]*2;
		fatbuf[index] = 0xff;
		fatbuf[index+1] = 0xff;

		if(curdir==NULL) 
		{ 

			if((ret= lseek(fd,ROOTDIR_OFFSET,SEEK_SET))<0)
				perror("lseek ROOTDIR_OFFSET failed");
			offset = ROOTDIR_OFFSET;
			while(offset < DATA_OFFSET)
			{
				if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
					perror("read entry failed");

				offset += abs(ret);

				if(buf[0]!=0xe5&&buf[0]!=0x00)
				{
					while(buf[11] == 0x0f)
					{
						if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
							perror("read root dir failed");
						offset +=abs(ret);
					}
				}

				else
				{       
					offset = offset-abs(ret);     
					for(i=0;i<=strlen(filename);i++)
					{
						c[i]=toupper(filename[i]);
					}			
					for(;i<=10;i++)
						c[i]=' ';

					//c[11] = 0x01;
					c[11] = 0x11;
					
					
					changeTimeDate(&c[22],&c[24]);
					//changeDate(&c[24]);

					c[26] = (clusterno[0] &  0x00ff);
					c[27] = ((clusterno[0] & 0xff00)>>8);

					c[28] = (size &  0x000000ff);
					c[29] = ((size & 0x0000ff00)>>8);
					c[30] = ((size& 0x00ff0000)>>16);
					c[31] = ((size& 0xff000000)>>24);

					if(lseek(fd,offset,SEEK_SET)<0)
						perror("lseek ud_cf failed");
					if(write(fd,&c,DIR_ENTRY_SIZE)<0)
						perror("write failed");




					free(pentry);
					if(WriteFat()<0)
						exit(1);
						
						
					ud_cd(filename);
					ud_mkdir(".");
					ud_mkdir("..");
					ud_cd("..");

					return 1;
				}

			}
		}
		else 
		{
			cluster_addr = (curdir->FirstCluster -2 )*CLUSTER_SIZE + DATA_OFFSET;
			if((ret= lseek(fd,cluster_addr,SEEK_SET))<0)
				perror("lseek cluster_addr failed");
			offset = cluster_addr;
			while(offset < cluster_addr + CLUSTER_SIZE)
			{
				if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
					perror("read entry failed");

				offset += abs(ret);

				if(buf[0]!=0xe5&&buf[0]!=0x00)
				{
					while(buf[11] == 0x0f)
					{
						if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
							perror("read root dir failed");
						offset +=abs(ret);
					}
				}
				else
				{ 
					offset = offset - abs(ret);      
					for(i=0;i<=strlen(filename);i++)
					{
						c[i]=toupper(filename[i]);
					}
					for(;i<=10;i++)
						c[i]=' ';

					c[11] = 0x11;


					changeTimeDate(&c[22],&c[24]);
										
					c[26] = (clusterno[0] &  0x00ff);
					c[27] = ((clusterno[0] & 0xff00)>>8);

					c[28] = (size &  0x000000ff);
					c[29] = ((size & 0x0000ff00)>>8);
					c[30] = ((size& 0x00ff0000)>>16);
					c[31] = ((size& 0xff000000)>>24);

					if(lseek(fd,offset,SEEK_SET)<0)
						perror("lseek ud_cf failed");
					if(write(fd,&c,DIR_ENTRY_SIZE)<0)
						perror("write failed");




					free(pentry);
					if(WriteFat()<0)
						exit(1);
					
					//ud_cd(filename);
					//ud_mkdir(".");
					//ud_mkdir("..");
					//ud_cd("..");

					return 1;
				}

			}
		}
	}
	else
	{
		printf("This filename is exist\n");
		free(pentry);
		return -1;
	}
	return 1;

}


//cf command
int ud_cf(char *filename,int size)
{

	struct Entry *pentry;
	int ret,i=0,cluster_addr,offset;
	unsigned short cluster,clusterno[100];
	unsigned char c[DIR_ENTRY_SIZE];
	int index,clustersize;
	unsigned char buf[DIR_ENTRY_SIZE];
	pentry = (struct Entry*)malloc(sizeof(struct Entry));


	clustersize = (size / (CLUSTER_SIZE));

	if(size % (CLUSTER_SIZE) != 0)
		clustersize ++;

	ret = ScanEntry(filename,pentry,0);
	if (ret<0)
	{
		for(cluster=2;cluster<1000;cluster++)
		{
			index = cluster *2;
			if(fatbuf[index]==0x00&&fatbuf[index+1]==0x00)
			{
				clusterno[i] = cluster;

				i++;
				if(i==clustersize)
					break;

			}

		}

		for(i=0;i<clustersize-1;i++)
		{
			index = clusterno[i]*2;

			fatbuf[index] = (clusterno[i+1] &  0x00ff);
			fatbuf[index+1] = ((clusterno[i+1] & 0xff00)>>8);


		}
		index = clusterno[i]*2;
		fatbuf[index] = 0xff;
		fatbuf[index+1] = 0xff;

		if(curdir==NULL)
		{ 

			if((ret= lseek(fd,ROOTDIR_OFFSET,SEEK_SET))<0)
				perror("lseek ROOTDIR_OFFSET failed");
			offset = ROOTDIR_OFFSET;
			while(offset < DATA_OFFSET)
			{
				if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
					perror("read entry failed");

				offset += abs(ret);

				if(buf[0]!=0xe5&&buf[0]!=0x00)
				{
					while(buf[11] == 0x0f)
					{
						if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
							perror("read root dir failed");
						offset +=abs(ret);
					}
				}


				else
				{       
					offset = offset-abs(ret);     
					for(i=0;i<=strlen(filename);i++)
					{
						c[i]=toupper(filename[i]);
					}			
					for(;i<=10;i++)
						c[i]=' ';

					//c[11] = 0x01;
					c[11] = 0x01;
					
					
					changeTimeDate(&c[22],&c[24]);
					//changeDate(&c[24]);

					c[26] = (clusterno[0] &  0x00ff);
					c[27] = ((clusterno[0] & 0xff00)>>8);

					c[28] = (size &  0x000000ff);
					c[29] = ((size & 0x0000ff00)>>8);
					c[30] = ((size& 0x00ff0000)>>16);
					c[31] = ((size& 0xff000000)>>24);

					if(lseek(fd,offset,SEEK_SET)<0)
						perror("lseek ud_cf failed");
					if(write(fd,&c,DIR_ENTRY_SIZE)<0)
						perror("write failed");




					free(pentry);
					if(WriteFat()<0)
						exit(1);
						
						


					return 1;
				}

			}
		}
		else 
		{
			cluster_addr = (curdir->FirstCluster -2 )*CLUSTER_SIZE + DATA_OFFSET;
			if((ret= lseek(fd,cluster_addr,SEEK_SET))<0)
				perror("lseek cluster_addr failed");
			offset = cluster_addr;
			while(offset < cluster_addr + CLUSTER_SIZE)
			{
				if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
					perror("read entry failed");

				offset += abs(ret);

				if(buf[0]!=0xe5&&buf[0]!=0x00)
				{
					while(buf[11] == 0x0f)
					{
						if((ret = read(fd,buf,DIR_ENTRY_SIZE))<0)
							perror("read root dir failed");
						offset +=abs(ret);
					}
				}
				else
				{ 
					offset = offset - abs(ret);      
					for(i=0;i<=strlen(filename);i++)
					{
						c[i]=toupper(filename[i]);
					}
					for(;i<=10;i++)
						c[i]=' ';

					c[11] = 0x01;


					changeTimeDate(&c[22],&c[24]);
										
					c[26] = (clusterno[0] &  0x00ff);
					c[27] = ((clusterno[0] & 0xff00)>>8);

					c[28] = (size &  0x000000ff);
					c[29] = ((size & 0x0000ff00)>>8);
					c[30] = ((size& 0x00ff0000)>>16);
					c[31] = ((size& 0xff000000)>>24);

					if(lseek(fd,offset,SEEK_SET)<0)
						perror("lseek ud_cf failed");
					if(write(fd,&c,DIR_ENTRY_SIZE)<0)
						perror("write failed");




					free(pentry);
					if(WriteFat()<0)
						exit(1);

					return 1;
				}

			}
		}
	}
	else
	{
		printf("This filename is exist\n");
		free(pentry);
		return -1;
	}
	return 1;

}

void help()
{
	printf("please input a command, including followings:\n\tls\t\t\tlist all files\n\tcd <dir>\t\tchange direcotry\n\tcf <filename> <size>\tcreate a file\n\tdf <file>\t\tdelete a file\n\tmkdir <dirname>\t\tcreate a directory\n\trmdir <dir>\t\tdelete a directory\n\texit\t\t\texit this system\n");
}


int main()
{
	struct Entry *pentry;
	char input[10];
	int size=0;
	int ret;
	char name[12];
	
	welcome();
	
	if((fd = open(DEVNAME,O_RDWR))<0)
		perror("open failed");
	initBootSec();
	if(ReadFat()<0)
		exit(1);
	help();
	
	/*
	pentry = (struct Entry*)malloc(sizeof(struct Entry));
	ret = ScanEntry(".",pentry,1);
	if(ret < 0)
	{
		ud_mkdir(".");
		free(pentry);
	}
	ret = ScanEntry("..",pentry,1);
	if(ret < 0)
	{
		ud_mkdir("..");
		free(pentry);
	}
	*/
	
	while (1)
	{
		printf(">");
		scanf("%s",input);

		if (strcmp(input, "exit") == 0)
			break;
		else if (strcmp(input, "ls") == 0)
			ud_ls();
		else if(strcmp(input, "cd") == 0)
		{
			scanf("%s", name);
			ud_cd(name);
		}
		else if(strcmp(input, "df") == 0)
		{
			scanf("%s", name);
			ud_df(name);
		}
		else if(strcmp(input, "cf") == 0)
		{
			scanf("%s", name);
			scanf("%s", input);
			size = atoi(input);
			ud_cf(name,size);
		}
		else if(strcmp(input, "mkdir") == 0)
		{
			scanf("%s", name);
			ud_mkdir(name);
		}
		else if(strcmp(input, "rmdir") == 0)
		{
			scanf("%s", name);
			ud_rmdir(name);
		}
		else
			help();
	}	

	return 0;
}




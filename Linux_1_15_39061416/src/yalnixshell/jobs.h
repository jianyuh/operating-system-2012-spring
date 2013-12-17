//get the job by searchValue according to the searchParameter
Job* getJob(int searchValue, int searchParameter)
{
	usleep(10000);
	int i,found;
	Job* job = jobHead;
	found =0;
	switch (searchParameter) {
	case BY_PROCESS_ID:
		while (job != NULL) {
			/*
			i=0;
			for (i=0;job->pid[i]!=-1;i++)
			{
				if(i>=MAXPIPE + 1)
				{
					found =0;
					break;
				}
				if(job->pid[i] == searchValue )
				{
					found = 1;
					return job;
					//break;
				}
			}
			//if(found ==1) break;
			*/

			if (job->groupid == searchValue)
				return job;
			else
				job = job->next;
		}
		break;
	case BY_JOB_ID:
		while (job != NULL) {
			if (job->id == searchValue)
				return job;
			else
				job = job->next;
		}
		break;
	case BY_JOB_STATUS:
		while (job != NULL) {
			if (job->status == searchValue)
				return job;
			else
				job = job->next;
		}
		break;
	default:
		return NULL;
		break;
	}
	return NULL;
}



//modifies the status of a job
int changeJobStatus(int pgid, int status)
{
        usleep(10000);
        Job *job = jobHead;
        if (job == NULL) {
                return 0;
        } else {
                int counter = 0;
                while (job != NULL) {
                        if (job->groupid == pgid) {
                                job->status = status;
                                return TRUE;
                        }
                        counter++;
                        job = job->next;
                }
                return FALSE;
        }
}

//deletes a no more active process from the linked list
Job* delJob(Job* job)
{
        usleep(10000);
        if (jobHead == NULL)
                return NULL;
        Job* currentJob;
        Job* beforeCurrentJob;

        currentJob = jobHead->next;
        beforeCurrentJob = jobHead;

        if (beforeCurrentJob->groupid == job->groupid) {

                beforeCurrentJob = beforeCurrentJob->next;
                numActiveJobs--;
                return currentJob;
        }

        while (currentJob != NULL) {
                if (currentJob->groupid == job->groupid) {
                        numActiveJobs--;
                        beforeCurrentJob->next = currentJob->next;
						free(currentJob);
                }
                beforeCurrentJob = currentJob;
                currentJob = currentJob->next;
        }
        return jobHead;
}


//prints the active processes launched by the shell
void printJobs()
{
        printf("\nJobs List:\n");
        printf(
                "---------------------------------------------------------------------------\n");
        printf("| %7s  | %30s | %7s |  %6s |\n", "job no.", "command", "groupid",
                "status");
        printf(
                "---------------------------------------------------------------------------\n");
        Job* job = jobHead;
        if (job == NULL) {
                printf("| %s %62s |\n", "No Jobs.", "");
        } else {
                while (job != NULL) {
                        printf("|  %7d | %30s | %7d | %6c |\n", job->id, job->cmd,
                               job->groupid, job->status);
                        job = job->next;
                }
        }
        printf(
                "---------------------------------------------------------------------------\n");
}


//inserts an active process in the linked list
Job* insertJob(int pid, pid_t pgid, char* name,int status)
{
        usleep(10000);
        Job *newJob = (Job *)malloc(sizeof(Job));

        strcpy(newJob->cmd, name);
        newJob->pid = pid;
        newJob->groupid = pgid;
        newJob->status = status;

        newJob->next = NULL;

        if (jobHead == NULL) {
                numActiveJobs++;
                newJob->id = numActiveJobs;
                return newJob;
        } else {
                Job *auxNode = jobHead;
                while (auxNode->next != NULL) {
                        auxNode = auxNode->next;
                }
                newJob->id = auxNode->id + 1;
                auxNode->next = newJob;
                numActiveJobs++;
                return jobHead;
        }
}

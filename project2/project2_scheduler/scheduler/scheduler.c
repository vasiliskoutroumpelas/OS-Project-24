/*
Vasileios Koutroumpelas, 1093397
Filippos Minopetros, 1093431
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

typedef struct node {
    char* name;
	pid_t pid;
	char* state;
	clock_t entry;
    struct node* next;
    struct node* prev;
}Process;

void insert_end(Process** head, char* name, int pid, char* state, clock_t entry)
{
	// create new node
	Process* new_node = (Process *)malloc(sizeof(struct node));
	new_node->name = (char *)malloc(sizeof(char)*100);
	new_node->state = (char *)malloc(sizeof(char)*10);

	// put data in new node
	strcpy(new_node->name, name);
	new_node->pid = pid;
	strcpy(new_node->state, state);
	new_node->entry = entry;

	// new node will be the last one so next is null
	new_node->next = NULL;
	
	// in case list is empty
	if (*head == NULL)
	{
		new_node->prev = NULL;
		*head = new_node;
		return;
	}
	
	// find the last node
	Process * temp = *head;
	while (temp->next!=NULL)
	{
		temp = temp->next;
	}

	temp->next = new_node;
	new_node->prev = temp;
	return;
}

Process* pop_first(Process** head) 
{
	// if none in the list
	if (*head == NULL)
	{
		printf("List is empty\n");
		return NULL;
	}

	// if only one in the list
	if ((*head)->next == NULL)
	{
		Process *temp = *head;
		*head = NULL;
		return temp;
	}
	Process *temp = *head;
	*head = (*head)->next;
	(*head)->prev = NULL;
	return temp;
}

void print_list(Process* list) {
    Process* current = list;

    while (current != NULL) {
        printf("Name: %s, PID: %d, State: %s, Entry: %ld\n",
               current->name, current->pid, current->state, (long)current->entry);
        current = current->next;
    }
	free(current);
}

void print_process(Process* process) {
	printf("Name: %s, PID: %d, State: %s\n",
			process->name, process->pid, process->state);
}

void print_time_since_entry(Process* process) {
	printf("Time since entry: %ld\n", process->entry);
}

void update_state(Process* process, const char* state) {
    strcpy(process->state, state);
}

void start_process(Process* process) 
{
	pid_t pid = fork();
	if (pid == 0)
	{
		// printf("%s\n", process->name);
		execl(process->name, process->name, NULL);
		perror("execl fail");
	}
	else
	{
		process->pid = pid;
	}
}


int child_finished;
void handle_child_finish(int sigid) {
	child_finished = 1;
}

void sleep_milliseconds(unsigned int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int main(int argc,char **argv)
{	
	if (argc < 3 || argc > 4)
	{
        printf("Usage: %s <policy> [<quantum>] <input_filename>\n", argv[0]);
        exit(1);
    }

	char *policy = argv[1];
	int quantum = 1000; 

	FILE* file; 

	if (argc == 4 && strcmp(policy, "RR")==0)
	{
		quantum = atoi(argv[2]);
		file = fopen(argv[3], "r");
		printf("Policy: %s\nQuantum: %d\n", policy, quantum);
	}
	else if (argc == 3 && strcmp(policy, "RR")==0)
	{
		file = fopen(argv[2], "r");
		printf("Policy: %s\nDefault Quantum: %d\n", policy, quantum);
	}
	else if (argc == 3 && strcmp(policy, "FCFS")==0)
	{
		quantum=2147483647; //max int
		file = fopen(argv[2], "r");
		printf("Policy: %s\n", policy);
	}
	else
	{
        printf("Usage: %s <policy> [<quantum>] <input_filename>\n", argv[0]);
        exit(1);
	}

    if (file == NULL) {
        perror("fopen");
        return 1;
    }

	Process *list;
	char name[100];
	
	while (fscanf(file, "%s\n", name) != EOF)
	{
		insert_end(&list, name, -1, "NEW", time(NULL));
	}
	
	fclose(file);
	// print_list(list);

	struct sigaction sact;
	sact.sa_handler = handle_child_finish;
	sact.sa_flags = SA_NOCLDSTOP;
	
	
	if (sigaction (SIGCHLD, &sact, NULL) < 0)
		perror("could not set action for SIGINT\n");
	


	time_t start = time(NULL);
	while (list != NULL)
	{
		Process* process = pop_first(&list);
		
		if (strcmp(process->state, "NEW")==0)
		{
			start_process(process);
			update_state(process, "RUNNING");
			printf("\n");
			print_process(process);
		}
		else
		{
			kill(process->pid, SIGCONT);
			update_state(process, "RUNNING");
			print_process(process);
		}
		
		sleep_milliseconds(quantum);
		
		if(child_finished)
		{
			update_state(process, "EXITED");
			process->entry = time(NULL)-start;
			print_process(process);
			print_time_since_entry(process);
			child_finished = 0;
		}
		else
		{
			kill(process->pid, SIGSTOP);
			update_state(process, "STOPPED");
			print_process(process);
			
			insert_end(&list, process->name, process->pid, "STOPPED", process->entry);
		}
	}
	time_t end = time(NULL);
	printf("\nTotal time was %ld sec\n", end-start);
	free(list);
	return 0;
}

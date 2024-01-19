/*
Vasileios Koutroumpelas, 1093397
Filippos Minopetros, 1093431
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

// structure that describes a process
typedef struct node {
    char* name;
	pid_t pid;
	char* state;
	double finish_time;
    struct node* next;
    struct node* prev;
}Process;

// helper function to get current time (copied from project1)
double get_wtime(void) {
  struct timeval t;
  gettimeofday(&t, NULL);
  return (double)t.tv_sec + (double)t.tv_usec * 1.0e-6;
}

void insert_end(Process** head, char* name, int pid, char* state)
{
	// create new node
	Process* new_node = (Process *)malloc(sizeof(struct node));
	new_node->name = (char *)malloc(sizeof(char)*100);
	new_node->state = (char *)malloc(sizeof(char)*10);

	// put data in new node
	strcpy(new_node->name, name);
	new_node->pid = pid;
	strcpy(new_node->state, state);

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

// display function, prints the details of all the process in the list 
void print_list(Process* list) {
    Process* current = list;

    while (current != NULL) {
        printf("Name: %s, PID: %d, State: %s",
               current->name, current->pid, current->state);
        current = current->next;
    }
	free(current);
}

// display function, prints the details of a single process
void print_process(Process* process) {
	printf("\nName: %s, PID: %d, State: %s\n",
			process->name, process->pid, process->state);
}

// display function, prints the finish timeof a process after exit
void print_time_since_entry(Process* process) {
	printf("Time since entry: %.2lf sec\n", process->finish_time);
}

// helper function, updates the state of a process
void update_state(Process* process, const char* state) {
    strcpy(process->state, state);
}

// function that starts a process via forking and executing the child process
void start_process(Process* process) 
{
	pid_t pid = fork();
	if (pid == 0) // child process
	{
		execl(process->name, process->name, NULL);
		perror("execl fail");
	}
	else if(pid < 0)
	{
		perror("fork");
		exit(1);
	}
	else // parent process
	{
		process->pid = pid;
	}
}

// global variables to be used by the scheduler
Process *list, *current_process;

// global variable which acts as a flag for the status of a process in execution
int child_finished=0;
void handle_child_finish(int sigid) { // SIGCHLD
	child_finished = 1;
}

// helper function that converts milliseconds to seconds and nanoseconds to be used by nanosleep
void sleep_milliseconds(unsigned int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int main(int argc,char **argv)
{
	// lines of code that secure the user is executing the program the right way	
	if (argc < 3 || argc > 4)
	{
        printf("Usage: %s <policy> [<quantum>] <input_filename>\n", argv[0]);
        exit(1);
    }

	// declaration of variables
	char *policy = argv[1];
	int quantum = 1000; 
	FILE* file; 
	
	// initialisation of variables based on the execution arguements
	if (argc == 4 && strcmp(policy, "RR")==0)
	{
		quantum = atoi(argv[2]);
		file = fopen(argv[3], "r");
		printf("Policy: %s\nQuantum: %d msec\n", policy, quantum);
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

	// insert all processes from the file to the queue
	char name[100];
	while (fscanf(file, "%s\n", name) != EOF)
	{
		insert_end(&list, name, -1, "NEW");
	}
	
	fclose(file);

	// defining action
	struct sigaction sact;
	sact.sa_handler = handle_child_finish;
	sact.sa_flags = SA_NOCLDSTOP;
	
	if (sigaction (SIGCHLD, &sact, NULL) < 0)
		perror("could not set action for SIGINT\n");
	

	// start of scheduler
	double start = get_wtime();
	while (list != NULL)
	{
		current_process = pop_first(&list); // get processs that waits first in the queue
		
		if (strcmp(current_process->state, "NEW")==0) // if not started, start
		{
			start_process(current_process);
			update_state(current_process, "RUNNING");
			print_process(current_process);
		}
		else // continue if process is stopped
		{
			kill(current_process->pid, SIGCONT);
			update_state(current_process, "RUNNING");
			print_process(current_process);
		}
		
		sleep_milliseconds(quantum); // wait for process to finish (FCFS) or wait process to be executed for seleted quantum
		
		if(child_finished) // if process finished update its state and print info
		{
			update_state(current_process, "EXITED");
			current_process->finish_time = get_wtime()-start;
			print_process(current_process);
			print_time_since_entry(current_process);
			child_finished = 0;
		}
		else // if process is not finished in selected quantum stops and is inserted back in queue
		{
			kill(current_process->pid, SIGSTOP);
			update_state(current_process, "STOPPED");
			print_process(current_process);
			
			insert_end(&list, current_process->name, current_process->pid, "STOPPED");
		}
	}
	double end = get_wtime();
	printf("\nTotal time was %.2lf sec\n", end-start);
	free(list);
	return 0;
}

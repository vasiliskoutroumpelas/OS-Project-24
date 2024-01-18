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
        printf("Name: %s, PID: %d, State: %s\n",
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
	printf("Time since entry: %lf\n", process->finish_time);
}

// helper function, updates the state of a process
void update_state(Process* process, const char* state) {
    strcpy(process->state, state);
}

// global variables which act as flags for the status of an IO execution
int io_start=0;
int io_finish=0;

// global variables to be used by the scheduler
Process *list, *current_process;

void handle_io_start(int sigid) { // SIGUSR1
	io_start=1;
}

void handle_io_finish(int sigid) { // SIGUSR2
	io_finish=1;
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
	FILE* file; 

	// initialisation of variables based on the execution arguements
	if (argc == 3 && strcmp(policy, "FCFS")==0)
	{
		file = fopen(argv[2], "r");
		printf("Policy: %s\n", policy);
	}
	else if ((argc == 3 || argc == 4) && strcmp(policy, "RR")==0)
	{
		printf("Round Robin is not available with processes that request IO\n");
		exit(1);
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
	pid_t pid;

	// defining actions
	struct sigaction sact_io_start, sact_io_finish;
    sact_io_start.sa_handler = handle_io_start;
    sact_io_finish.sa_handler = handle_io_finish;
	sact_io_start.sa_flags = 0;
	sact_io_finish.sa_flags = 0;
	
	if (sigaction (SIGUSR1, &sact_io_start, NULL) < 0)
		perror("could not set action for SIGUSR1\n");
	if (sigaction (SIGUSR2, &sact_io_finish, NULL) < 0)
		perror("could not set action for SIGUSR2\n");

	// start of scheduler
	double start = get_wtime();
	Process* process_io;
	while (list != NULL)
	{
		current_process = pop_first(&list); // get processs that waits first in the queue
		if (list == NULL && strcmp(current_process->state, "WAITING IO")==0 && !io_finish) // if there is no other process in queue and the process selected is waiting io, wait for io
		{
			printf("\nNo other process to be scheduled. Waiting IO to be finished.\n");
			while (!io_finish);
		}
		if (!io_finish || current_process->pid!=process_io->pid) // start a process via forking and executing the child process if io has not finished, or if it has finished and the process that requested it is not yet selected
		{
			pid = fork();
			if (pid == 0)
			{
				execl(current_process->name, current_process->name, NULL);
				perror("execl fail");
			}
			else
			{
				update_state(current_process, "RUNNING");
				current_process->pid = pid;
				print_process(current_process);
				waitpid(current_process->pid, NULL, 0);
				if (io_start) // if process is requesting io stops it and puts it back in queue
				{
					io_start=0;
					process_io = current_process;
					update_state(process_io, "WAITING IO");
					print_process(process_io);
					insert_end(&list, process_io->name, process_io->pid, "WAITING IO");
					continue;
				}
				else // if process has not requested io, it is being executed normally until it finishes
				{
					waitpid(current_process->pid, NULL, 0);
					current_process->finish_time = get_wtime() - start;
					update_state(current_process, "EXITED");
					print_process(current_process);
					print_time_since_entry(current_process);
				}
			}
		}
		else // if io has finished, continue the process that requested it until it finishes 
		{
			io_finish=0;
			kill(process_io->pid, SIGCONT);
			update_state(process_io, "RUNNING");
			print_process(process_io);
			waitpid(process_io->pid, NULL, 0);
			process_io->finish_time = get_wtime() - start;
			update_state(process_io, "EXITED");
			print_process(process_io);
			print_time_since_entry(process_io);
		}
	}
	double end = get_wtime();
	printf("\nTotal time was %lf sec\n", end-start);
	free(list);

	return 0;
}

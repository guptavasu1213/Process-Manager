#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>


#define INTERVAL 1 // Used for checking processes after INTERVAL seconds

int peakUsage;
clock_t start; // Declaring the start timer for the program

// Struct of process information
typedef struct {
	int pid; // Stores the process id
	int start; // Records the time when the process started
	int maxTime; // max time for the process to run
	int killedBit;
	// 1 - process is killed due to exceeding time; 0- opposite of 1
	int memUsage; // Stores memory usage of the process
} processInfo;

processInfo *processList; // Pointer to the information of the first process
int *totalProcesses; // Total number of processes created in the program

/*
 * PURPOSE: Performing error check for the pointer to see if it is NULL
 * PARAMETERS: Character pointer
 * RETURN: N/A
 */
void errorCheck1(char *arg)
{
	if (arg == NULL) {
		printf("\nError: Memory not allocated.\n");
		exit(1);
	}
}

/*
 * PURPOSE: Performing error check for the pointer to see if it is NULL
 * PARAMETERS: Pointer to a char pointer
 * RETURN: N/A
 */
void errorCheck2(char **arg)
{
	if (arg == NULL) {
		printf("\nError: Memory not allocated.\n");
		exit(1);
	}
}

/*
 * PURPOSE:
 *		- Function breaks the given command into words (seperated
 *		  by spaces) till the EOL.
 *		- 'args' a 2-d array storing each word is populated
 *		- Adds a NULL at the end of the 'args' array to mark
 *		  the end of it
 * PARAMETERS:
 *		- command: is a pointer to the start of a line
 *		- null_pos: stores the address of the null terminator
 *		  in the line.
 *		- countArguments: the number of arguments in a command
 *		- timer: the max time a process in a command is allowed
 *		  to run.
 * RETURN:
 *		- The char array 'args' is returned (which contains each
 *		  word in the command at each index).
 *	  The last index stores NULL.
 */
char **words(char *command, char **null_pos, int *countArguments, int **timer)
{
	// Saves the address of null terminator
	*null_pos = strchr(command, '\0');
	*countArguments = 0; // Initializing the count to zero

	char **args = malloc(sizeof(char *));

	int flagNum = 0;

	errorCheck2(args); // checks if the pointer is NULL

	char *ptr = strtok(command, " "); // extract words from strings

	while (ptr != NULL) {
		if (flagNum == 1) { // When the timer value is in ptr
			**timer = atoi(ptr);
			break;
		}
		// Reallocating for char * for more arguments
		args = realloc(args, sizeof(char *)*(*countArguments+2));
		errorCheck2(args);

		// Allocating for chars for the argument to be stored
		args[*countArguments] =
				malloc(sizeof(char) * (strlen(ptr) + 1));
		errorCheck1(args[*countArguments]);
		// When the last index in a string is a comma
		if (strcmp(&ptr[strlen(ptr)-1], ",") == 0) {
			strncpy(args[*countArguments], ptr, strlen(ptr)-1);
			args[*countArguments][strlen(ptr)-1] = '\0';
			flagNum = 1; // denotes the timer count is up next
		} else
			// Copying the argument including the null terminator
			strncpy(args[*countArguments], ptr,
				strlen(ptr)+1);

		++(*countArguments); // Incrementing

		// The  strtok() returns a pointer to the next token,
		// or NULL if there are no more tokens.
		ptr = strtok(NULL, " ");
	}
	// Adding NULL at the last index of args array
	args[*countArguments] = NULL;
	return args;
}

/* PURPOSE: Frees the memory in the 2-D array "args"
 * PARAMETERS:
 *		- args: the array to be freed
 *		- countArguments: the number of arguments in 'args'
 * RETURN: N/A
 */
void freeArgs(char **args, int countArguments)
{
	int i;
	// Freeing including the allocated space for NULL
	for (i = 0; i <= countArguments; i++)
		free(args[i]); // Freeing for each index
	free(args); // Freeing the array pointer "args"
}

/* PURPOSE:
 *	- Creates a child process
 *	- Runs the command by passing 'myargs' to execvp() in the child process
 *	- The parent does not wait for the child process to terminate
 *	- If the command ran through execvp() successfully, the
 * PARAMETERS:
 *	- myargs: 2-D array which contains an argument in each index (excluding
 *	  the last index which contains a NULL)
 *	- countCommands: index of the child process to be created in the
 *	  processList
 * RETURN: N/A
 */
void executeProcess(char **myargs, int countCommands, char *shared_memory,
	char *fileName)
{	// When there is no allocated space for the processes
	if (countCommands == 0)
		processList = malloc(sizeof(processInfo));
	else
		processList = realloc(processList,
					 sizeof(processInfo)*(countCommands+1));
	if (processList == NULL) {
		printf("\nError: Memory not allocated.\n");
		exit(1);
	}
	sprintf(shared_memory, "Successful"); // Saving to the shared memory

	int rc = fork();

	if (rc < 0) { // fork failed; exit
		fprintf(stderr, "fork failed\n");
		exit(1);
	} else if (rc == 0) { // Child process
		// To avoid displaying the contents when commands are run
		fflush(stdout);
		if (fileName == NULL)
			fclose(stdout);
		else
			freopen(fileName, "a+", stdout);
		fclose(stderr);
		execvp(myargs[0], myargs);
		// Storing the error message in the shared memory
		sprintf(shared_memory, strerror(errno));
		exit(0); // Runs when execvp() does not run successfully

	} else { // parent process
		processList[countCommands].start = time(NULL);
		processList[countCommands].pid = rc;
	}
}

/*
 * PURPOSE:
 *	- The handler gets triggered when Ctrl+Z is entered by the user
 *	- The function displays the time in the format required
 *	- Runs through all the processes in the processList to terminate
 *	  then if they are running.
 *	- Displays the time taken for program execution
 * PARAMETERS: arg
 * RETURN: N/A
 */
static void handleTermination(int arg)
{
	time_t t = time(NULL);
	const struct tm *tmp = localtime(&t);
	char time_string[100]; // Hardcoding for the date and time

	strftime(time_string, sizeof(time_string),
					"%b %d, %Y %I:%M:%S %p", tmp);
	// Format: Oct 1, 2019 12:12:00 PM
	printf("\nSIGINT signal received, %s\n", time_string);

	// Go through all the signals and kill the running ones
	int i, status, return_val;

	for (i = 0; i < *totalProcesses; i++) {
		return_val = waitpid(processList[i].pid, &status, WNOHANG);
		// If process is running, kill it
		if (return_val == 0) {
			kill(processList[i].pid, SIGKILL);
			printf("[%d] terminated (pid: %d)\n", i,
				processList[i].pid);
		}
		// If the process terminated by itself at the time
		else if (return_val == processList[i].pid) {
			printf("[%d] terminated (pid: %d)\n", i,
				processList[i].pid);
		}
	}
	// Free memory before Termination
	free(processList);
	printf("Exiting (total time: %ld seconds, peak memory usage: %dMB)\n",
				(clock()-start)/CLOCKS_PER_SEC, peakUsage);

	exit(0); // 0 means success
}

/*
 * PURPOSE:
 * PARAMETERS:
 * RETURN: 
 */
int programMemory(int pid)
{
	char procPath[100]; // ################
	int sz = 10, processMem; // ##############

	sprintf(procPath, "/proc/%d/statm", pid);
	FILE *fp = fopen(procPath, "r");

	if (fp == NULL)
		return -1;

	// Allocating memory based on the size of the file
	char *fileContents = malloc(sz*(sizeof(char)) + 1); // +1 for the '\0'

	fread(fileContents, 1, sz, fp); // Reading in the file
	fclose(fp); // Closing the file

	fileContents[strchr(fileContents, ' ')-fileContents] = '\0';
	processMem = atoi(fileContents);
	// Process in zombie state has all values
	if (processMem == 0)
		return -1;
	// 1 Page is 4 KB##############
	processMem = processMem*4*0.001;

	free(fileContents);
	return processMem;
}

/*
 * PURPOSE:
 *	- This is a handler function which is triggered when the interval
 *	  in the alarm goes off.
 *	- Normal report is generated by reporting the running processes.
 *	- If there is no running child process, the program terminates.
 * PARAMETERS: arg
 * RETURN: N/A
 */
static void fiveSecondCheck(int arg)
{
	int i, status, return_val, numberOfNotWorkingProcesses = 0;
	int tempPeakUsage = 0;

	// Displaying Day, Date and Time in the given format.
	time_t t = time(NULL);
	const struct tm *tmp = localtime(&t);
	char time_string[100]; // Hardcoding for the date and time

	strftime(time_string, sizeof(time_string),
				"%a, %b %d, %Y %I:%M:%S %p", tmp);
	// Format: Oct 1, 2019 12:12:00 PM
	printf("\nNormal Report, %s\n", time_string);

	for (i = 0; i < *totalProcesses; i++) {
		return_val = waitpid(processList[i].pid, &status, WNOHANG);

		if (return_val == 0) {
			// If timer exceeds, kill the process
			if (time(NULL)-processList[i].start >=
				processList[i].maxTime) {
				 // Kill the process
				kill(processList[i].pid, SIGKILL);
				processList[i].killedBit = 1;
				printf("[%d] Time exceeded, %ds, terminated,\
					memory usage: %dMB\n", i,
					processList[i].maxTime,
					processList[i].memUsage);
				numberOfNotWorkingProcesses++;
			} else {
				processList[i].memUsage =
					programMemory(processList[i].pid);
				tempPeakUsage = tempPeakUsage +
							processList[i].memUsage;
				printf("[%d] Running, memory usage:\
					%dMB\n", i,
					programMemory(processList[i].pid));
			}
		} else if (return_val == processList[i].pid) { // Zombie state
			// Don't print the exit message when the
			// process is killed
			if (processList[i].killedBit == 1)
				continue; // Go to the next process
			printf("[%d] Exited, memory usage: %dMB\n",
				i, processList[i].memUsage);
			numberOfNotWorkingProcesses++;
		} else
			numberOfNotWorkingProcesses++;
	}

	if (numberOfNotWorkingProcesses == *totalProcesses) {
		printf("All processes have stopped running!\n");
		exit(0);
	}
	if (tempPeakUsage > peakUsage)
		peakUsage = tempPeakUsage; // peakUsage stores the highest value

	alarm(INTERVAL); // Sets the interval again to 'INTERVAL' seconds
}

/*
 * PURPOSE:
 *	- Starts the clock of program execution
 *	- Specifies the a handler when Ctrl+C is entered by the user
 * PARAMETERS: argc, argv
 * RETURN: integer zero when the program finishes successfully
 */
int main(int argc, char *argv[])
{
	// Intializing the global variable
	start = clock(); // Used to program execution time
	int segment_id;
	FILE *fptr;

	// Handler specified for situation when Ctrl+C is hit
	signal(SIGINT, handleTermination);
	int logging = 0, c; // Write to the file by default
	char *fileName = NULL;

	while ((c = getopt(argc, argv, ":fo:")) != -1) {
		switch (c) {
		case 'f':
			logging = 1; // Write to stdout
			break;
		case 'o':
			fileName = malloc(strlen(optarg)+1);
			strncpy(fileName, optarg, strlen(optarg));
			fileName[strlen(optarg)] = '\0';

			fptr = fopen(fileName, "w");
			fclose(fptr);
			break;
		case':'://missing argument
			printf("\nError: %s: option '-%c' requires an\
				argument\n\n", argv[0], optopt);
			exit(1);

		case'?'://invalid argument
			printf("\nError: %s: option '-%c' is invalid:\
				ignored\n\n",
			argv[0], optopt);
		  exit(1);
		}
	}
	if (logging == 0) { // Write to the file
		fclose(stdout);
		freopen("macD.log", "w+", stdout);
	}
	int sz; // Stores the length of the file opened

	// Reading in the configuration file
	FILE *fp = fopen("long.conf", "r"); //###############

	if (fp == NULL) { // If there is an error in opening the file.
		printf("Error: File not found or unable to open the file.\n");
		exit(1);
	}

	// Determining the length of the file
	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Allocating memory based on the size of the file
	char *commands = malloc(sz*(sizeof(char)) + 1); // +1 for the '\0'

	errorCheck1(commands); // Error check

	fread(commands, 1, sz, fp); // Reading in the file
	fclose(fp); // Closing the file

	commands[sz] = '\0'; // Adding null terminator at the end of the string

	// strtok returns the first line in the file ending with '\n'
	char *ptr = strtok(commands, "\n"); // extract tokens from strings

	// Stores the address of where the next line in the message begins
	char **nextline = malloc(sizeof(char *));

	errorCheck2(nextline);  // Error check

	int i = 0, countCommands = 0, countArguments = 0;

	totalProcesses = &countCommands; // Initializing global variable

	char **args; // pointer to array of words (OR Array or character arrays)

	char *shared_memory;
	int *timer = malloc(sizeof(int));
	const int size = 1; // Rounds up to one page size

	segment_id = shmget(IPC_PRIVATE, size, 0600);
	shared_memory = (char *) shmat(segment_id, NULL, 0);

	// Going through each line in the 'commands'
	// Each line contains a single command to be executed
	while (ptr != NULL) {
		fflush(stdout);
		*timer = -1; // Denotes that the process didn't have a max time
		// Populating args with each argument in a single command
		args = words(ptr, nextline, &countArguments, &timer);

		// Creates a child process for the given args
		executeProcess(args, countCommands, shared_memory, fileName);
		processList[countCommands].maxTime = *timer;
		processList[countCommands].memUsage = 0;
		processList[countCommands].killedBit = 0;

		// Prints the current command
		printf("[%d]", countCommands);
		for (i = 0; i < countArguments; i++)
			printf(" %s", args[i]);

		sleep(1); // Sleeps for a second

		if (strcmp(shared_memory, "Successful") == 0) {
			printf(", started successfully (pid: %d)\n",
						processList[countCommands].pid);
		} else if (strcmp(shared_memory, "Failed") == 0)
			printf(", failed to start\n");
		else
			printf(", %s\n", shared_memory);

	    // Freeing the memory allocated by the commands
		freeArgs(args, countArguments);

		countCommands++; // Incrementing the number of commands

		// When nextline reaches the EOF
		if (*nextline == &commands[sz])
			break;

		else // The  strtok() returns a pointer to the next token
			ptr = strtok((*nextline)+1, "\n");
	}
	// Freeing the allocated memory
	free(nextline);
	free(commands);
	free(timer);
	// Handler function runs every 'INTERVAL' seconds
	signal(SIGALRM, fiveSecondCheck);
	alarm(INTERVAL);

	// Infinite loop
	while (1)
		continue;
	return 0;
}

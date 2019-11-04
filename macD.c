/*-----------------------------------------------------------------
 * Name: Vasu Gupta
 * Student ID: 3066521
 * Assignment 1
 * CMPT 360
 *----------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#define INTERVAL 5 // Used for checking processes after INTERVAL seconds

clock_t start; // Declaring the start timer for the program

// Struct of process information
typedef struct {
	int pid; // Stores the process id
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
 * RETURN:
 *		- The char array 'args' is returned (which contains each
 *		  word in the command at each index).
 *	  The last index stores NULL.
 */
char **words(char *command, char **null_pos, int *countArguments)
{
	// Saves the address of null terminator
	*null_pos = strchr(command, '\0');
	*countArguments = 0; // Initializing the count to zero

	char **args = malloc(sizeof(char *));

	errorCheck2(args); // checks if the pointer is NULL

	char *ptr = strtok(command, " "); // extract words from strings

	while (ptr != NULL) {
		// Reallocating for char * for more arguments
		args = realloc(args, sizeof(char *)*(*countArguments+2));
		errorCheck2(args);

		// Allocating for chars for the argument to be stored
		args[*countArguments] =
				malloc(sizeof(char) * (strlen(ptr) + 1));
		errorCheck1(args[*countArguments]);

		// Copying the argument including the null terminator
		strncpy(args[*countArguments], ptr, strlen(ptr)+1);

		++(*countArguments); // Incrementing

		// The  strtok() returns a pointer to the next token,
		// or NULL if there are no more tokens.
		ptr = strtok(NULL, " ");
	}
	// Adding NULL at the last index of args array
	args[*countArguments] = NULL;
	return args;
}

/*
 * PURPOSE: Frees the memory in the 2-D array "args"
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

/*
 * PURPOSE:
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
void executeProcess(char **myargs, int countCommands)
{
	if (countCommands == 0)
		processList = malloc(sizeof(processInfo));
	else
		processList = realloc(processList,
					 sizeof(processInfo)*(countCommands+1));

	if (processList == NULL) {
		printf("\nError: Memory not allocated.\n");
		exit(1);
	}

	int rc = fork();

	int status; // for passing to waitpid()

	if (rc < 0) { // fork failed; exit
		fprintf(stderr, "fork failed\n");
		exit(1);
	} else if (rc == 0) { // Child process
		// To avoid displaying the contents when commands are run
		fclose(stdout);
		// To avoid displaying an error if an argument is invalid
		fclose(stderr);
		execvp(myargs[0], myargs);  // runs /bin/ls
		exit(0); // Runs when execvp() does not run successfully

	} else { // parent process
		waitpid(rc, &status, WNOHANG);
		processList[countCommands].pid = rc;
		// processList[countCommands].num = return_val;
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
	printf("Exiting (total time: %ld seconds)\n",
				(clock()-start)/CLOCKS_PER_SEC);

	exit(0); // 0 means success
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
	// Displaying Day, Date and Time in the given format.
	time_t t = time(NULL);
	const struct tm *tmp = localtime(&t);
	char time_string[100]; // Hardcoding for the date and time

	strftime(time_string, sizeof(time_string),
				"%a, %b %d, %Y %I:%M:%S %p", tmp);
	// Format: Oct 1, 2019 12:12:00 PM
	printf("\nNormal Report, %s\n", time_string);

	int i, status, numberOfNotWorkingProcesses = 0;

	for (i = 0; i < *totalProcesses; i++) {
		// waitpid() returns 0 when the process is running
		if (waitpid(processList[i].pid, &status, WNOHANG) == 0)
			printf("[%d] Running\n", i);
		else
			numberOfNotWorkingProcesses++;
	}
	// When all the processes in the struct array are not working
	// anymore, terminate the program.
	if (numberOfNotWorkingProcesses == *totalProcesses) {
		printf("All processes have stopped running!\n");
		exit(0);
	}
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

	// Handler specified for situation when Ctrl+C is hit
	signal(SIGINT, handleTermination);

	int sz; // Stores the length of the file opened

	// Reading in the configuration file
	FILE *fp = fopen("/tmp/cmpt360/macD.conf", "r");

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

	int i = 0, status = 0;
	int countCommands = 0, countArguments = 0;

	totalProcesses = &countCommands; // Initializing global variable

	char **args; // pointer to array of words (OR Array or character arrays)

	// Going through each line in the 'commands'
	// Each line contains a single command to be executed
	while (ptr != NULL) {
		// Populating args with each argument in a single command
		args = words(ptr, nextline, &countArguments);

		// Creates a child process for the given args
		executeProcess(args, countCommands);

		// Prints the current command
		printf("[%d]", countCommands);
		for (i = 0; i < countArguments; i++)
			printf(" %s", args[i]);

		sleep(1); // Sleeps for a second

		if (waitpid(processList[countCommands].pid,
							&status, WNOHANG) == 0)
			printf(", started successfully (pid: %d)\n",
						processList[countCommands].pid);
		else
			printf(", failed to start\n");

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

	// Handler function runs every 'INTERVAL' seconds
	signal(SIGALRM, fiveSecondCheck);
	alarm(INTERVAL);

	// Infinite loop
	while (1)
		continue;
	return 0;
}

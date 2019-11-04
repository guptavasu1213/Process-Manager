#include <sys/types.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main()
{
int segment_id;
char *shared_memory;
const int size = 1; // Rounds up to one page size

  // printf("CMPPP: %d\n", strcmp("Vasu", "asu"));
  segment_id = shmget(IPC_PRIVATE, size, S_IRUSR | S_IWUSR);
  shared_memory = (char *) shmat(segment_id, NULL, 0);

  sprintf(shared_memory, "Successful"); // Storing "Successful" in shared_memory
  printf("*%s\n" , shared_memory); // printing brfore fork 

  pid_t pid;
  pid = fork();

  if (pid < 0){
   fprintf(stderr, "Fork failed");
   return 1;
  }
  else if (pid == 0){ // Child
  execlp("/bin/ls", "ls", NULL);
  sprintf(shared_memory, "Failed");   // Storing "Failed in shared_memory"
  }
  else {
  wait(NULL);
  printf("*%s\n", shared_memory); // printing after fork
  shmdt(shared_memory);
  shmctl(segment_id, IPC_RMID, NULL);
  }
  return 0;
}
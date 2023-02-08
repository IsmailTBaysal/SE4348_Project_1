/*

CS/SE 4348: Operating Systems Concepts Section 004

Ismail Timucin Baysal
Programming Project 1

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/stat.h>


const int MEMORY_SIZE = 4096;
const int BUFFER_SIZE = 256;

// Error msg
void errormsg( char *msg ) {
   perror( msg );
   exit( 1 );
}
// Number check
int isNumber(char *input) {
    for (int i = 0; input[i] != '\0'; i++)
        if (isalpha(input[i]))
            return 0;
    return 1;
}

int main(int argc, char* argv[]){

  int PROCESS_NUM;
  int size;
  char fileName[20];
  int i;
  char value[BUFFER_SIZE];
  int segment_id;
  int segment_id2;
  int segment_id3;
  int *sharedArry[MEMORY_SIZE];
  int *sharedCounter;
  int temp;
  int count = 0;
  
  // Getting inputs from user
  printf("Please enter the number of child processes: ");
  scanf("%d", &PROCESS_NUM);
  
  // Wrong input
  while (PROCESS_NUM < 0 && isdigit(PROCESS_NUM) == 0){
    printf("Process number cannot be negative or a character! \n");
    printf("Please enter positive number:");
    scanf("%d", &PROCESS_NUM);
  }

  printf("Please enter the array size: ");
  scanf("%d", &size);
  
  // Wrong input  
  while (size < 0 && isdigit(size) == 0){
    printf("Array size cannot be negative or a character! \n");
    printf("Please enter positive number:");
    scanf("%d", &size);
  }

  printf("Please enter file name: ");
  scanf("%s", fileName); 
  
  // Calculates portions for processes
  int portion = size/(PROCESS_NUM-1);
  int bonus = size - portion * (PROCESS_NUM-1);
  
  //Creating Shared Memories
  segment_id = shmget( IPC_PRIVATE, MEMORY_SIZE, S_IRUSR | S_IWUSR );
  if ( segment_id < 0 ) errormsg( "ERROR in creating a shared memory segment\n" );
  segment_id2 = shmget( IPC_PRIVATE, MEMORY_SIZE, S_IRUSR | S_IWUSR );
  if ( segment_id2 < 0 ) errormsg( "ERROR in creating a shared memory-2 segment\n" );
  segment_id3 = shmget( IPC_PRIVATE, MEMORY_SIZE, S_IRUSR | S_IWUSR );
  if ( segment_id3 < 0 ) errormsg( "ERROR in creating a shared memory-3 segment\n" );
  
  // Attach an area of local memory to the shared memory segment
  char *shared_memory = ( char * ) shmat( segment_id, NULL, 0 );
  sharedArry[MEMORY_SIZE] = ( int * ) shmat( segment_id2, NULL, 0 );
  sharedCounter = ( int * ) shmat( segment_id3, NULL, 0 );
  
  
  // Counter is initially 0
  (*sharedCounter) = 0;
  
  //Reads From File and Stores in an Array
	FILE *fp;

	if((fp = fopen(fileName,"r")) ==  NULL){
		printf("An error occured while reading the file!");
		exit(1);
	}
		
  while(fscanf(fp,"%d",&temp)==1){
    count++;
  }
  
  rewind(fp);
  
  for(int i=0; i<count; i++){
    fscanf(fp, "%d", &sharedArry[i]);
  }

  // Error handling
  if(count == 0){  //do not create array of size 0
    fclose(fp);
    printf("File is empty!\n");
    return 1;
  }else if(count < size){
    printf("File contains less than your input!\n");
    fclose(fp);
    return 2;
  }else if(count > size){
    printf("File contains more than your input!\n");
    fclose(fp);
    return 3;
  }
      
  fclose(fp);
  
  
  int pids[PROCESS_NUM];
  int pipes[PROCESS_NUM + 1][2];


  // Creating Pipes
  for (i = 0 ; i < PROCESS_NUM + 1; i++){
    if(pipe(pipes[i]) == -1){
      printf("Error with creating pipe\n");
      return 1;
    }
  }
  
  // Creating Processes
  for (i = 0 ; i < PROCESS_NUM; i++){
    pids[i] = fork();
    if(pids[i] == -1){
      printf("Error with creating process\n");
      return 2;
    }
    if(pids[i] == 0){
      int j;
      
      // Child process
      
      // Attach an area of local memory to the shared memory segment
      char *shared_memory = ( char * ) shmat( segment_id, NULL, 0 );
      sharedArry[MEMORY_SIZE] = ( int * ) shmat( segment_id2, NULL, 0 );
      int *sharedCounter = ( int * ) shmat( segment_id3, NULL, 0 );

      for (j = 0 ; j < PROCESS_NUM + 1; j++){
        if(i != j){
          close(pipes[j][0]);
        }
        if(i + 1 != j){
          close(pipes[j][1]);
        }
      }

      do{
        
        // Initial variables for child process
        int x;
        int n;
        int lastNumber = 0;
        int numberInArray;
        
        // Reads index from pipe
        if(read(pipes[i][0], &x ,sizeof(int)) == -1){
          printf("Error at reading\n");
          return 3;
        }
        
        // Gets user input through shared memory
        n = atoi(shared_memory);
        
        // Bonus(last) part will be calculated here 
        if((i+1) == PROCESS_NUM){ 
          lastNumber = x + bonus; //End point for child
          for (x; x < lastNumber; x++){ //Starts from its portion until the end point 
          numberInArray = sharedArry[x];
            if (numberInArray == n){
              (*sharedCounter)++; 
            }
          }
        }else{ // Each child will calculate their portion here
          
          lastNumber = x + portion;         //End point for child
          for (x; x < lastNumber; x++){     //Starts from its portion until the end point
            numberInArray = sharedArry[x];  
            if (numberInArray == n){        
              (*sharedCounter)++;            
            }
          }
        }
        
        // Child last index to other processes
        if(write(pipes[i+1][1] , &x ,sizeof(int)) == -1 ){
          printf("Error at writing \n");
          return 4;      
        }
        
      }while(isNumber(shared_memory) && (strncmp(shared_memory,"quit",4)));  // Continues until user enters 'quit'
      
      // Break the link between the local memory and the shared memory segment
      shmdt( shared_memory );
      shmdt( sharedCounter );
      shmdt( sharedArry );
      // Close the pipes
      close(pipes[i][0]);
      close(pipes[i + 1][1]);
      
      return 0;
    } 
  }
  
  // Main process
  
  do{
    // Initial variables for parent process 
    int number;
    int x=0;
    int lastResult;
    
    // Getting value from the user 
    printf("Please enter a number to calculate occurence in file or type quit to exit:" );
    scanf("%s",&value);
    
    // Send it to the shared memory
    sprintf( shared_memory, value );

    if(isNumber(shared_memory) && (strncmp(shared_memory,"quit",4))){    //valid input

      // Main writes initial index end let child do calculations
      if(write(pipes[0][1] , &x ,sizeof(int)) == -1 ){
        printf("Error at writing \n");
      return 4;      
      }
      // Main reads last index
      if(read(pipes[PROCESS_NUM][0], &x ,sizeof(int)) == -1){
        printf("Error at reading\n");
      return 3;
      }
      // Print result to the user
      printf("The final result is %d\n", (*sharedCounter));
      lastResult = (*sharedCounter);
      (*sharedCounter) = 0; // Reset Counter
      
    
    
    
  }else if(!strncmp(value,"quit",4)){ // User wants to terminate
    printf("\nThe last result is %d\n", lastResult);
    printf("Goodbye!\n\n" );
    //Close pipes
     
    close(pipes[0][1]);
    close(pipes[PROCESS_NUM][0]);
    
    // break the link between the local memory and the shared memory segment
    shmdt( shared_memory );
    shmdt( sharedCounter );
    shmdt( sharedArry );
    // Waiting for child processes
    for (int i = 0 ; i < PROCESS_NUM; i++){
      wait(NULL);
  }
  }else{ //invalid input
  printf("Invalid input!\n" );
  }
  }while(strncmp(shared_memory,"quit",4)); //Program runs until user enters 'quit'


  for (i = 0 ; i < PROCESS_NUM; i++){
    wait(NULL);
  }
  
  // mark the shared memory segment for destruction
  shmctl( segment_id, IPC_RMID, NULL );
  shmctl( segment_id2, IPC_RMID, NULL );
  shmctl( segment_id3, IPC_RMID, NULL );
  return 0;
}
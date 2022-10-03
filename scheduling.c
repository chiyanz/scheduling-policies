// must compile with: gcc  -std=c99 -Wall -o scheduling scheduling.c
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
////////////////////////////////
struct Queue {
  int front, rear, size;
  unsigned capacity;
  int *array;
};

// function to create a queue
// of given capacity.
// It initializes size of queue as 0
struct Queue *createQueue(unsigned capacity) {
  struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
  queue->capacity = capacity;
  queue->front = queue->size = 0;

  // This is important, see the enqueue
  queue->rear = capacity - 1;
  queue->array = (int *)malloc(queue->capacity * sizeof(int));
  return queue;
}

// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue *queue) { return (queue->size == queue->capacity); }

// Queue is empty when size is 0
int isEmpty(struct Queue *queue) { return (queue->size == 0); }

// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue *queue, int item) {
  if (isFull(queue))
    return;
  queue->rear = (queue->rear + 1) % queue->capacity;
  queue->array[queue->rear] = item;
  queue->size = queue->size + 1;
}

// Function to remove an item from queue.
// It changes front and size
int dequeue(struct Queue *queue) {
  if (isEmpty(queue))
    return INT_MIN;
  int item = queue->array[queue->front];
  queue->front = (queue->front + 1) % queue->capacity;
  queue->size = queue->size - 1;
  return item;
}

// Function to get front of queue
int front(struct Queue *queue) {
  if (isEmpty(queue))
    return INT_MIN;
  return queue->array[queue->front];
}

// Function to get rear of queue
int rear(struct Queue *queue) {
  if (isEmpty(queue))
    return INT_MIN;
  return queue->array[queue->rear];
}
//////////////////////////////////
void fcfs(FILE *fp, char *filename) { // first come first serve implementation
  int c; // number of processes to schedule
  fscanf(fp, "%d\n", &c);
  int processes[c][6];
  int finishTime[c];
  int i;
  int cpuTime = 0;
  
  for (i = 0; i < c; i = i + 1) {
    fscanf(fp, "%d %d %d %d\n", &processes[i][0], &processes[i][1],
           &processes[i][2], &processes[i][3]);
    processes[i][4] = 0; // inital status undefined
    // 0 = not loaded 1 = running, 2 = ready, 3 = blocked 4 = finihsed 
    processes[i][5] = 0; // stores the remaining CPU time 
  }
  int finished = 0;
  FILE *output = fopen(filename, "w");
  struct Queue *queue = createQueue(100);
  int cycle = 0;
  while (finished != 1) {
    for (i = 0; i < c; i = i + 1) {
      if (processes[i][3] == cycle) {     // first push any processes that just arrived
        enqueue(queue, i);
        processes[i][4] = 2; // new processes are automatically ready 
        //initialize how many cycles it'll take before it gets blocked
        processes[i][5] = ceil(processes[i][1] * 0.5);
        processes[i][1] = processes[i][1] - processes[i][5];
      }
    }

    int pIndex = front(queue);
    // change first process's status
    if (processes[pIndex][4] == 0 || processes[pIndex][4] == 2 || processes[pIndex][4] == 1) {
      processes[pIndex][4] = 1; //if first process wasn't running but is ready to run, run it
      cpuTime = cpuTime + 1; // a process is running this cycle so increase CPU utilization
      processes[pIndex][5] = processes[pIndex][5] - 1;
    }

    // print out the status before running
    fprintf(output, "%d ", cycle);
    for(i = 0; i < c; i = i + 1) {
      // iterate through the array of processes and print their status if they have arrived 
      switch(processes[i][4]) {
        case 1: 
          fprintf(output, "%d:running ", i);
          break;
        case 2: 
          fprintf(output, "%d:ready ", i);
          break;
        case 3: 
          fprintf(output, "%d:blocked ", i);
      }
    }
    fprintf(output, "\n");
    fflush(output);
    
    // update process statuses 
    // decrease remaining cpu time and blocking time 
    for(i = 0; i < c; i = i + 1) {
      if(processes[i][4] == 1) {
        if(processes[i][5] == 0) { //finished half or all cpu-time
          if(processes[i][1] == 0) { //no remaining cpu-time: process is finished 
            processes[i][4] = 4;
            finishTime[i] = cycle + 1;
            dequeue(queue);
          }
          else { 
            // process not finished, but used up half of its cpu time, change to blocked
            processes[i][5] = processes[i][1];
            processes[i][1] = 0; //push remaining cpu time
            if(processes[i][2]) { // if the process requires I/O, block it
              processes[i][4] = 3;
              dequeue(queue);
            }
          }
        }
        else {
          // still has remaining running time, remains as top of the queue
          //nothing to be done to this process
        }
      }
      // if it was blocked, decrease remaining block duration and update status to ready if needed
      else {
        if(processes[i][4] == 3) {
        processes[i][2] = processes[i][2] - 1;
          if(processes[i][2] == 0) { // I/O finished, no longer blocked 
            processes[i][4] = 2; //change state to ready
            enqueue(queue, i);
          }
        }
      }
    }

    int check = 1;
    //check if all processes finished 
    for(i = 0; i < c; i = i + 1) {
      if(!(processes[i][4] == 4)) {
        //not all processes have finished yet
        check = 0;
        break;
      }
    }
    if(check) {
      // no processes if un-finished so everything is finished 
      finished = 1;
    }
    else {
      // not finished yet, one more cycle
      cycle = cycle + 1;
    }
  }
  //print conclusion
  fprintf(output, "\n");
  fprintf(output, "Finishing time: %d\n", cycle);
  fprintf(output, "CPU utilization: %.2f\n", (float)cpuTime / (cycle + 1)); // to be calculated 
  for(i = 0; i < c; i = i + 1) {
    fprintf(output, "Turnaround process %d: %d\n", i, finishTime[i] - processes[i][3]);
  }
  fclose(output);
}

//////////////////////////////////

void rrq2(FILE *fp, char *filename) { // round robin implementation
  //basically reuses ffcs implementation but adds a quantum tracker
  int c; // number of processes to schedule
  fscanf(fp, "%d\n", &c);
  int processes[c][7];
  int finishTime[c];
  int i;
  int cpuTime = 0;
  for (i = 0; i < c; i = i + 1) {
    fscanf(fp, "%d %d %d %d\n", &processes[i][0], &processes[i][1],
           &processes[i][2], &processes[i][3]);
    processes[i][4] = 0; // inital status undefined
    // 0 = not loaded 1 = running, 2 = ready, 3 = blocked 4 = finihsed 
    processes[i][5] = 0; // stores the remaining CPU time 
    processes[i][6] = 0; // stores cycles spent continuously running
  }

  int finished = 0;
  FILE *output = fopen(filename, "w");
  struct Queue *queue = createQueue(100);
  int cycle = 0;
  while (finished != 1) {
    for (i = 0; i < c; i = i + 1) {
      if (processes[i][3] == cycle) {     // first push any processes that just arrived
        enqueue(queue, i);
        processes[i][4] = 2; // new processes are automatically ready 
        //initialize how many cycles it'll take before it gets blocked
        processes[i][5] = ceil(processes[i][1] * 0.5);
        processes[i][1] = processes[i][1] - processes[i][5];
      }
    }

    int pIndex = front(queue);
    // change first process's status
    if (processes[pIndex][4] == 0 || processes[pIndex][4] == 2 || processes[pIndex][4] == 1) {
      processes[pIndex][4] = 1; //if first process wasn't running but is ready to run, run it
      cpuTime = cpuTime + 1; // a process is running this cycle so increase CPU utilization
      processes[pIndex][5] = processes[pIndex][5] - 1;
      processes[pIndex][6] = processes[pIndex][6] + 1; // increase its quantum by 1
    }

    // print out the status before running
    fprintf(output, "%d ", cycle);
    for(i = 0; i < c; i = i + 1) {
      // iterate through the array of processes and print their status if they have arrived 
      switch(processes[i][4]) {
        case 1: 
          fprintf(output, "%d:running ", i);
          break;
        case 2: 
          fprintf(output, "%d:ready ", i);
          break;
        case 3: 
          fprintf(output, "%d:blocked ", i);
      }
    }
    fprintf(output, "\n");
    fflush(output);
    
    // update process statuses 
    // decrease remaining cpu time and blocking time 
    for(i = 0; i < c; i = i + 1) {
      if(processes[i][4] == 1) {
        if(processes[i][5] == 0) { //finished half or all cpu-time
          if(processes[i][1] == 0) { //no remaining cpu-time: process is finished 
            processes[i][4] = 4;
            finishTime[i] = cycle + 1;
            dequeue(queue);
          }
          else { 
            // process not finished, but used up half of its cpu time, change to blocked
            processes[i][5] = processes[i][1];
            processes[i][1] = 0;
            processes[i][4] = 3;
            dequeue(queue);
          }
        }
        else {
          // still has remaining running time
          if(processes[i][6] == 2) { //was ran for 2 cycles, change status to ready and move it to the end of queue
            processes[i][4] = 2;
            processes[i][6] = 0; // reset quantum
            int t = dequeue(queue);
            enqueue(queue, t);
          }
        }
      }
      else {
        if(processes[i][4] == 3) {
          processes[i][2] = processes[i][2] - 1;
          if(processes[i][2] == 0) { // I/O finished, no longer blocked 
            processes[i][4] = 2; //change state to ready
            enqueue(queue, i);
          }
        }
      }
    }
    int check = 1;
    //check if all processes finished 
    for(i = 0; i < c; i = i + 1) {
      if(!(processes[i][4] == 4)) {
        //not all processes have finished yet
        check = 0;
        break;
      }
    }
    if(check) {
      // no processes if un-finished so everything is finished 
      finished = 1;
    }
    else {
      // not finished yet, one more cycle
      cycle = cycle + 1;
    }
  }
  //print conclusion
  fprintf(output, "\n");
  fprintf(output, "Finishing time: %d\n", cycle);
  fprintf(output, "CPU utilization: %.2f\n", (float)cpuTime / (cycle + 1)); // to be calculated 
  for(i = 0; i < c; i = i + 1) {
    fprintf(output, "Turnaround process %d: %d\n", i, finishTime[i] - processes[i][3]);
  }
  fclose(output);
}

void srtf(FILE *fp, char *filename) { // shortest remaining time first implementation
  int c; // number of processes to schedule
  fscanf(fp, "%d\n", &c);
  int processes[c][6];
  int finishTime[c];
  int i;
  int cpuTime = 0;
  
  for (i = 0; i < c; i = i + 1) {
    fscanf(fp, "%d %d %d %d\n", &processes[i][0], &processes[i][1],
           &processes[i][2], &processes[i][3]);
    processes[i][4] = 0; // inital status undefined
    // 0 = not loaded 1 = running, 2 = ready, 3 = blocked 4 = finihsed 
    processes[i][5] = 0; // stores the remaining CPU time 
  }
  int finished = 0;
  FILE *output = fopen(filename, "w");
  int cycle = 0;
  while (finished != 1) {
    for (i = 0; i < c; i = i + 1) {
      if (processes[i][3] == cycle) {     // first push any processes that just arrived
        processes[i][4] = 2; // new processes are automatically ready 
        //initialize how many cycles it'll take before it gets blocked
        processes[i][5] = ceil(processes[i][1] * 0.5);
        processes[i][1] = processes[i][1] - processes[i][5];
      }
    }

    //find the one with shortest remaining run-time among the ready processes and run it
    int shortest = INT_MAX;
    int shortestIndex = -1;
    for (i = 0; i < c; i = i + 1) {
      if(processes[i][4] == 2) {
        if((processes[i][1] + processes[i][5]) < shortest) {
          shortest = processes[i][1] + processes[i][5];
          shortestIndex = i;
        }
      }
    }

    if(shortestIndex > -1) { //if a process can be run
      processes[shortestIndex][4] = 1; 
    }

    // print out the status before running
    fprintf(output, "%d ", cycle);
    for(i = 0; i < c; i = i + 1) {
      // iterate through the array of processes and print their status if they have arrived 
      switch(processes[i][4]) {
        case 1: 
          fprintf(output, "%d:running ", i);
          break;
        case 2: 
          fprintf(output, "%d:ready ", i);
          break;
        case 3: 
          fprintf(output, "%d:blocked ", i);
      }
    }
    fprintf(output, "\n");
    fflush(output);
    
    // update process statuses 
    // decrease remaining cpu time and blocking time 
    for(i = 0; i < c; i = i + 1) {
      if(processes[i][4] == 1) {
        processes[i][5] = processes[i][5] - 1;
        cpuTime = cpuTime + 1;
        if(processes[i][5] == 0) { //finished half or all cpu-time
          if(processes[i][1] == 0) { //no remaining cpu-time: process is finished 
            processes[i][4] = 4;
            finishTime[i] = cycle + 1;
          }
          else { 
            // process not finished, but used up half of its cpu time, change to blocked
            processes[i][5] = processes[i][1];
            processes[i][1] = 0; //push remaining cpu time
            processes[i][4] = 3;
          }
        }
        else {
          processes[i][4] = 2; //reset status to ready since i may not be ran again
        }
      }
      // if it was blocked, decrease remaining block duration and update status to ready if needed
      else {
        if(processes[i][4] == 3) {
          processes[i][2] = processes[i][2] - 1;
          if(processes[i][2] == 0) { // I/O finished, no longer blocked 
            processes[i][4] = 2; //change state to ready
          }
        }
      }
    }

    int check = 1;
    //check if all processes finished 
    for(i = 0; i < c; i = i + 1) {
      if(!(processes[i][4] == 4)) {
        //not all processes have finished yet
        check = 0;
        break;
      }
    }
    if(check) {
      // no processes if un-finished so everything is finished 
      finished = 1;
    }
    else {
      // not finished yet, one more cycle
      cycle = cycle + 1;
    }
  }
  //print conclusion
  fprintf(output, "\n");
  fprintf(output, "Finishing time: %d\n", cycle);
  fprintf(output, "CPU utilization: %.2f\n", (float)cpuTime / (cycle + 1)); // to be calculated 
  for(i = 0; i < c; i = i + 1) {
    fprintf(output, "Turnaround process %d: %d\n", i, finishTime[i] - processes[i][3]);
  }
  fclose(output);
}

int main(int argc, char *argv[]) {

  int scheduling;
  FILE *fp;                // for creating the output file
  char filename[100] = ""; // the file name

  // Check that the command line is correct
  if (argc != 3) {
    printf("usage:  ./scheduling alg input\n");
    printf("alg: the scheduling algorithm: 0, 1, or 2\n");
    printf("input: the processes inut file\n");
    exit(1);
  }

  scheduling = (int)atoi(argv[1]); // the scheduling algorithm

  // Check that the file specified by the user exists and open it
  if (!(fp = fopen(argv[2], "r"))) {
    printf("Cannot open file %s\n", argv[2]);
    exit(1);
  }

  // form the output file name
  sprintf(filename, "%d-%s", scheduling, argv[2]);

  // run the specified scheduling algorithm
  if(scheduling == 0 ) {
    fcfs(fp, filename);
  }
  else if(scheduling == 1) {
    rrq2(fp, filename);
  }
  else if(scheduling == 2) {
    srtf(fp, filename);
  }

  // close the processes file
  fclose(fp);

  return 0;
}


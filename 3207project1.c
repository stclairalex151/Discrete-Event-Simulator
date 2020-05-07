/*
 * Program name: 3207 Project 1 (Discrete event simulator
 * Programmer:   Alex St.Clair
 * Program Desc: Handles an event log that tracks usage of a CPU and two disks
 * https://templeu.instructure.com/courses/63780/files/folder/Lab%20Slides/Fall%202019%20(Chenglong)?preview=5767225 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHARS 20        //max chars in a string from input file
#define MAX_QUEUE_SIZE 500   //max number of elements in line for each component

int job_counter = -1; //tracks the number of the last job
int cpu_status = 0;   //1 if cpu is being used, 0 if idle
int d1_status = 0;    //1 if disk 1 is being used, 0 if idle
int d2_status = 0;    //1 if disk 2 is being used, 0 if idle
int jobs_done = 0;    //total number of jobs completed

//struct created for configuration variables
typedef struct {    
    int seed; //int for random generator
    int start_time; //start time for event queue
    int end_time; //end time for event queue
    int arrive_min; //minimum time between job starts
    int arrive_max; //maximum time between job starts
    int quit_prob; //probability index of a job needing disk I/O
    int cpu_min; //min time spent at the cpu
    int cpu_max; //max time spent at the cpu
    int d1_min; //min time spent at disk 1
    int d1_max; //max time spent at disk 1
    int d2_min; //min time spent at disk 2
    int d2_max; //max time spent at disk 2
} Config;

//event
typedef struct {
    int time; //timestamp for the event
    int jobno; //each event contains a job number to count events
    int type; //index for the event type, to be decoded for instructions
} Event;

//queue of EVENTS
typedef struct {
    Event *ptr; //the pointer to the queue
    int next;   //the location of the next up in the queue
    int last;   //the location of the last entry in the queue
    int size;   //the number of elements in the queue
} Queue;

//event queue / priority queue
typedef struct {
    int capacity;           //maximum number of contents of queue
    int size;               //tracks the current size/location of next elem.
    Event *eventListPtr;    //pointer to priority queue
} EventQueue;

//FUNCTION
int get_val(char *buffer);
Config read_config(Config);
Queue *q_create();
int is_empty(Queue q1);
void q_push(Queue *q1, Event *e1);
Event *q_pop(Queue *q1);
Event* e_create(int time, int type, int job_no);
EventQueue *pq_create(int capacity);
void pq_push(EventQueue *pq, Event e);
void reheap_push(EventQueue *pq, int index);
void reheap_pop(EventQueue *pq, int index);
Event pq_pop(EventQueue *pq);
void pq_free(EventQueue *pq);
int pq_isEmpty(EventQueue *pq);
int pq_isFull(EventQueue *pq);
void pq_print(EventQueue *pq);
char *type_to_string(Event *e, char *buffer); 
void print_config(FILE *ptr, Config values);
int random_gen(int max, int min);
int prob_select(int percent);
void process_cpu(Event *job, EventQueue *pq, Queue *cpu, Queue *d1, Queue *d2, Config values, FILE *ptr);
void process_disk(Event *job, EventQueue *pq, Queue *d1, Queue *d2, FILE *ptr);


//MAIN
int main() {
    FILE *log = fopen("logs.txt","w");  //creates log.txt for writing
    int time = 0;   //tracks the current time (dependent on events)
    Event *cur_job = (Event*)malloc(sizeof(Event));
    Config values;
    values = read_config(values); //read in the values from the input file
    
    int cpu_len = 0;
    int d1_len = 0;
    int d2_len = 0;
    int pq_len = 0;
    int count = 0;
    int maxpq = 0;
    int maxcpu = 0;
    int maxd1 = 0;
    int maxd2 = 0;
    
    srand(values.seed); //seed the random generator    
    print_config(log, values);  //prints config vals to log.txt

    EventQueue pq = *pq_create(10); //the event priority queue
    Queue cpu = *q_create();
    Queue d1 = *q_create();
    Queue d2 = *q_create();

    Event *j1 = e_create(values.start_time, 1, 1);
    pq_push(&pq, *j1);  
    Event *end = e_create(values.end_time , 11, 0);
    pq_push(&pq, *end);
    
    while(!pq_isEmpty(&pq) && (time < values.end_time)){
        count ++;
        *cur_job = pq_pop(&pq); //pulls job out of pq
        time = cur_job->time;   //sets global time to time of event
        
        pq_len += pq.size;
        cpu_len += cpu.size;
        d1_len += d1.size;
        d2_len += d2.size;
        
        if(pq.size > maxpq)
            maxpq = pq.size;
        if(maxcpu < cpu.size)
            maxcpu = cpu.size;
        if(maxd1 < d1.size)
            maxd1 = d1.size;
        if(maxd2 < d2.size)
            maxd2 = d2.size;
        
        
        switch (cur_job->type){
            case 1: //began the simulation
                process_cpu(cur_job, &pq, &cpu, &d1, &d2, values, log);
                break;
            case 3: //d1 arrival
                process_disk(cur_job, &pq, &d1, &d2, log);
                break;
            case 5: //d2 arrival
                process_disk(cur_job, &pq, &d1, &d2, log);
                break;
            case 7: //CPU finish
                process_cpu(cur_job, &pq, &cpu, &d1, &d2, values, log);
                break;
            case 8: //d1 finish
                process_disk(cur_job, &pq, &d1, &d2, log);
                break;
            case 9: //d2 finish
                process_disk(cur_job, &pq, &d1, &d2, log);
                break;
            case 11:break;//end of simulation
        }
        
        //proceeding CPU
        if((!(is_empty(cpu))) && (cpu_status == 0)){
            Event *process;
            process = q_pop(&cpu);//get the task from FIFO (lost after this)
            process->type = 2;//job began at CPU
      
            char buffer[30];//job is now being serviced by the cpu
            fprintf(log, "\nTime %d Job %d%s" , process->time, process->jobno , type_to_string(process, buffer));
            
            Event *job_fin = e_create(time + random_gen(values.cpu_min,values.cpu_max) , 7, process->jobno);//set up finish time for the job
            pq_push(&pq, *job_fin);//corresponding finish event
            cpu_status = 1;
        }
        
        //proceeding Disk 1
        if((!(is_empty(d1))) && (d1_status == 0)){
            Event *process;
            process = q_pop(&d1);//get the task from FIFO (does nothing with this)
            
            process->type = 4;//began d1
            char buffer[30];//device is now being serviced by the first disk
            fprintf(log, "\nTime %d Job %d%s" , process->time, process->jobno , type_to_string(process, buffer));
            
            Event *job_fin = e_create(time + random_gen(values.d1_min,values.d1_max) , 10, process->jobno);//set up finish time for the job
            pq_push(&pq, *job_fin);//corresponding finish event
            d1_status = 1;
        }
        
        //proceeding Disk 2
        if((!(is_empty(d2))) && (d2_status == 0)){
            Event *process;
            process = q_pop(&d2);//get the task from FIFO (does nothing with this)
            
            char buffer[30];//device is now being serviced by disk 2
            fprintf(log, "\nTime %d Job %d%s" , process->time, process->jobno , type_to_string(process, buffer));
            
            Event *job_fin = e_create(time + random_gen(values.d2_min,values.d2_max) , 10, process->jobno);//set up finish time for the job
            pq_push(&pq, *job_fin);//corresponding finish event
            d2_status = 1;
        } 
    }
    cpu_len /= count;
    d1_len /= count;
    d2_len /= count;
    pq_len /= count;
    
    Event finalJob = pq_pop(&pq);
    printf("\n---Results---");
    printf("\nPQ average length: %d" , pq_len);
    printf("\ncpu average length: %d" , cpu_len);
    printf("\nd1 average length: %d" , d1_len);
    printf("\nd2 average length: %d" , d2_len);
    printf("\nMax pq: %d" , maxpq);
    printf("\nMax cpu: %d" , maxcpu);
    printf("\nMax d1: %d" , maxd1);
    printf("\nMax d2: %d" , maxd2);
    printf("\nServiced %d jobs in %d units\n~%.4f jobs per unit time" 
            , finalJob.jobno, time, ((float)finalJob.jobno)/time);
    printf("\n\n");

    return (0);
}

/**
 * Trims the string of text from the input text file to
 * obtain the integer value
 * @param buffer a storage location for the string
 * @return the integer value
 */
int get_val(char *buffer) {
    int i;
    int num; //gets the integer value of the number found
    int value = 0; //the running-total value to be returned


    for (i = 3; i < strlen(buffer); i++) {  //starts at 3 to avoid the 1 in d1_max
        if (buffer[i] >= '0' && buffer[i] <= '9') {
            num = buffer[i] - '0';
            value = value * 10 + num;
        }
    }
    return value;
}

/**
 * Constructor for a queue, sets first and last to 0
 * @return the created queue
 */
Queue *q_create() {
    Queue *q1;
    q1 = (Queue*)malloc(sizeof(Queue));
    
    q1->ptr = (Event*) malloc(MAX_QUEUE_SIZE * sizeof(Event));
    q1->last = 0;
    q1->next = 0;
    q1->size = 0;
    
    return q1;
}

/**
 * Adds an element to the queue unless it is full
 * @param q1 the queue being added to
 * @param e1 the event being added
 */
void q_push(Queue *q1, Event *e1){
    if(q1->size == MAX_QUEUE_SIZE){
        printf("Queue is full, cannot push element onto queue. ");
        exit(2);
    }
    else{        
        q1->ptr[(q1->last) % MAX_QUEUE_SIZE] = *e1;
        q1->last++;
        q1->size++;
    }
}

/**
 * Removes the first element from the queue
 * @param q1 the queue being modified
 * @return the event that was popped off
 */
Event *q_pop(Queue *q1){
    if(q1->size == 0){
        printf("Queue is empty, nothing to pull from queue. ");
        exit(3);
    }
    else{
        Event *e = &(q1->ptr[q1->next]);
        q1->next++;
        q1->size--;
        return e;
    }
    
}

/**
 * Constructor for the event
 * @param time the time that the event occurs
 * @param type the typecode for the event
 * @return the newly created event
 */
Event* e_create(int time, int type, int job_no){
    Event *e;   //pointer to new event being created 
    e = (Event *) malloc(sizeof(Event));
    
    e->jobno = job_no;
    e->time = time;
    e->type = type;
    
    return e;
}

/**
 * Returns 1 if the queue is empty, 0 otherwise
 * @param q1 the queue in question
 * @return boolean value
 */
int is_empty(Queue q1) {
    return (q1.size == 0);
}

/**
 * Reads the data from input.txt and sets values
 * @param c1 a configuration object to be added to
 * @return the config struct with values filled
 */
Config read_config(Config c1) {
    char buffer[20]; //buffer for each input string
    FILE *fp = fopen("input.txt", "r"); //open the config file

    if (fp == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    //grab a line from the input file, and extract the integer
    fgets(buffer, MAX_CHARS, fp);
    c1.seed = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.start_time = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.end_time = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.arrive_min = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.arrive_max = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.quit_prob = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.cpu_min = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.cpu_max = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.d1_min = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.d1_max = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.d2_min = get_val(buffer);
    fgets(buffer, MAX_CHARS, fp);
    c1.d2_max = get_val(buffer);

    fclose(fp);
    return c1;
}

/**
 * Constructor for a priority queue
 * @param capacity the maximum number of contents in the queue
 * @return the newly created priority queue
 */
EventQueue *pq_create(int capacity){
    EventQueue *pq = (EventQueue *)malloc(sizeof(EventQueue));
    
    if(pq == NULL){
        printf("Memory Error!");
        exit(4);
    }
    
    pq->capacity = capacity;
    pq->size = 0;
    pq->eventListPtr = (Event *)malloc(sizeof(Event));
    
    if(pq->eventListPtr == NULL){
        printf("Memory Error!");
        exit(4);
    }
    
    return pq;
}

/**
 * Adds an element in order to priority queue
 * @param pq the priority queue
 * @param e the event being added
 */
void pq_push(EventQueue *pq, Event e){
    if(pq_isFull(pq)){
        printf("\n***PRIORITY QUEUE IS FULL***\ncreate a bigger pq capacity\n");
        exit(4);
    }
    pq->eventListPtr[pq->size] = e;
    reheap_push(pq, pq->size);
    (pq->size)++;    
}

/**
 * reheap function for when an object gets pushed to the pq
 * @param pq priority queue
 * @param index the index of the last object
 */
void reheap_push(EventQueue *pq, int index){
    Event temp;   //temp storage
    int parent_node = (index-1)/2; //index of parent

    if(pq->eventListPtr[parent_node].time > pq->eventListPtr[index].time){
        //switch the elements and recursively call
        temp = pq->eventListPtr[parent_node];
        pq->eventListPtr[parent_node] = pq->eventListPtr[index];
        pq->eventListPtr[index] = temp;
        reheap_push(pq,parent_node);
    }
}

/**
 * Reheap function for when an object gets popped off the pq
 * @param pq the priority queue
 * @param parent the location of the parent object
 */
void reheap_pop(EventQueue *pq, int parent){
    int left = parent*2+1;      //location of left node
    int right = parent*2+2;     //location of right node
    int min;                    //the min of the two children
    Event temp;                 //temp storage location

    //make sure that left and right are in bounds
    if(left >= pq->size || left <0)
        left = -1;
    if(right >= pq->size || right <0)
        right = -1;

    //see if left is smaller
    if(left != -1 && pq->eventListPtr[left].time < pq->eventListPtr[parent].time)
        min = left;
    else
        min = parent;
    
    if(right != -1 && pq->eventListPtr[right].time < pq->eventListPtr[min].time)
        min = right;

    //if min is a child (needs swapped)
    if(min != parent){
        temp = pq->eventListPtr[min];
        pq->eventListPtr[min] = pq->eventListPtr[parent];
        pq->eventListPtr[parent] = temp;

        // recursive  call
        reheap_pop(pq, min);
    }
}

/**
 * Removes an event from the priority queue
 * @param pq priority queue
 * @return the event popped off
 */
Event pq_pop(EventQueue *pq){
    Event e;
    if(pq_isEmpty(pq)){
        printf("\nQueue is Empty\n");
        exit(5);
    }
    // replace first node by last and delete last
    e = pq->eventListPtr[0];
    pq->eventListPtr[0] = pq->eventListPtr[pq->size-1];
    pq->size--;
    
    reheap_pop(pq, 0);
    
    return e;
}

/**
 * Frees the pq CAUSES ERROR 
 * @param pq priority queue
 */
void pq_free(EventQueue *pq){
    free(pq);
}

/**
 * Returns 1 if the pq is empty
 * @param pq priority queue
 * @return boolean value
 */
int pq_isEmpty(EventQueue *pq){
    return(pq->size == 0);
}

/**
 * returns 1 if the pq is full
 * @param pq the priority queue
 * @return boolean value
 */
int pq_isFull(EventQueue *pq){
    return (pq->size == pq->capacity);
}

/**
 * Prints the content of the priority queue.
 * NOTE: The queue will appear out of order.
 * @param pq the priority queue
 */
void pq_print(EventQueue *pq){
    int i;
    printf("\n_______Event Queue_______\n");
    for(i=0;i< pq->size;i++){
        printf("%-5d| %-4s%d|%-20d\n" , pq->eventListPtr[i].time, "Job" , pq->eventListPtr[i].jobno, pq->eventListPtr[i].type);
    }                                                                                               //translate type to string
}

/**
 * Translates the typecode to a string
 * with its explanation
 * @param e the event in question
 * @param buffer a location for the string
 * @return the buffer filled with status as string
 */
char* type_to_string(Event *e, char *buffer){
    memset(buffer, 0, sizeof(*buffer));//clear the buffer
    switch(e->type){
        case 0:
            strcpy(buffer, " entered the system");
            break;
        case 1:
            strcpy(buffer, " entered CPU queue. \0");
            break;
        case 2:
            strcpy(buffer, " began using the CPU. \0");
            break;
        case 3:
            strcpy(buffer, " arrived at Disk 1. \0");
            break;
        case 4:
            strcpy(buffer, " began using Disk 1. \0");
            break;
        case 5:
            strcpy(buffer, " arrived at Disk 2. \0");
            break;
        case 6:
            strcpy(buffer, " began using Disk 2. \0");
            break;
        case 7:
            strcpy(buffer, " finished using the CPU. \0");
            break;
        case 8:
            strcpy(buffer, " finished using Disk 1. \0");
            break;
        case 9:
            strcpy(buffer, " finished using Disk 2. \0");
            break;
        case 10:
            strcpy(buffer, " exited the system. \0");
            break;
        case 11:
            strcpy(buffer, " ended the simulation. \0");
            break;
        default:
            printf("\nEvent type mismatch. Terminating program. \n");
            exit(7);
            break;
    }
    return buffer;    
}

void print_config(FILE *ptr, Config vals){
    fprintf(ptr, "SEED %d\n", vals.seed);
    fprintf(ptr, "INIT_TIME %d\n", vals.start_time);
    fprintf(ptr, "FIN_TIME %d\n", vals.end_time);
    fprintf(ptr, "ARRIVE_MIN %d\n", vals.arrive_min);
    fprintf(ptr, "ARRIVE_MAX %d\n", vals.arrive_max);
    fprintf(ptr, "QUIT_PROB %d\n", vals.quit_prob);
    fprintf(ptr, "CPU_MIN %d\n", vals.cpu_min);
    fprintf(ptr, "CPU_MAX %d\n", vals.cpu_max);
    fprintf(ptr, "D1_MIN %d\n" , vals.d1_min);
    fprintf(ptr, "D1_MAX %d\n" , vals.d1_max);
    fprintf(ptr, "D2_MIN %d\n" , vals.d2_min);
    fprintf(ptr, "D1_MAX %d\n" , vals.d1_max);
}

/**
 * Creates a random number between max and min
 * @param max the upper limit
 * @param min the lower limit
 * @return the value generated
 */
int random_gen(int max, int min){
    return (rand() % (max - min + 1)) + min;  
}

/**
 * Determines if an event exits, based off probability
 * @param percent the value of 
 * @return 1 if event occurs, 0 if not
 */
int prob_select(int percent){
    if((percent < 0) || percent > 100){
        printf("%d is an invalid probability. exiting." , percent);
        exit(8);
    }
    else{
        int temp = random_gen(0,100);
        if(temp <= percent)
            return 1;
        else
            return 0;
    }
}

/**
 * Handler for any job statuses that
 * warrant the processing of the cpu
 * @param job current job
 */
void process_cpu(Event *job, EventQueue *pq, Queue *cpu, 
        Queue *d1, Queue *d2, Config values, FILE *ptr){
    
    if (job->type == 1){//if job is an arrival
        char buffer[30];  //arrived into system.
        fprintf(ptr, "\nTime %d Job %d%s" , job->time, job->jobno , type_to_string(job, buffer));
        
        Event *next_job= e_create(job->time + random_gen(values.arrive_min, values.arrive_max), 1, job->jobno + 1); //next job
        pq_push(pq, *next_job); //enqueue the new job
        
        q_push(cpu , job);
    }
    else{//if job is done at cpu (7)
        cpu_status = 0;  //flip cpu status
        
        char buffer[30];  //job is done at cpu
        fprintf(ptr, "\nTime %d Job %d%s" , job->time, job->jobno , type_to_string(job, buffer));
        
        if (!prob_select(values.quit_prob)){//if going to the disk
            if(d1->size <= d2->size){//d1 has a shorter line
                Event d1 = *e_create(job->time , 3, job->jobno);
                pq_push(pq, d1);//add d1 arrival to pq
            }else{
                Event d2 = *e_create(job->time , 5, job->jobno);
                pq_push(pq, d2);//add d2 arrival to pq
            }
        }
        else{   //job is finished
            job->type = 10;
            char buffer[30];//write finish event to log
            fprintf(ptr, "\nTime %d Job %d%s" , job->time, job->jobno , type_to_string(job, buffer));
        }
    }
}

/**
 * Handler for any job statuses that
 * warrant the processing of the disks
 * @param job current job
 */
void process_disk(Event *job, EventQueue *pq, Queue *d1, Queue *d2, FILE *ptr){
    if(job->type == 3){//arrival d1
        q_push(d1, job);    //put existing job in d1 queue
        char buffer[30];    //write d1 arrival
        fprintf(ptr, "\nTime %d Job %d%s" , job->time, job->jobno , type_to_string(job, buffer));
    }
    else if(job->type == 5){//arrival d2
        q_push(d2, job);
        char buffer[30];  //write d2 arrival
        fprintf(ptr, "\nTime %d Job %d%s" , job->time, job->jobno , type_to_string(job, buffer));
    }
    else if(job->type == 8){//finish d1
        char buffer[30];    //write cpu re-arrival        
        d1_status = 0;//disk 1 is now idle
        //push same job back into cpu with cpu arrival status
        job->type = 1;  //set job to begin at cpu again
        pq_push(pq, *job);
        fprintf(ptr, "\nTime %d Job %d%s" , job->time, job->jobno , type_to_string(job, buffer));
    }
    else{
        char buffer[30];    //write cpu re-arrival
        d2_status = 0;//disk 2 is now idle
        //push same job back into cpu with cpu arrival status
        job->type = 1;  //set job to begin at cpu again
        pq_push(pq, *job);
        fprintf(ptr, "\nTime %d Job %d%s" , job->time, job->jobno , type_to_string(job, buffer));
    }
}
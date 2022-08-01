#include "3140_concur.h"
#include "realtime.h"
#include <stdlib.h>
#include <MKL46Z4.h>

/*
 * The variables defined in <realtime.h>
 */
volatile realtime_t current_time;
int process_deadline_met = 0;
int process_deadline_miss = 0;

//process_state data structure
struct process_state {
	unsigned int* sp; //stack pointer -- should be updated to cursp in process_select()
	unsigned int* orig_sp; //the start of the stack for this process. used when clearing memory after process terminates
	int stk_size; //necessary to know how much memory (from orig_sp) in stack to clear
	struct process_state* next; //the next process queued
	//Below are the new fields in our process_state
	//Inspired by the discussion slides for lab 5
	int is_realtime;
	int msec_start;
	int msec_end;
};
typedef struct process_state process_t;

process_t* current_process = NULL; //the pcb of the process currently running (re: process we are context switching from)
process_t* process_queue = NULL; //the queue of non-realtime processes
process_t* ready_queue = NULL; //the queue of ready realtime processes, in order of increasing absolute deadline
process_t* unready_queue = NULL; //the queue of unready realtime processes, in order of increasing absolute start time
/*
 * Enqueue for the non-realtime process_queue
 * Puts process_t* p at the end of the process_queue
 */
int enqueue (process_t* p) {
	process_t* tmp;
	int size = 0;
	if (process_queue == NULL){ //queue empty; this is first element
		process_queue = p;
		p->next = NULL;
		return 1;
	}
	else{
		tmp = process_queue;
		while (tmp->next != NULL){ //traverse to end of queue
			tmp = tmp->next;
			size += 1;
		}
		tmp->next = p; //add this process to the back of the queue
		size += 1;
		p->next = NULL;
	}
	return size;
};
/*
 * Dequeue for the non-realtime process_queue
 * Returns the head of the process_queue or NULL if it is empty
 */
process_t* dequeue (){
	if (process_queue == NULL){ //if queue is empty return null
		return NULL;
	}else {
		process_t* temp = process_queue;
		process_queue = process_queue->next;
		temp->next = NULL;
		return temp;
	}
}

/*
 * Enqueues process_t* p to the ready_queue
 * Specifically, inserts p to the appropriate position in ready_queue that maintains its invariant:
 * elements are in order of their absolute deadline
 */
void ready_enqueue(process_t* p){
	process_t* tmp;
	tmp = ready_queue;
	if (ready_queue == NULL){ //queue empty; this is first element
		ready_queue = p;
		p->next = NULL;
	}else if(tmp->next->msec_end >p->msec_end){ //case where pshould go to the front
		p->next = tmp;
		ready_queue = p;
	}else{
		//while we're not at the last element and the next elements are still earlier
		while (tmp->next != NULL && (tmp->next->msec_end < p->msec_end)){ //traverse to end of queue
			tmp = tmp->next;
		}
		process_t* tmp2 = tmp->next;
		tmp->next = p; //add this process to the back of the queue
		p->next = tmp2;
	}
	return;
}

/*
 * Enqueues process_t* p to the unready_queue
 * Specifically, inserts p to the appropriate position in ready_queue that maintains its invariant:
 * elements are in order of their absolute start time
 */
void unready_enqueue(process_t* p){
	process_t* tmp;
	tmp = unready_queue;
	if (unready_queue == NULL){ //queue empty; this is first element
		unready_queue = p;
		p->next = NULL;
	}else if(tmp->next->msec_start > p->msec_start){ //case where pshould go to the front
		p->next = tmp;
		unready_queue = p;
	}else{
		tmp = unready_queue;
		//while we're not at the last element and the next elements are still earlier
		while (tmp->next != NULL && (tmp->next->msec_start < p->msec_start)){ //traverse to end of queue
			tmp = tmp->next;
		}
		process_t* tmp2 = tmp->next;
		tmp->next = p; //add this process to the back of the queue
		p->next = tmp2;
	}
	return;
}

/*
 * Dequeues from the head of the ready_queue
 * Identical to normal dequeue just takes from the ready_queue
 */
process_t* ready_dequeue(){
	if (ready_queue == NULL){ //if queue is empty return null
			return NULL;
		}else {
			process_t* temp = ready_queue;
			ready_queue = ready_queue->next;
			temp->next = NULL;
			return temp;
		}
}

/*
 * Dequeues from the head of the unready_queue
 * Identical to normal dequeue just takes from the unready_queue
 */
process_t* unready_dequeue(){
	if (unready_queue == NULL){ //if queue is empty return null
			return NULL;
		}else {
			process_t* temp = unready_queue;
			unready_queue = unready_queue->next;
			temp->next = NULL;
			return temp;
		}
}
//
//
//Functions required by the lab
//
//
/*
 * Creates a non-realtime process
 */
int process_create (void (*f)(void), int n){
	process_t* new_process = malloc(sizeof(process_t)); //allocate space for this data

	if (new_process == NULL) return -1; //if there was no space, fail

	new_process -> sp = process_stack_init(f, n);
	if (new_process -> sp == NULL) return -1; //if we couldn't allocate space for this process's stack, fail

	new_process->orig_sp = new_process->sp; //at the start these two pointers at the same cuz this process hasn't run
	new_process->stk_size = n;
	//
	//Below are the new additions for lab5
	//
	new_process->is_realtime = 0; //isn't realtime
	new_process->msec_start = 0; //these fields are irrelevant -- think of them as Xs b/c this process is not realtime
	new_process->msec_end = 0;
	enqueue(new_process); //queue this process to non-realtime queue
	return 0;


};

/*
 * Creates a realtime process
 */
int process_rt_create(void (*f)(void), int n, realtime_t* start, realtime_t* deadline){
	process_t* new_process = malloc(sizeof(process_t)); //allocate space for this data
	if (new_process == NULL) return -1; //if there was no space, fail

	new_process -> sp = process_stack_init(f, n);
	if (new_process -> sp == NULL) return -1; //if we couldn't allocate space for this process's stack, fail

	new_process->orig_sp = new_process->sp; //at the start these two pointers at the same cuz this process hasn't run
	new_process->stk_size = n;
	//
	//Realtime-specific things below
	//
	new_process->is_realtime = 1; //this is realtime
	new_process->msec_start = start->msec + start->sec * 1000; //store the start time in milliseconds
	new_process->msec_end = deadline->msec + start->msec + 1000 * ( deadline->sec + start->sec); //store absolute deadline in msec
	//now check if this process is ready, this decides which queue to enqueue it to
	if (new_process->msec_start > (current_time.msec + 1000 * current_time.sec)){
		//unready
		unready_enqueue(new_process);
	}else{
		//ready
		ready_enqueue(new_process);
	}
	return 0;
}
/*
 * Not implementing this
 */
int process_rt_periodic(void (*f)(void), int n, realtime_t *start, realtime_t *deadline, realtime_t *period){
	return 0;
}
/*
 * Initializes our scheduler
 */
void process_start (void){

	//the above stuff is from OH -- we want to avoid an infinitely nested loop of process_select calls if we let PIT0 (context switch)
	//take us out of the waiting while loop w/o an updated current time

	SIM->SCGC6 = SIM_SCGC6_PIT_MASK; // Enable clock to PIT module
	PIT->MCR = PIT_MCR_FRZ_MASK;// Turn on PIT
	PIT->CHANNEL[0].LDVAL = 0x00A0000; //Load period
	PIT->CHANNEL[1].LDVAL = 0x0002710; //Load the period of the timer w/ 10,000 cycles per interrupt (1 interrupt/ms))

	//PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK; //Enable PIT0
	PIT->CHANNEL[1].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK; //Enable PIT1

	NVIC_EnableIRQ(PIT_IRQn);
	NVIC_EnableIRQ(SVCall_IRQn);
	NVIC_SetPriority(PIT_IRQn, 0);
	NVIC_SetPriority(SVCall_IRQn, 1);


	//also set up current_time
	current_time.msec = 0;
	current_time.sec = 0;
	process_begin();
};

//interrupt handler for PIT1. Increment current_time
void PIT1_Service (void) {
	PIT->CHANNEL[1].TFLG |= 1; //Clears the timeout
	if (current_time.msec < 999){
		current_time.msec += 1;
	}else{
		current_time.msec = 0;
		current_time.sec += 1;
	}
}

/*
 * Our original process select from lab3
 * Round robing scheduling from process_queue, with some alteration for realtime considerations
 * Specifically if the process that's quitting is realtime
 */
unsigned int * round_robin_process_select(unsigned int * cursp){
	//if we are just starting up -- no process has been run yet
	if (current_process == NULL){
		//if no process has been queued
		if (process_queue == NULL){
			//quit everything
			return NULL;
		}
		//otherwise, get the first process from the pq
		current_process = dequeue();
		return current_process->sp;
	}else if (current_process->is_realtime == 1){
		//we also need to check before going further if the
		//process we're switching from is or isn't rt
		//if it is, keep running it -- it has a deadline; nonrt processes have deadline inf
		if (cursp != NULL){
			return cursp;
		}
	}
	//now we have two guarantees: have a previous process, and it either wasn't realtime or was realtime but terminated.
	process_t* prev_proc = current_process; //save what that previous process was
	prev_proc->sp = cursp; //and update the previous process' sp to wherever it just left off
	//if we have other processes to choose from
	if (process_queue != NULL){
		//choose the next process round-robin style
		current_process = dequeue();
		if (cursp == NULL){
			//if this has terminated, free stuff but also check if the deadline was missed
			NVIC_DisableIRQ(PIT_IRQn);
			if (prev_proc->is_realtime == 1){
				if (current_time.msec + 1000 * current_time.sec > prev_proc->msec_end){
					process_deadline_miss+=1;
				}else{
					process_deadline_met += 1;
				}
			}
			process_stack_free(prev_proc->orig_sp, prev_proc->stk_size);
			free(prev_proc);
			NVIC_EnableIRQ(PIT_IRQn);
		}else{
			if (prev_proc->is_realtime == 1){
				ready_enqueue(prev_proc);
			}else{
				//recall we're only in this branch if prev process hasn't terminated -- which means it wasn't realtime
				//and goes into the normal process_queue
				enqueue(prev_proc);
			}
		}
	}else{
		//have no more processes in process queue
		if (cursp == NULL){
			//do the freeing stuff but also...
			NVIC_DisableIRQ(PIT_IRQn);
			//make sure to update the process_deadline_miss and process_deadline_met if this was realtime
			if (prev_proc->is_realtime == 1){
				if (current_time.msec + 1000 * current_time.sec > prev_proc->msec_end){
					process_deadline_miss+=1;
				}else{
					process_deadline_met += 1;
				}
			}
			process_stack_free(prev_proc->orig_sp, prev_proc->stk_size);
			free(prev_proc);
			NVIC_EnableIRQ(PIT_IRQn);
			return NULL;
		}else{
			//if the prev proc wasn't done and we have nothing else to choose from, j keep running it
			return prev_proc->sp;
		}
	}
	//by this point we've put the front of the process_queue into current_process
	return current_process->sp;

}

/*
 * A process selector that works on the assumption that we have ready processes to choose from
 * Has no guarantee on whether or not current process was realtime or not -- takes care of that
 */
unsigned int * from_ready_queue_process_select(unsigned int * cursp){
	//if we are just starting our concurrency...
	if (current_process == NULL){
		//this branch shouldn't happen based on logic in real process_select
		if (ready_queue == NULL){ //if we have no processes to run, fail
			return NULL;
		}
		current_process = ready_dequeue();
		return current_process->sp;
	}else{
		//there is a process that we are context switching from
		process_t* prev_proc = current_process; //save what that previous process was
		prev_proc->sp = cursp; //and update the previous process' sp to wherever it just left off
		if (ready_queue != NULL){
			//if there's another ready process, consider the front of the ready_queue
			current_process = ready_dequeue();
			if (cursp == NULL){
				//if the previous process terminated, do the freeing stuff and also check if we need to update our counters
				NVIC_DisableIRQ(PIT_IRQn);
				//check if it was rt and then if deadline was missed
				if (prev_proc->is_realtime == 1){
					if (current_time.msec + 1000 * current_time.sec > prev_proc->msec_end){
						//missed deadline
						process_deadline_miss+=1;
					}else{
						process_deadline_met += 1;
					}
				}
				process_stack_free(prev_proc->orig_sp, prev_proc->stk_size);
				free(prev_proc);
				NVIC_EnableIRQ(PIT_IRQn);
			}else{
				//if it's non real time put it back in process_queue otherwise if it is put in ready queue
				if (prev_proc->is_realtime == 1){
					//check if this has an earlier deadline -- if so re-enqueue current_process and just return this sp
					if (prev_proc->msec_end <= current_process->msec_end){
						ready_enqueue(current_process); //put this back (constant time cuz it gets added to the front)
						current_process = prev_proc;
						return prev_proc->sp; //keep running this process b/c it's deadline is still earliest
					}
					ready_enqueue(prev_proc);
				}else{
					enqueue(prev_proc);
				}
			}
		}
		//if there wasn't another ready process, we can only have entered this function if current_process is_realtime.
		//so in this other case we don't really do anything: we do return the current_process's sp because it hasn't been
		//updated to anything else, and we know it's realtime and hasn't terminated, so it's ok to keep going with it
		return current_process->sp;
	}
}
/*
 * The main process select with logic to coordinate realtime and non-realtime processes
 */
unsigned int * process_select(unsigned int * cursp){
	// First check if the unready_queue has an processes that are now ready
	// This is easy b/c the unready_queue is sorted in incr order of start time -- so takes an at most linear pass to get
	// all that is ready
	if (unready_queue != NULL){
		process_t* tmp = unready_queue;
		while (tmp != NULL && (tmp->msec_start < (current_time.sec * 1000 + current_time.msec))){
			process_t* to_enq = unready_dequeue();
			ready_enqueue(to_enq);
			tmp = unready_queue;
		}
	}
	//if we've got stuff on the ready queue or the process we're cs from is realtime and hasn't terminated
	if (ready_queue != NULL || (current_process->is_realtime == 1 && cursp != NULL)){
		//do the realtime ps
		return from_ready_queue_process_select(cursp);
	} else if (ready_queue == NULL && (process_queue != NULL || (current_process->is_realtime == 0 && cursp != NULL))){ //if nothing is ready but we have non-realtime to run
		//if there isn't anything on the ready queue and pq isn't null or our last process was non-rt and hasn't terminated
		//do non-realtime ps
		return round_robin_process_select(cursp);
	}else if (unready_queue != NULL){ //if none in ready and none in pq
		//in this case, a process must have also just terminated (otherwise one of the other branches would have fired)
		if (cursp == NULL && current_process->is_realtime == 1){
			//means it just ended and was realtime
			if (current_time.msec + 1000 * current_time.sec > current_process->msec_end){
				process_deadline_miss+=1;
			}else{
				process_deadline_met += 1;
			}
		}
		process_t* not_ready = unready_dequeue(); //the process closest to being ready
		while (not_ready->msec_start > (current_time.sec * 1000 + current_time.msec)){
					//don't do anything -- wait
					if (PIT->CHANNEL[1].TFLG == 1){
						PIT1_Service();
					}
				}
		ready_enqueue(not_ready);
		return from_ready_queue_process_select(cursp);
	} else { //no processes left in any queue
		//check if a process just terminated
		if (cursp == NULL && current_process->is_realtime == 1){
			//means it just ended and was realtime
			if (current_time.msec + 1000 * current_time.sec > current_process->msec_end){
				process_deadline_miss+=1;
			}else{
				process_deadline_met += 1;
			}
		}
		return NULL;
	}
}

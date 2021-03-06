/*  main.c  - main */

#include <xinu.h>
#define CONSUMED_MAX 100 // we will use this for buffer size

pid32 producer_id; // producer process
pid32 consumer_id; // consumer process
pid32 timer_id; // timer process

int32 shared_buffer[CONSUMED_MAX];// shared buffer
int32 consumed_count = 0; // consumer count
int32 data = 1; // data to be added in buffer
int32 head = 0 ; // head of Queue
int32 tail = 0; // tail of Queue

sid32 shared_buffer_lock; // mutex to locak the buffer
sid32 producer; // semaphore for producer
sid32 consumer; // semaphore for consumer

/* mutex implementations */
void mutex_acquire(sid32 mutex)
{
	wait(mutex);
}

void mutex_release(sid32 mutex)
{
	signal(mutex);
}

/*producer produces data at tail of the shared buffer if there is space in the shared buffer*/
process producer1(void)
{
	while(TRUE)
	{
		wait(producer);

		mutex_acquire(shared_buffer_lock);
		shared_buffer[tail] = data;
		kprintf("Producing %d on %d\n", data,tail);				
		tail = ((tail+1)%CONSUMED_MAX);
		data++;
		mutex_release(shared_buffer_lock);	
	
		signal(consumer);
		
	}

	return OK;
}


/*consumer consumes data from the head of the shared buffer if there are more items left to consume*/
process consumer1(void)
{

	while(TRUE)
	{
		wait(consumer);

		mutex_acquire(shared_buffer_lock);		
		int temp = shared_buffer[head];
		shared_buffer[head] = 0; //clear contents of location
		consumed_count += 1;
		kprintf("Consuming %d from %d\n", temp,head);		
		head = (head+1)%CONSUMED_MAX;
		mutex_release(shared_buffer_lock);

		signal(producer);
		
	}


	return OK;
}

/* Timing utility function for comapring implementations */
process time_and_end(void)
{
	int32 times[5];
	int32 i;

	for (i = 0; i < 5; ++i)
	{
		times[i] = clktime_ms;
		yield();

		consumed_count = 0;
		while (consumed_count < CONSUMED_MAX * (i+1))
		{
			yield();
		}

		times[i] = clktime_ms - times[i];

	}

	kill(producer_id);
	kill(consumer_id);

	for (i = 0; i < 5; ++i)
	{
		kprintf("TIME ELAPSED (%d): %d\n", (i+1) * CONSUMED_MAX, times[i]);
	}
}

//main process that creates producer and consumer process
process	main(void)
{
	recvclr();
	shared_buffer_lock = semcreate(1); // this is a mutex
	producer = semcreate(CONSUMED_MAX); // semaphore
	consumer = semcreate(0); // semaphore
	producer_id = create(producer1, 4096, 50, "producer", 0);
        consumer_id = create(consumer1, 4096, 50, "consumer", 0);	 
	timer_id = create(time_and_end, 4096, 50, "timer", 0);
	resched_cntl(DEFER_START);
	resume(producer_id);
	resume(consumer_id);
	resume(timer_id);
	resched_cntl(DEFER_STOP);
	return OK;
}

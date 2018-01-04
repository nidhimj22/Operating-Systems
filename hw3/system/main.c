#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>

// to control kprintf
sid32 mutex;
pid32 subscriber1, subscriber2, publisher, broker_pid;

void call_back1(topic16 topic,uint32 data){
	kprintf("Function call_back1 is called with arguments %d and %d\n",topic,data);
	
}
void call_back2(topic16 topic,uint32 data){
	kprintf("Function call_back2 is called with arguments %d and %d\n",topic,data);

}

process example_process1 (void){
	void (*handler)(topic16,uint32);
	handler = call_back1;	
	subscribe(10, handler);
	kprintf("process example_process1 subscribed to topic 10 with handler call_back1\n"); 
	subscribe(0x8704, handler);
	kprintf("process example_process1 subscribed to 0x8704 with handler call_back1\n"); 
	return OK;
}

process example_process2 (void){
	void (*handler)(topic16,uint32);
	handler = call_back2;
	subscribe(4, handler);
	kprintf("process example_process2 subscribed to topic 4 with handler call_back2\n"); 
	subscribe(0x870A, handler);
	kprintf("process example_process2 subscribed to 0x870A with handler call_back2\n"); 
	return OK;
}

process example_process3 (void){
	publish(10,100);
	kprintf("process example_process3 publishes to topic 10 data 100\n");
	publish(4,40);
	kprintf("process example_process3 publishes to topic 4 data 40\n");
	publish(0x8704,80);
	kprintf("process example_process3 publishes to 0x8704 data 80\n");
	publish(0x870A,200);
	kprintf("process example_process3 publishes to 0x870A data 200\n");
	return OK;
}


syscall  subscribe(topic16  topic,  void (*handler)(topic16, uint32)){
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;	/* ptr to process' table entry	*/
	mask = disable();
	pid32 pid = getpid();
	if (isbadpid(pid)) {
		restore(mask);
		return SYSERR;
	}
	prptr = &proctab[pid];

	// check if topic is already not subscribed by 8 processes
	if (topictab[topic].num_processes>=MAXCOUNT){
		restore(mask);
		wait(mutex);
		kprintf("*Topic is already subscribed to 8 processes*"); 
		signal(mutex);
	
		return SYSERR;
	}

  	//check if process can subscribe to the topic, group/topic
	uint8 group_number = (uint8)((topic & (0xFF << 8) ) >> 8);
	uint8 topic_number = (uint8)(topic & 0xFF);

	if (prptr->nsub == 256)
	{
		restore(mask);
		wait(mutex);
		kprintf("*Process is already subscribed to 256 topics*"); 
		signal(mutex);
		return SYSERR;
	}

	if (prptr->groups[topic_number]!=0){
		restore(mask);
		wait(mutex);
		kprintf("*Process is already subscribed to the same topic in another group*"); 
		signal(mutex);
		
		return SYSERR;
	}

	// make changes in the process tab
	prptr->groups[topic_number] = group_number;
	prptr->nsub++;

	// add process id to the topictab
	uint32 i = topictab[topic].num_processes;
	topictab[topic].pid[i] = pid;
	topictab[topic].handler[i] = handler;
	topictab[topic].num_processes++;
	
	restore(mask);		/* restore interrupts */
	wait(mutex);
	kprintf("Process %d subscribed to group_nummber %d and topic_number %d",pid,group_number,topic_number); 
	signal(mutex);
	return OK;
}


syscall  unsubscribe(topic16  topic){
    intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;	/* ptr to process' table entry	*/
	mask = disable();
	pid32 pid = getpid();
	if (isbadpid(pid)) {
		restore(mask);
		return SYSERR;
	}
	prptr = &proctab[pid];
  
	// check if process is subscribed to the topic
	if (prptr->nsub <= 0) {
		restore(mask);
		wait(mutex);
		kprintf("Process %d is not subscribed to any topic",pid); 
		signal(mutex);
		return SYSERR;
	}
	uint8 group_number = (uint8)((topic & (0xFF << 8) ) >> 8);
	uint8 topic_number = (uint8)(topic & 0xFF);

	if (prptr->groups[topic_number]!=group_number){
		restore(mask);
		wait(mutex);
		kprintf("Process %d is not subscribed to the topic %d",pid,topic); 
		signal(mutex);
		return SYSERR;
	}

	// change process tab 
	prptr->groups[topic_number] = 0;
	prptr->nsub = prptr->nsub - 1;

	// change topictab	
	uint32 j = 0;
	for (j=0;j<topictab[topic].num_processes;j++){
		if (topictab[topic].pid[j]==pid){
			while(j<topictab[topic].num_processes -1){
				topictab[topic].pid[j] = topictab[topic].pid[j+1];
				topictab[topic].handler[j] = topictab[topic].handler[j+1];
			}
			topictab[topic].num_processes--;
			break;
		}
	}

	restore(mask);		/* restore interrupts */
	wait(mutex);
	kprintf("Process %d unsubscribed from group_nummber %d and topic_number %d",pid,group_number,topic_number); 
	signal(mutex);
	return OK;

}


syscall  publish(topic16  topic,  uint32  data){
	intmask	mask;			/* saved interrupt mask		*/
	mask = disable();
	pid32 pid = getpid();
	if (isbadpid(pid)) {
		restore(mask);
		return SYSERR;
	}
	// create new node for linked list
  	datum *new_datum  = (datum *)getmem(sizeof(datum));
	if((int32)new_datum == SYSERR) {
		restore(mask);
		return SYSERR;
	}

	new_datum->data = data;
	new_datum->topic = topic;
	// insert new node at tail of linked list
	if (tail_data==NULL){
		head_data = tail_data = new_datum;
	}
	else{
		tail_data->next = new_datum;
		tail_data = tail_data->next;
	}
	restore(mask);		/* restore interrupts */
	wait(mutex);
	kprintf("process %d published %d to topic %d\n", pid,data,topic);
	signal(mutex);
	return OK;
}

datum *get_next_pending_publish(){
	if (head_data==NULL)
		return NULL;
	datum *pending = head_data;
	// remove node from head of list
	if (head_data==tail_data){
		head_data = tail_data = NULL;
	}
	else
		head_data = head_data->next;
	return pending;
}

process broker(void) {
	kprintf("*Broker started running*");
	while(TRUE)
	{  
	  	datum *temp = get_next_pending_publish();
		if(temp!=NULL){
	
  			uint32 i=0;
		  	
			topic16 topic = temp->topic;
		  	uint8 group_number = (uint8)((topic & (0xFF << 8) ) >> 8);
			uint8 topic_number = (uint8)(topic & 0xFF);
			kprintf("Broker is calling handlers");
			for(i=0;i<topictab[topic].num_processes;i++){
		  		(*topictab[topic].handler[i])(temp->topic,temp->data);
		  	}
		  	// if groupd number is 0, send message to all groups from 1 to 255
		  	if(group_number==0){
		  		uint8 group;
		  		for(group=1;group<256;group++){
		  			topic16 new_topic_number = (topic16)(((topic16)group) << 8 | topic_number);		  			
					for(i=0;i<topictab[new_topic_number].num_processes;i++)
				  		(*topictab[new_topic_number].handler[i])(temp->topic,temp->data);
		  		}
		  	}
			freemem((char *)temp,sizeof(datum)); // delete the data now as it is no longed needed
		}  	
	}
  			
	return OK;
}

process main(void)
{
	mutex = semcreate(1);
	
	subscriber1=create(example_process1, 4096,50,"subscriber1", 0);
	subscriber2=create(example_process2, 4096,50,"subscriber2", 0);
	publisher=create(example_process3, 4096,50,"publisher", 0);
	broker_pid=create(broker, 4096,50,"broker", 0);

	resume(publisher);
	resume(subscriber1);
	resume(subscriber2);
	resume(broker_pid);	

	return OK;
}

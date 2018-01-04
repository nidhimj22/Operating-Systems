#include <xinu.h>
#include <stdio.h>
#include <stdlib.h>

// to control kprintf
sid32 mutex;

/*
Special methods to manipulate the message queue in the procent structure 
message is added at queue and mwessage is removed from the head so that order of receiving messages is not changed
*/

void addMessage(struct procent *prptr, umsg32 message)
{
	prptr->messagequeue[prptr->qtail] = message;
	prptr->qlength++;
	prptr->qtail = (prptr->qtail+1)%MAXCOUNT;
}

umsg32 removeMessage(struct procent *prptr)
{
	umsg32 returnValue = prptr->messagequeue[prptr->qhead];
	prptr->messagequeue[prptr->qhead] = NULL;
	prptr->qlength--;
	prptr->qhead = (prptr->qhead+1)%MAXCOUNT;
	return returnValue;
}

/*
To send or receive messages, we follow the same protocol
1. save the interupt mask
2. obtain the process table entry
3. disbale the interupt mask
4. check if good pid
5. check message queue for addition or removal of messages
6. add or remove element
7. set process state
8. restore the interupts 
*/
syscall sendMsg(pid32 pid, umsg32 msg)
{
  intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;	/* ptr to process' table entry	*/
	mask = disable();
	if (isbadpid(pid)) {
		restore(mask);
		return SYSERR;
	}
	prptr = &proctab[pid];
  
  // check if message can be added to the messagequeue
	if ((prptr->prstate == PR_FREE) || (prptr->qlength == MAXCOUNT))
	{
		restore(mask);
		wait(mutex);
		kprintf("*Message Queue for Process %d is full. Message not sent.*\n",pid); 
		signal(mutex);
		return SYSERR;
	}
	addMessage(prptr,msg); //enqueues the element
	/* If recipient waiting or in timed-wait make it ready */
	if (prptr->prstate == PR_RECV) {
		ready(pid);
	} else if (prptr->prstate == PR_RECTIM) {
		unsleep(pid);
		ready(pid);
	}
	restore(mask);		/* restore interrupts */
	wait(mutex);
	kprintf("Message %d sent to the process %d\n", msg,pid);
	signal(mutex);
	return OK;
}

uint32 sendMsgs(pid32 pid, umsg32* msgs, uint32 msg_count)
{
	if(msg_count > MAXCOUNT || msg_count <= 0) //will be more than the queue can handle
	{
		wait(mutex);
		kprintf("*You can only send 1 to 10 messages*\n");
		signal(mutex);
		return SYSERR;
	}
	else
	{

	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;	/* ptr to process' table entry	*/
	mask = disable();
	if (isbadpid(pid)) {
		restore(mask);
		return SYSERR;
	}
	prptr = &proctab[pid];

  // check if message can be added to the messagequeue
	if ((prptr->prstate == PR_FREE) || (prptr->qlength == MAXCOUNT))
	{
		restore(mask);
		wait(mutex);
		kprintf("*Message Queue for Process %d is full. Message not sent.*\n",pid); 
		signal(mutex);
		return SYSERR;
	}
	uint32 messages_sent = MAXCOUNT - prptr->qlength;
	uint32 i = 0;
	for(i=0; i < messages_sent; i++){
		addMessage(prptr,msgs[i]); //enqueues the element
		wait(mutex);
		kprintf("Message %d sent to the process %d\n", msgs[i],pid);
		signal(mutex);
	}
	/* If recipient waiting or in timed-wait make it ready */
	if (prptr->prstate == PR_RECV) {
		ready(pid);
	} else if (prptr->prstate == PR_RECTIM) {
		unsleep(pid);
		ready(pid);
	}
	restore(mask);		/* restore interrupts */
	wait(mutex);
	kprintf("Sent %d # of messages to process %d\n", messages_sent, pid);
	signal(mutex);
	return messages_sent;
	}
}


umsg32 receiveMsg(void)
{
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;		/* ptr to process' table entry	*/
	umsg32	msg;			/* message to return		*/
	mask = disable();
	prptr = &proctab[currpid]; //gets current process
	if (prptr->qlength == 0) { //empty queue
		prptr->prstate = PR_RECV;
		resched();		/* block until message arrives	*/
	}
	msg = removeMessage(prptr);
	restore(mask);
	wait(mutex);
	kprintf("Message %d received by process %d\n", msg, currpid);
	signal(mutex);
	return msg;
}

syscall receiveMsgs(umsg32* msgs, uint32 msg_count)
{
	if(msg_count <= 0 || msg_count > MAXCOUNT) //more than the queue can hold
	{
		wait(mutex);
		kprintf("*You can only receive 1 to 10 messages*\n");
		signal(mutex);
		return SYSERR;
	}
	intmask	mask;			/* saved interrupt mask		*/
	struct	procent *prptr;	/* ptr to process' table entry	*/
	mask = disable();
	prptr = &proctab[currpid];
	if (prptr->qlength < msg_count) { //not all the messages have been sent
		prptr->prstate = PR_RECV;
		resched();		/* block until all messages have been sent */
	}
	
	uint32 i = 0;
  umsg32	msg;
	for (i=0; i < msg_count; i++){
		msg = removeMessage(prptr); //read from message queue
	  msgs[i] = msg;
		wait(mutex);
		kprintf("Message %d received by process %d\n", msg, currpid);
		signal(mutex);
	}

	restore(mask);
	wait(mutex);
	kprintf("Received %d # of messages by process %d\n", msg_count,currpid);
	signal(mutex);
	return OK;
}

uint32 sendnMsg (uint32 pid_count, pid32* pids, umsg32 msg)
{
	if(pid_count<= 0 || pid_count > 3) //too many processes being sent messages
	{
		wait(mutex);
		kprintf("*You can only send to 1 to 3 processes*\n");
		signal(mutex);
		return SYSERR;
	}
	else
	{
		uint32 success_pid = 0;
		int32 i = 0;
		while(i < pid_count)
		{
			if(sendMsg(*(pids + i), msg) == SYSERR) //attempt to send message
			{
				wait(mutex);
				kprintf("*Sending message %d to process %d resulted in an error.*\n", msg, *(pids + i)); //queue is full
				signal(mutex);
			}
			else
			{
				success_pid++;
				wait(mutex);
				kprintf("Message %d sent to the process %d\n", msg,pids[i]);
				signal(mutex);
				i++; //continue
			}

		}
		wait(mutex);
		kprintf("Sent %d message to %d #  of processes %d\n", msg,success_pid);
		signal(mutex);
		return pid_count;
	}
}

process main(void)
{
	mutex = semcreate(1);
	/* 
	send message test 
	receivemsg test
	sendmsgs test
	receivemsgs test
	*/
	uint32 messages [MAXCOUNT];
	kprintf("TESTING SEND MESSAGE\n");
	messages[0] = 0;
	messages[1] = 1;
	messages[2] = 2;
	messages[3] = 3;
	messages[4] = 4;
	messages[5] = 5;
	messages[6] = 6;
	messages[7] = 7;
	messages[8] = 8;
	messages[9] = 9;
	int32 i = 0;
	while(i < MAXCOUNT){
		sendMsg(getpid(), messages[i]);
		i++;	
	}
	kprintf("TESTING RECEIVE MESSAGE\n");
	i = 0;
	while(i < MAXCOUNT)
	{
		receiveMsg();
		i++;
	}
	kprintf("TESTING SEND MESSAGES\n");
	sendMsgs(getpid(), messages, MAXCOUNT);
	kprintf("TESTING receive MESSAGES\n");
	receiveMsgs(messages, MAXCOUNT);
	return OK;
}
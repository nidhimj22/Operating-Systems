/* userret.c - userret */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  userret  -  Called when a process returns from the top-level function
 *------------------------------------------------------------------------
 */
void	userret(void)
{
	pid32 pid = getpid();
	struct	procent *prptr;	/* ptr to process' table entry	*/
	prptr = &proctab[pid];
  
  // check if process is subscribed to the topic
	if (prptr->nsub > 0) {
		uint8 i;
		for(i=0;i<256;i++){	
			if (prptr->groups[i]!=0){
				topic16 new_topic_number = ((topic16)i) << 8 | prptr->groups[i];
				unsubscribe(new_topic_number);
			}
		}
	}
	
	kill(getpid());			/* Force process to exit */
}

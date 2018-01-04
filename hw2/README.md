Changes are in the file - 
1. main.c (system calls are implemented here)
2. protypes.h (system calls are declared here)
3. process.h (process table is changed here)
4. create.c (process table creation is changed here)

I have changed the process table by adding a message queue, a head and tail element, and a length for the message queue. 
Now there is a queue which is there in the process table. When new messages are added they get added to the buffer and are removed from there itself. 
Circular buffer has been used to managed the addition and removal of messages. 

To test the code, just replace my process main with your driver code. Also make changes to the create.c, prototypes.h and process.h. Compile the code and run again. 

Sample test code to check my working is added in the void main function. 

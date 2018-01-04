The changes in the code are in the include and system folders. 
The syscalls are implemented in system/main.c

Process tab and topic tab have been modified in the include/process.h file.
I have used a Linked list for all the data that is published .. that is also in include/process.h
There are some modifictions in system/create.c because of change in the process table.
include/kernal.h and include/prototypes.h have the declarations
system/userret.c contains code for a process to unsubscribe before kill
system/initialize.c has modifies initiliases the variables.

Please use these files and change the main to test. Dont forget to resume(create(broker..)) in the main process.

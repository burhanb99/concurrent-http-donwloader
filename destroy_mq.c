#include <stdio.h> 
#include <sys/ipc.h> 
#include <sys/msg.h> 

int main()
{
	key_t key; 
    int msgid; 
  
    // ftok to generate unique key 
    key = ftok("progfile", 65); 
  
    // msgget creates a message queue 
    // and returns identifier 
    msgid = msgget(key, 0666 | IPC_CREAT); 

    //destory mq
    msgctl(msgid, IPC_RMID, NULL); 
  
    return 0; 


}//end main
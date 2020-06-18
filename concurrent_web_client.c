#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/stat.h> 
#include <semaphore.h>
#include <fcntl.h> 

#define SEM_NAME "/semaphore_cwc"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
sem_t *semaphore_cwc;
//Structure for message queue
typedef struct msg_buffer { 
    int type; 
    int clen;
    int status; 
}msg;

int msgid; 
int sock;
char* domain_name;
char* file_path;
int N;
char* ftype;

int ReadHttpStatus(int sock)
{
    char c;
    char buff[1024]="",*ptr=buff+1;
    int bytes_received, status;
    //printf("Begin Response ..\n");
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1){
            perror("ReadHttpStatus");
            exit(1);
        }

        if((ptr[-1]=='\r')  && (*ptr=='\n' )) break;
        ptr++;
    }
    *ptr=0; 
    ptr=buff+1;

    sscanf(ptr,"%*s %d ", &status);

    //printf("%s\n",ptr);
    //printf("status=%d\n",status);
    //printf("End Response ..\n");
    return (bytes_received>0)?status:0;

}//end ReadHttpStatus

//Parses response and returns 'Content-Length' 
int ParseHeader(int sock)
{
    char c;
    char buff[1024]="",*ptr=buff+4;
    int bytes_received, status;
    //printf("Begin HEADER ..\n");
    while(bytes_received = recv(sock, ptr, 1, 0)){
        if(bytes_received==-1)
        {
            perror("Parse Header");
            exit(1);
        }

        if(
            (ptr[-3]=='\r')  && (ptr[-2]=='\n' ) &&
            (ptr[-1]=='\r')  && (*ptr=='\n' )
        ) break;
        ptr++;
    }

    *ptr=0;
    ptr=buff+4;
    //printf("%s",ptr);

    if(bytes_received)
    {
        ptr=strstr(ptr,"Content-Length:");
        if(ptr)
        {
            sscanf(ptr,"%*s %d",&bytes_received);
        }
        else
            bytes_received=-1; //unknown size

       //printf("Content-Length: %d\n",bytes_received);
    }
    //printf("End HEADER ..\n");
    return  bytes_received ;

}//end parse header

int fetch_and_push(int range, int i)
{
    char send_data[1024];
    //printf("in fetch_and_push for child %d\n" , i);
    if(i != N)
        snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-%d\r\n\r\n", file_path, domain_name, ((i-1)*range), (i*range-1));
    else if(i == N)
        snprintf(send_data, sizeof(send_data), "GET /%s HTTP/1.1\r\nHost: %s\r\nRange: bytes=%d-\r\n\r\n", file_path, domain_name, ((i-1)*range));

    //send GET request for data in the particular range
    if(send(sock, send_data, strlen(send_data), 0)==-1)
    {
        perror("Failed to send GET msg to server\n");
        exit(2); 
    }

    msg m;
    m.type = i;
    m.status = ReadHttpStatus(sock);
    m.clen = ParseHeader(sock);
    printf("##############\n");
    printf("m.type : %d\n", m.type);
    printf("m.status : %d\n", m.status);
    printf("m.clen : %d\n", m.clen);
    printf("##############\n");

    //write the recieved segment of data in a temporary file which will later be joined by the parent process
    if(m.status == 206 || m.status == 200)
    {
        int bytes_received;
        char recv_data[1024];
        char open[50];
        sprintf(open , "temp%d.%s" , i, ftype);
        FILE* fd=fopen(open,"wb");
        int bytes = 0;
        while(bytes_received = recv(sock, recv_data, 1024, 0))
        {
            if(bytes_received==-1)
            {
                perror("receive\n");
                exit(3);
            }
            fwrite(recv_data,1,bytes_received,fd);
            bytes+=bytes_received;
            if(bytes==m.clen)
            {
                break;
            }
        }//end while 
    }//end if status
    
    //send the http status , content length to the parent to take necessary further action
    msgsnd(msgid, &m, sizeof(msg), 0);
    return 0;
}//end fetch_and_push

int main(int argc, char* argv[])
{
	if(argc != 5)
	{
		printf("Incorrect format or too few arguments\n");
		printf("Use as : \n");
		printf("concurrent_downloader <domain_name> <file_path(on_server)> <N(level_of_concurrency)> <save_file_name>\n");
        exit(1);
	}//end if

    //Initialize semaphore 
    sem_unlink(SEM_NAME);
    semaphore_cwc = sem_open(SEM_NAME, O_CREAT | O_EXCL, SEM_PERMS, 1);
    if (semaphore_cwc == SEM_FAILED) 
    {
        perror("sem_open() error");
        exit(EXIT_FAILURE);
    }

	//Store CLA's in varibles
	domain_name = argv[1];
	file_path = argv[2];
	N = atoi(argv[3]);
    char save[50];
    strcpy(save , argv[4]);
    ftype = strtok(argv[4] , ".");
    ftype = strtok(0 , "\0");
	
    //Declare necessary variables
	struct sockaddr_in server_addr;
    struct hostent *h;
    char send_data[1024];
  
    //Initialize the message queues
    key_t key = ftok("progfile", 65); 
    msgid = msgget(key, 0666 | IPC_CREAT);
    msgctl(msgid, IPC_RMID, NULL);
    msgid = msgget(key, 0666 | IPC_CREAT);
    
    //Resolve domain name to IP 
    h = gethostbyname(domain_name);
    if (h == NULL)
    {
       herror("gethostbyname Error\n");
       exit(1);
    }

    //open socket for communication
    if ((sock = socket(AF_INET, SOCK_STREAM, 0))== -1)
    {
       perror("Socket Error\n");
       exit(1);
    }

    //set the paramters of sockaddr_in structure to establish a connection
	server_addr.sin_family = AF_INET;//ipv4     
    server_addr.sin_port = htons(80);//http port 80
    server_addr.sin_addr = *((struct in_addr *)h->h_addr);//ip address
    bzero(&(server_addr.sin_zero),8); 

    //Establish the connection
    printf("Connecting ...\n");
    if (connect(sock, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1)
    {
       perror("Connection Error\n");
       exit(1); 
    }

    //Send HEAD request to fetch the file size
    printf("Sending HEAD request to fetch file size ...\n");
    snprintf(send_data, sizeof(send_data), "HEAD /%s HTTP/1.1\r\nHost: %s\r\n\r\n", file_path, domain_name);
	if(send(sock, send_data, strlen(send_data), 0)==-1)
    {
        perror("Failed to send HEAD msg to server\n");
        exit(2); 
    }

    int clen = ParseHeader(sock);
    if(clen < 0)
    {
        printf("some error occured while fetching the file size...aborting...\n");
        exit(1);
    }
    printf("File Size : %d\n", clen);
    //Divide the total length of the file into N parts to be fetched
    int range = clen / N;

    //fork the child processes and fetch the parts in the respective ranges
    for(int i = 1 ; i <= N ; i++)
    {
        pid_t p = fork();
        if(p == 0)
        {
            //execute the child process which fetches data in the range stores it in a temporary file and pushes status to message queue
            if (sem_wait(semaphore_cwc) < 0) 
            {
                perror("sem_wait() failed\n");
            }

            fetch_and_push(range, i);

            if (sem_post(semaphore_cwc) < 0) 
            {
                perror("sem_post() failed\n");
            }
            exit(0);
        }//end if in child
        else
        {
            // wait(NULL);
            continue;
        }
    }//end of main for

    printf("\nIn parent Now Receiving..\n");
    int count = N;
    //Maximum number of retries after which the download stops
    int retries = 20;

    //Check if all segments received correctly; if not resend
    while(count > 0 && retries>0)//keep looping until all segments have been received correctly
    {
        msg ms;
        msgrcv(msgid, &ms, sizeof(msg), 0, 0); 
        printf("Message recieved\n");
        printf("Message type : %d\n", ms.type);
        printf("Message status : %d\n", ms.status);
        printf("Message clen : %d\n", ms.clen);
        
        if(ms.status != 206 && ms.status != 200)
        {
            printf("Retrying for segment %d\n" , ms.type);
            pid_t p = fork();
            if(p == 0)
            {
                if (sem_wait(semaphore_cwc) < 0) 
                {
                    perror("sem_wait() failed\n");
                }

                fetch_and_push(range, ms.type);
                
                if (sem_post(semaphore_cwc) < 0) 
                {
                    perror("sem_post() failed\n");
                }
                exit(0);
            }
            else
            {
                wait(NULL);
                retries--;
                continue;
            }
        }//end if wrong status

        count--;
    }//end while
    
    if(retries == 0)
    {
        printf("Maximum Limit for retries was exceeded...aborting...\n");
        exit(1);
    }
    
    printf("\nSave file name : %s\n", save);

    FILE* fd_main=fopen(save,"wb");
    printf("\nAssembling and Saving data...\n\n");
    //join the data and fwrite it
    char c;
    char open[50];
    for(int i = 0 ; i < N ; i++)
    {
        sprintf(open , "temp%d.%s" , i+1 , ftype);
        printf("%s\n" , open);
        FILE* fd_temp=fopen(open,"rb+");
        
        c = fgetc(fd_temp); 
        while (c != EOF) 
        { 
            fputc(c, fd_main); 
            c = fgetc(fd_temp); 
        }//end while
        fclose(fd_temp);
        
        //Comment this piece of code to let the temporary files created during the download persist
        if(remove(open) != 0)
        {
            printf("Delete Error\n");
        }
    }//end for 

    printf("\nSuccess..\n");
    fclose(fd_main);
    close(sock);
    msgctl(msgid, IPC_RMID, NULL); 
    printf("\n\nDone.\n\n");

    return 0;

}//end main
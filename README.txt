################################

NOTES:
-> Example Usage:
~$ ./concurrent_downloader ocw.mit.edu /ans7870/6/6.006/s08/lecturenotes/files/t8.shakespeare.txt 5 shakespeare.txt
~$ ./concurrent_downloader speedtest.ftp.otenet.gr /files/test1Mb.db 8 1MB.db

-> Type 'make' to create the executables and then type 'make exec' to run a sample download from 

   https://ocw.mit.edu/ans7870/6/6.006/s08/lecturenotes/files/t8.shakespeare.txt

-> The concurrent_web_client.c is a program to download files over the internet using multiple processes.

-> The level of concurrency, i.e., the number of processes to divide the whole download into can be controlled via    
   specifying N in the command line arguments.

-> In the terminal type :
	~$ concurrent_downloader <domain_name> <file_path(on_server)> <N(level_of_concurrency)> <save_file_name>

-> The program works best for downloading text files hosted over a HTTP connnection due to the fact that the assembly of 
   partial files is done in a sequential manner using fwrite.

-> Trying to download pdf,png,jpeg might work at times but not always given the sequential join mechanism which may not 
   work for certain files.

-> The program will not work if the site does not support ranged GET requests.

-> The program may not work if the file is hosted over a HTTPS connection although it does work for certain sites. 

-> Sometimes if a message queue with the same name as used in the program persist, the program may misbehave although 	
   deletion of previous message queue has been implemented in the code itself. In such a case please run destroy_mq.c to clear any message queue with the same name.
   Eg:
   ~$ ./destroy_mq

-> The program has been tested on a few sites that provided sample content. The sites are:
	1)http://25.io/toau/audio/sample.txt
	2)https://ocw.mit.edu/ans7870/6/6.006/s08/lecturenotes/files/t8.shakespeare.txt
	3)http://speedtest.ftp.otenet.gr/files/test100Mb.db
	4)http://speedtest.ftp.otenet.gr/files/test1Mb.db
	5)http://speedtest.ftp.otenet.gr/files/test10Mb.db
	6)http://speedtest.ftp.otenet.gr/files/test100k.db
	7)https://norvig.com/big.txt


################################
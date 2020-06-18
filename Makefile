all : output

exec : concurrent_downloader 
	./concurrent_downloader ocw.mit.edu /ans7870/6/6.006/s08/lecturenotes/files/t8.shakespeare.txt 5 shakespeare.txt

output : concurrent_web_client.c destroy_mq.c 
	gcc -g concurrent_web_client.c -o concurrent_downloader -pthread
	gcc -g destroy_mq.c -o destroy_mq
 
clean : 
	rm concurrent_downloader
	rm destroy_mq

#include <sys/utsname.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <stdio.h>
#include <linux/unistd.h>
#include "includeme.h"

//need to test if 1024 works
#define P2P_MSG_BUFFER_SIZE 1024
#define SUCCESS_MSG_TRY	10



static unsigned long long lego_time_nsec(void)
{
	struct timeval tv;
	time_t t;

	gettimeofday(&tv, NULL);

	unsigned long long nsec=tv.tv_usec*1000;

	return nsec;
}

void getDelay(unsigned long long* us, unsigned long long* ss, unsigned long long*srr ,char* msg, int mlen, long int urr){
	for(int i=0;i<mlen;i++){
		if(msg[i]=='u'&&msg[i+1]=='s'){
			*us=atoll(msg+i+3);
		}
		else if(msg[i]=='s'&&msg[i+1]=='s'){
			*ss=atoll(msg+i+3);
		}
		else if(msg[i]=='s'&&msg[i+1]=='r'&&msg[i+2]=='r'){
			*srr=atoll(msg+i+4);
		}
	}
}

int main(void)
{
	const int TEST_SRC_NID = 0;
	const int TEST_SRC_PID = 14;
	const int TEST_DST_NID = 1;
	const int TEST_DST_PID = 14;

	int my_nid = get_local_nid();
	int my_pid = getpid();

	printf("Performance Test:\n");
	printf("[LOCAL NID]: %d, [LOCAL PID]: %d\n", my_nid, my_pid);


	//SENDER
	if (my_nid == TEST_SRC_NID) {
		// Take a short nap in case the receiver has yet up? Or rely on the failing mechanism?
		// sleep(10);

		printf("[SENDER]: HI I'm Sender NID: %d, PID: %d\n", my_nid, my_pid);
		printf("Start Sending\n");

		int msg_len = P2P_MSG_BUFFER_SIZE;
		char msg[P2P_MSG_BUFFER_SIZE];

		int success_deliver_count = 0;

		int retlen = P2P_MSG_BUFFER_SIZE;
		void * retbuf = malloc(P2P_MSG_BUFFER_SIZE * sizeof(char));

		unsigned long long nsec;

		while (success_deliver_count < SUCCESS_MSG_TRY) {

			memset(msg, 0, P2P_MSG_BUFFER_SIZE);
			memset(retbuf, 0, P2P_MSG_BUFFER_SIZE);

			sprintf(msg,"test perf");
			printf("ITE:%d SENDING: %s\n",success_deliver_count,msg);
			
			nsec=lego_time_nsec();
			sprintf(msg,"us:%d|");
			msg_len=strlen(msg)+1;
			// printf("[SENDER BEFORE SEND:%d]: %s\n", success_deliver_count, msg);
			
			remote_send_reply(TEST_DST_NID, TEST_DST_PID, msg, msg_len, retbuf, retlen);
			
			nsec=lego_time_nsec();



			printf("[SENDER DONE SEND ITE:%d]: %s\n", success_deliver_count, retbuf);
			success_deliver_count += 1;
		}


		printf("[SENDER]: Finish sending! 886\n");
	}

	// RECEIVER
	else {
		printf("[RECEIVER]: HI I'm Receiver NID: %d, PID: %d\n", my_nid, my_pid);

		int success_receive_count = 0;
		char recv_msg[P2P_MSG_BUFFER_SIZE];

		while (success_receive_count < SUCCESS_MSG_TRY) {
			memset(recv_msg, 0, P2P_MSG_BUFFER_SIZE);

			remote_recv(recv_msg, P2P_MSG_BUFFER_SIZE);
			printf("[RECEIVER ITE:%d]: %s\n", success_receive_count, recv_msg);

			success_receive_count += 1;
		}

		printf("[RECEIVER]: Finish receiving! 886\n");

	}
	
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>

int sd;


int reve_packet(){
    unsigned char buf[1024];
	unsigned char *eth_head;
	unsigned char *ip_head;
	int proto;
	unsigned char *p;
    
	int temp = recvfrom(sd,buf,sizeof(buf),0,NULL,NULL);
    if(temp != -1){
		eth_head = buf;
		p = eth_head;
        ip_head = eth_head+14;
		proto = (ip_head + 9)[0];
        printf("%d\n",proto);
		switch(proto){
			case 1:{
            
			printf("MAC address: %.2x:%02x:%02x:%02x:%02x:%02x==> %.2x:%02x:%02x:%02x:%02x:%02x\n",p[6],p[7],p[8],p[9],p[10],p[11],p[0],p[1],p[2],p[3],p[4],p[5]);
			for(int i=0;i<6;i++){
                unsigned char tp;
                tp = p[i];
                p[i] = p[i+6];
                p[i+6]=tp;
            }
            printf("MAC address: %.2x:%02x:%02x:%02x:%02x:%02x==> %.2x:%02x:%02x:%02x:%02x:%02x\n",p[6],p[7],p[8],p[9],p[10],p[11],p[0],p[1],p[2],p[3],p[4],p[5]);

            ip_head = eth_head+14;
			p = ip_head+12;
			printf("IP:%d.%d.%d.%d==> %d.%d.%d.%d\n",
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
            for(int i=0;i<4;i++){
                unsigned char tp;
                tp = p[i];
                p[i] = p[i+4];
                p[i+4] = tp;
            }

            printf("IP:%d.%d.%d.%d==> %d.%d.%d.%d\n",
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
            buf[34] = 0;
            struct sockaddr_ll sll;
            bzero(&sll,sizeof(sll));

            sll.sll_ifindex = if_nametoindex("ens33");
            // printf("%s\n",route_info[i].interface);
            sendto(sd,buf,42,0,
            (struct sockaddr*)&sll,sizeof(sll));
            sleep(0.5);
            break;
            }
			}
			// default:break;
    }
	else{
		printf("-1\n");
		sleep(1);
	}
}

int main(){
    sd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    while(1){
        reve_packet();
        sleep(1);
    }

}
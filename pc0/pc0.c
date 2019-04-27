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

#define MAX_ROUTE_INFO 10
#define MAX_ARP_SIZE 10
#define MAX_DEVICE 10


struct route_item{
    char destination[16];
    char gateway[16];
    char netmask[16];
    char interface[16];
}route_info[MAX_ROUTE_INFO];
// the sum of the items in the route table
int route_item_index=0;

//the informaiton of the " my arp cache"
struct arp_table_item{
    char ip_addr[16];
    char mac_addr[18];
}arp_table[MAX_ARP_SIZE];
// the sum of the items in the arp cache
int arp_item_index =0;

// the storage of the device , got information from configuration file : if.info
struct device_item{
    char interface[14];
    char mac_addr[18];
}device[MAX_DEVICE];
//the sum of the interface
int device_index=0;
char myip[16];
char dstip[16];
int pid=0;
int seq=0;

void init(){
    FILE *fp = fopen("route.txt","r");
    while(feof(fp)==0) 
    {
        fscanf(fp,"%s", route_info[route_item_index].destination);
		fscanf(fp,"%s",route_info[route_item_index].gateway);
		fscanf(fp,"%s",route_info[route_item_index].netmask);
		fscanf(fp,"%s",route_info[route_item_index].interface);
        route_item_index++;
    }
    fclose(fp);

	fp = fopen("arp.txt","r");
	while(!feof(fp)){
		fscanf(fp,"%s",arp_table[arp_item_index].ip_addr);
		fscanf(fp,"%s",arp_table[arp_item_index].mac_addr);
		arp_item_index++;
	}
	fclose(fp);

	fp = fopen("device.txt","r");
	while(!feof(fp)){
		fscanf(fp,"%s",device[device_index].interface);
		fscanf(fp,"%s",device[device_index].mac_addr);
		device_index++;
	}
	fclose(fp);
}

struct ether{
    unsigned char dst[6];
    unsigned char src[6];
    unsigned char protocol[2];
};

unsigned short checksum(unsigned short *buf, int bufsz){
    unsigned long sum = 0xffff;
    while(bufsz > 1){
        sum += *buf;
        buf++;
        bufsz -= 2;
    }
    if(bufsz == 1)
        sum += *(unsigned char*)buf;
    sum = (sum & 0xffff) + (sum >> 16);
    sum = (sum & 0xffff) + (sum >> 16);
    return ~sum;
}

void fill_icmp(struct icmphdr* hdr){
    // 清空 
    memset(hdr, 0, sizeof(*hdr));
    // 初始化 ICMP Header
    hdr->type = 8;
    hdr->code = 0;
    hdr->checksum = 0;
    hdr->un.echo.id = pid & 0xffff;
    hdr->un.echo.sequence = seq++;
    hdr->checksum = checksum((unsigned short*)hdr, sizeof(*hdr));

}

void fill_iphdr(struct iphdr* hdr){
    // struct iphdr temp;
    hdr->version = 0x4;
    hdr->ihl = 0x5;
    hdr->tos = 0;
    hdr->tot_len = sizeof(struct iphdr) + sizeof(struct icmphdr);
    hdr->tot_len >>= 8;
    printf("%ld\n%ld\n",sizeof(struct iphdr),sizeof(struct icmphdr));
    printf("%d\n",hdr->tot_len);
    hdr->id = pid;
    hdr->frag_off = 0;
    hdr->ttl = 128;
    hdr->protocol = 1;
    hdr->check = 0;
    inet_pton(PF_INET, myip, &hdr->saddr);
    inet_pton(PF_INET, dstip, &hdr->daddr);

    hdr->check = checksum((unsigned short*)hdr,sizeof(hdr));
    uint8_t temp = (hdr->check)>>8;
    hdr->check = (hdr->check)<<8 + temp;
}

void fill_ether(struct ether *hdr){
    hdr->dst[0] = 0x00;
    hdr->dst[1] = 0x0c;
    hdr->dst[2] = 0x29;
    hdr->dst[3] = 0x2c;
    hdr->dst[4] = 0x4a;
    hdr->dst[5] = 0x67;

    hdr->src[0] = 0x00;
    hdr->src[1] = 0x0c;
    hdr->src[2] = 0x29;
    hdr->src[3] = 0x08;
    hdr->src[4] = 0x8d;
    hdr->src[5] = 0x20;

    hdr->protocol[0] = 0x08;
    hdr->protocol[1] = 0x00;
}

int sendpacket(int sd,char *port){
    unsigned char buf[1024];
    struct sockaddr_in addr;
    struct icmphdr icmp_hdr;
    struct iphdr ip_hdr;
    struct ether ether_hdr;
    fill_ether(&ether_hdr);
    fill_iphdr(&ip_hdr);
    fill_icmp(&icmp_hdr);
    memcpy(buf,&ether_hdr,sizeof(ether_hdr));
    memcpy(buf+14,&ip_hdr,sizeof(ip_hdr));
    memcpy(buf+34,&icmp_hdr,sizeof(icmp_hdr));
    // int sd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));

    struct sockaddr_ll sll;
	bzero(&sll,sizeof(sll));

	sll.sll_ifindex = if_nametoindex(port);
    int a = sendto(sd,buf, 34+sizeof(icmp_hdr),0,
            (struct sockaddr*)&sll,sizeof(sll));
}

int transmit(){
    unsigned char buf[1024];
	unsigned char *eth_head;
	unsigned char *ip_head;
	int proto;
	unsigned char *p;
    int sd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    while(1){
        /* code */
	int temp = recvfrom(sd,buf,sizeof(buf),0,NULL,NULL);
    if(temp != -1){
		eth_head = buf;
		p = eth_head;
        ip_head = eth_head+14;
		proto = (ip_head + 9)[0];
        // printf("%d\n",proto);
		switch(proto){
			case 1:{
			printf("MAC address: %.2x:%02x:%02x:%02x:%02x:%02x==> %.2x:%02x:%02x:%02x:%02x:%02x\n",p[6],p[7],p[8],p[9],p[10],p[11],p[0],p[1],p[2],p[3],p[4],p[5]);
			ip_head = eth_head+14;
			p = ip_head+12;
			printf("IP:%d.%d.%d.%d==> %d.%d.%d.%d\n",
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
            for(int i=0;i<route_item_index;i++)
            {
                unsigned char s[16];
                sprintf(s,"%d.%d.%d.0",p[4],p[5],p[6]);
                // inet_pton(PF_INET, route_info[i].destination, &temp);
                if(strcmp(s,route_info[i].destination)==0){
                    sendpacket(sd,route_info[i].interface);
                    break;
                }
            }
			}break;
			// default:break;
		}
	}
	else{
		printf("-1\n");
		sleep(1);
	}
    }
}


int main(int argc,char* argv[]){
    strcpy(dstip,argv[1]);
    strcpy(myip,"192.168.2.2");
    pid = getpid();
    unsigned char buf[1024];
    struct sockaddr_in addr;
    struct icmphdr icmp_hdr;
    struct iphdr ip_hdr;
    struct ether ether_hdr;
    fill_ether(&ether_hdr);
    fill_iphdr(&ip_hdr);
    fill_icmp(&icmp_hdr);
    memcpy(buf,&ether_hdr,sizeof(ether_hdr));
    memcpy(buf+14,&ip_hdr,sizeof(ip_hdr));
    memcpy(buf+34,&icmp_hdr,sizeof(icmp_hdr));
    int sd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));

    struct sockaddr_ll sll;
	bzero(&sll,sizeof(sll));

	sll.sll_ifindex = if_nametoindex("ens33");
    while(1){
        int a = sendto(sd,buf, 34+sizeof(icmp_hdr),0,
            (struct sockaddr*)&sll,sizeof(sll));
        sleep(1);
        printf("%d\n",a);
    }
    return 0;
}
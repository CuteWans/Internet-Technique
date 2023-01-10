#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>
#include <windows.h>
#include <winnt.h>

#include "pcap.h"

typedef struct {
  u_char byte[6];
} mac_t;
typedef struct {
  mac_t     daddr;
  mac_t     saddr;
  u_int16_t type;
} etherheader_t;

signed main(int argc, char* argv[]) {
  static char errbuf[PCAP_ERRBUF_SIZE];
  pcap_if_t*  alldevs;
  /* Retrieve the device list */
  if (pcap_findalldevs(&alldevs, errbuf) == -1) {
    fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
    return -1;
  }

  pcap_if_t* d;
  int        i = 0;
  /* Print the list */
  for (d = alldevs; d; d = d->next) {
    printf("%d. %s", ++i, d->name);
    if (d->description) printf(" (%s)\n", d->description);
    else printf(" (No description available)\n");
  }

  if (i == 0) {
    printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
    return -1;
  }

  printf("Enter the interface number (1-%d):", i);
  int inum;
  scanf("%d", &inum);

  if (inum < 1 || inum > i) {
    printf("\nInterface number out of range.\n");
    /* Free the device list */
    pcap_freealldevs(alldevs);
    return -1;
  }

  /* Jump to the selected adapter */
  for (d = alldevs, i = 0; i < inum - 1; d = d->next, i++)
    ;

  pcap_t* adhandle;
  /* Open the adapter */
  if ((adhandle =
         pcap_open_live(d->name,  // name of the device
                        65536,    // portion of the packet to capture.
                                  // 65536 grants that the whole packet will be
                                  // captured on all the MACs.
                        1,      // promiscuous mode (nonzero means promiscuous)
                        1000,   // read timeout
                        errbuf  // error buffer
                        ))
      == NULL) {
    fprintf(stderr,
            "\nUnable to open the adapter. %s is not supported by WinPcap\n",
            d->name);
    /* Free the device list */
    pcap_freealldevs(alldevs);
    return -1;
  }

  printf("\nlistening on %s...\n", d->description);

  /* At this point, we don't need any more the device list. Free it */
  pcap_freealldevs(alldevs);

  int                 res;
  struct pcap_pkthdr* header;
  const u_char*       pkt_data;
  /* Retrieve the packets */
  while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0) { // pkt_data保存以太网帧部分信息
    if (res == 0) continue; /* Timeout elapsed */

    /* convert the timestamp to readable format */
    time_t     local_tv_sec = header->ts.tv_sec;//捕获数据包的时间
    struct tm* ltime        = localtime(&local_tv_sec);//格式转换
    char       timestr[16];
    strftime(timestr, sizeof timestr, "%H:%M:%S", ltime);//格式化时间

    printf("[Timestamp] %s,%.6d\n", timestr, header->ts.tv_usec);//打印时间
    printf("[Capture length] %u\n", header->caplen);//ncap捕获的长度
    printf("[Total length] %u\n", header->len);//数据包总长度
    const etherheader_t* eh = pkt_data;
    printf("[Source MAC] %x:%x:%x:%x:%x:%x\n", eh->saddr.byte[0],
           eh->saddr.byte[1], eh->saddr.byte[2], eh->saddr.byte[3],
           eh->saddr.byte[4], eh->saddr.byte[5]);//saddr.byte储存mac源地址
    printf("[Destination MAC] %x:%x:%x:%x:%x:%x\n", eh->daddr.byte[0],
           eh->daddr.byte[1], eh->daddr.byte[2], eh->daddr.byte[3],
           eh->daddr.byte[4], eh->daddr.byte[5]);//daddr.byte储存mac目的地址
    printf("[EtherType] 0x%04hx\n", ntohs(eh->type));//type为以太类型
    puts("--------");
  }

  if (res == -1) {
    printf("Error reading the packets: %s\n", pcap_geterr(adhandle));
    return -1;
  }

  pcap_close(adhandle);
}

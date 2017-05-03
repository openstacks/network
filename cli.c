#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>

/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000   
#define PORT 60000
/************************************************************************************
 * tun_alloc: Network device allocation                                             *
 * char *dev should be the name of the device with a format string (e.g.            *
 * "tun%d"), but (as far as I can see) this can be any valid network device name.   *
 * Note that the character pointer becomes overwritten with the real device name    *
 * (e.g. "tun0").                                                                   *
 * https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/         *
 * Documentation/networking/tuntap.txt?id=HEAD                                      *  
 ***********************************************************************************/
int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);

  return fd;
}


/**************************************************************************
 * usage: prints usage and exits.                                         *
 **************************************************************************/
void usage(void) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "-i <ifacename> [-r <serverIP>]  [-f <flag>] [-u|-a]\n");
  fprintf(stderr, "-h\n");
  fprintf(stderr, "-i <ifacename>: Name of interface to use (mandatory)\n");
  fprintf(stderr, "-r <serverIP>:  specify server address (-f <serverIP>) (mandatory)\n");
  fprintf(stderr, "-u|-a: use TUN (-u, default) or TAP (-a)\n");
  fprintf(stderr, "-h: prints this help text\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  
  int tap_fd, option;
  int flags = IFF_TUN;
  char if_name[IFNAMSIZ] = "";
  uint16_t nread, nwrite, plength;
  char buffer[BUFSIZE];
  struct sockaddr_in local, remote;
  char remote_ip[16] = "";            /* dotted quad IP string */
  unsigned short int port = PORT;
  int sock_fd, net_fd, optval = 1;


  /* Check command line options */
  while((option = getopt(argc, argv, "r:i:uafh")) > 0) {
    switch(option) {
      case 'h':
        usage();
        break;
      case 'i':
        strncpy(if_name,optarg, IFNAMSIZ-1);
        break;
      case 'r':
        strncpy(remote_ip,optarg,15);
        break;
      case 'u':
        flags = IFF_TUN;
        break;
      case 'a':
        flags = IFF_TAP;
        break;
      default:
        printf("Unknown option %c\n", option);
        usage();
    }
  }

  argv += optind;
  argc -= optind;

  if(argc > 0) {
    printf("Too many options!\n");
    usage();
  }

  if(*if_name == '\0') {
    printf("Must specify interface name!\n");
    usage();
  } 

  /* initialize tun/tap interface */
  if ( (tap_fd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
    printf("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }

  printf("Successfully connected to interface %s\n", if_name);

  if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(1);
  }

    /* Client, try to connect to server */

    /* assign the destination address */
    memset(&remote, 0, sizeof(remote));
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = inet_addr(remote_ip);
    remote.sin_port = htons(port);

    /* connection request */
    if (connect(sock_fd, (struct sockaddr*) &remote, sizeof(remote)) < 0) {
      perror("connect()");
      exit(1);
    }

    net_fd = sock_fd;
    printf("CLIENT: Connected to server %s\n", inet_ntoa(remote.sin_addr));
      

  while(1) {
  
      /* data from tun/tap: just read it and write it to the network */
      
      nread = read(tap_fd, buffer, BUFSIZE);
    
      printf("Read %d bytes from the tap interface\n", nread);

      /* write length + packet */
      plength = htons(nread);
      nwrite = write(net_fd, (char *)&plength, sizeof(plength));
      nwrite = write(net_fd, buffer, nread);
      
      printf("Written %d bytes to the network\n", nwrite);
      
   
      /* data from the network: read it, and write it to the tun/tap interface. 
       * We need to read the length first, and then the packet */

      /* Read length */      
      nread = read(net_fd, (char *)&plength, sizeof(plength));
      if(nread == 0) {
        /* ctrl-c at the other end */
        break;
      }

      /* read packet */
      nread = read(net_fd, buffer, ntohs(plength));
      printf("Read %d bytes from the network\n", nread);

      /* now buffer[] contains a full packet or frame, write it into the tun/tap interface */ 
      nwrite = write(tap_fd, buffer, nread);
      printf("Written %d bytes to the tap interface\n", nwrite);
    
  }
  
  return(0);
}
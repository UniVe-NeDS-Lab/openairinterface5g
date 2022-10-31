
#include "o1.h"

int connect_socket()
{
  int sockfd = 0;
  struct sockaddr_un saddr;
  memset(&saddr, 0, sizeof(struct sockaddr_un));

  saddr.sun_family = AF_UNIX;
  strcpy(saddr.sun_path, O1_SOCKET_PATH);
  socklen_t saddrlen = sizeof(saddr.sun_family) + strlen(saddr.sun_path);

  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    LOG_E(NR_MAC, "Could not create socket\n");
    return 0;
  }
  if (connect(sockfd, &saddr, saddrlen) < 0) {
    LOG_E(NR_MAC, "Connect Failed\n");
    perror("Errno: \n");
    return 0;
  }

  return sockfd;
}

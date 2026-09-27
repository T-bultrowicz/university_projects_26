#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "cb.h"
#include "err.h"
#include "common.h"

#define BUFFER_SIZE     16384
#define MAX_HOSTNAME_LEN  255
#define MAX_PORT_LEN      5

char *progName = NULL;

static void helpAndExit()
{
  fatal("invocation: %s UDPaddress UDPport TCPport", progName);
}

struct sockaddr_and_len {
  socklen_t addr_len;
  struct sockaddr_storage addr;
};

static struct sockaddr_and_len _get_first_matching_addr(const char *hostStr,
                                                        const char *portStr,
                                                        int family, int flags)
{
  struct addrinfo *ai;
  struct addrinfo hints = {.ai_family = family, .ai_socktype = 0,
                           .ai_protocol = 0, .ai_flags = flags};

  int gai_error = getaddrinfo(hostStr, portStr, &hints, &ai);
  if (gai_error != 0) {
    fprintf(stderr, "getaddrinfo failed: %s", gai_strerror(gai_error));
    helpAndExit();
  }

  struct sockaddr_and_len ret;
  ret.addr_len = ai->ai_addrlen;
  *(struct sockaddr_in *)&(ret.addr) = *(struct sockaddr_in *)(ai->ai_addr);
  freeaddrinfo(ai);

  return ret;
}

static struct sockaddr_and_len parseHostAndPort(const char *hostStr, const char *portStr)
{
  return _get_first_matching_addr(hostStr, portStr, AF_INET, 0);
}

static struct sockaddr_and_len getListeningAddress(const char *portStr)
{
  return _get_first_matching_addr(NULL, portStr, AF_INET, AI_PASSIVE);
}

// Parametry: adres i port UDP są do rozgłaszania i słuchania,
// a port tcp do nasłuchiwania.
int main(int argc, char *argv[])
{
  progName = argv[0];
  if (argc != 4) {
    helpAndExit();
  }

  struct sockaddr_and_len udp_sending_address = parseHostAndPort(argv[1], argv[2]);
  struct sockaddr_and_len udp_listening_address = getListeningAddress(argv[2]);
  struct sockaddr_and_len tcp_address = getListeningAddress(argv[3]);

  struct sockaddr_in *udp_sending_addr_in = (struct sockaddr_in *)&(udp_sending_address.addr);
  if (!IN_MULTICAST(ntohl(udp_sending_addr_in->sin_addr.s_addr))) {
    fatal("Provided UDP address is not a multicast address.\n");
  }

  int tcp_listen_sock;
  if ((tcp_listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    syserr("socket");
  }

  if (bind(tcp_listen_sock, (struct sockaddr *)&(tcp_address.addr), tcp_address.addr_len) < 0) {
    syserr("bind");
  }

  if (listen(tcp_listen_sock, 1) < 0) {
    syserr("listen");
  }

  struct sockaddr_storage peer;
  socklen_t peer_len = sizeof peer;
  int tcp_sock;
  if ((tcp_sock = accept(tcp_listen_sock, (struct sockaddr *)&peer, &peer_len)) < 0) {
    syserr("accept");
  }

  char peer_addr_str[MAX_HOSTNAME_LEN + 1];
  char peer_port_str[MAX_PORT_LEN + 1];
  int gai_error = getnameinfo((struct sockaddr *)&peer, peer_len, peer_addr_str, (socklen_t)(sizeof peer_addr_str),
                              peer_port_str, (socklen_t)(sizeof peer_port_str), NI_NUMERICHOST | NI_NUMERICSERV);
  if (gai_error != 0) {
    fprintf(stderr, "getnameinfo failed: %s", gai_strerror(gai_error));
    helpAndExit();
  }
  fprintf(stderr, "TCP connection to %s:%s established.\n", peer_addr_str, peer_port_str);

  close(tcp_listen_sock);

  // TU TRZEBA DOPISAĆ KOD.
  int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_sock < 0) {
    syserr("socket");
  }

  int optval = 1;
  if (setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval) < 0) {
    syserr("setsockopt");
  }

  if (bind(udp_sock, (struct sockaddr *)&(udp_listening_address.addr), udp_listening_address.addr_len) < 0) {
    syserr("bind");
  }

  struct ip_mreq ipmreq;
  struct sockaddr_in * temp_address = (struct sockaddr_in *) &udp_sending_address.addr;
  ipmreq.imr_multiaddr = temp_address->sin_addr;
  ipmreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if (setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipmreq, sizeof ipmreq) < 0) {
    syserr("setsockopt");
  }

  /* Uncomment if we don't want to hear our own messages. */
  // optval = 0;
  // if (setsockopt(udp_sock, IPPROTO_IP, IP_MULTICAST_LOOP, &optval, sizeof optval) < 0) {
  //   syserr("setsockopt");
  // }

  struct pollfd fds[2] = {{tcp_sock, POLLOUT | POLLIN, 0}, {udp_sock, POLLOUT | POLLIN, 0}};
  // all needed buffors
  CircularBuffer data_to_send_tcp, data_to_send_udp;
  cbInit(&data_to_send_tcp); cbInit(&data_to_send_udp);
  char buffer[BUFFER_SIZE];

  // main loop
  bool keep_running = true;
  while (keep_running) {
    size_t data_tcp_size = cbGetContinuousCount(&data_to_send_tcp);
    size_t data_udp_size = cbGetContinuousCount(&data_to_send_udp);

    fds[0].events = (data_tcp_size > 0 ? POLLOUT : 0) | POLLIN;
    fds[1].events = (data_udp_size > 0 ? POLLOUT : 0) | POLLIN;
    fds[0].revents = 0; fds[1].revents = 0;
    if (poll(fds, 2, -1) < 0) {
      syserr("poll");
    }

    if ((fds[0].revents & POLLOUT) != 0) {
      ssize_t send_len = write(fds[0].fd, cbGetData(&data_to_send_tcp), data_tcp_size);
      if (send_len < 0) {
        error("write");
      } else {
        fprintf(stdout, "Sent %zd bytes to TCP peer.\n", send_len);
        cbDropFront(&data_to_send_tcp, (size_t) send_len);
      }
    }
    if ((fds[1].revents & POLLOUT) != 0) {
      ssize_t send_len = sendto(fds[1].fd, cbGetData(&data_to_send_udp), data_udp_size,
            0, (struct sockaddr *) &udp_sending_address.addr, udp_sending_address.addr_len);
      if (send_len < 0) {
        error("sendto");
      } else if ((size_t) send_len != data_udp_size) {
        fatal("Send incomplete message!");
      } else {
        fprintf(stdout, "Sent %zd bytes via multicast.\n", send_len);
        cbDropFront(&data_to_send_udp, (size_t) send_len);
      }
    }

    if ((fds[0].revents & POLLIN) != 0) {
      memset(buffer, 0, sizeof buffer);
      ssize_t recv_len = read(fds[0].fd, buffer, sizeof buffer);

      if (recv_len < 0) {
        error("read");
      } else if (recv_len == 0) {
        fprintf(stdout, "TCP connection closed by peer.\n");
        keep_running = false;
      } else {
        fprintf(stdout, "Received %zd bytes from TCP peer.\n", recv_len);
        cbPushBack(&data_to_send_udp, buffer, (size_t) recv_len);
      }
    }

    if (keep_running && (fds[1].revents & POLLIN) != 0) {
      memset(buffer, 0, sizeof buffer);
      ssize_t recv_len = recv(fds[1].fd, buffer, sizeof buffer, 0);

      if (recv_len < 0) {
        error("recv");
      } else {
        fprintf(stdout, "Received %zd bytes via multicast.\n", recv_len);
        cbPushBack(&data_to_send_tcp, buffer, (size_t) recv_len);
      }
    }

  }

  close(udp_sock);
  close(tcp_sock);

  return 0;
}

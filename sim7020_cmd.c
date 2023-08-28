/*
 * Copyright (C) 2020 Peter Sjödin, KTH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "net/af.h"
#include "net/ipv4/addr.h"
#include "net/ipv6/addr.h"
#include "net/sock/udp.h"

#include "periph/uart.h"

#include "net/sim7020.h"
#include "mqttsn_publisher.h" // added

int sim7020cmd_init(int argc, char **argv) {
  
  (void) argc; (void) argv;

  //int res = sim7020_init(UART_DEV(1), 9600);
  int res = sim7020_init();  
  if (res < 0)
    printf("Error %d\n", res);
  else
    printf("OK");
  return res;
}

int sim7020cmd_register(int argc, char **argv) {
  
  (void) argc; (void) argv;

  int res = sim7020_register();
  if (res < 0)
    printf("Error %d\n", res);
  else
    printf("OK");
  return res;
}

int sim7020cmd_activate(int argc, char **argv) {
  
  (void) argc; (void) argv;

  int res = sim7020_activate();
  if (res < 0)
    printf("Error %d\n", res);
  else
    printf("OK");
  return res;
}
  
int sim7020cmd_status(int argc, char **argv) {
  
  (void) argc; (void) argv;

  int res = sim7020_status();
  if (res < 0)
    printf("Error %d\n", res);
  else
    printf("OK");
  return res;
}

int sim7020cmd_stats(int argc, char **argv) {
  
  (void) argc; (void) argv;
  sim7020_netstats_t *ns = sim7020_get_netstats();

  printf("tx_unicast_count: %" PRIu32 "\n", ns->tx_unicast_count);
  printf("tx_mcast_count: %" PRIu32 "\n", ns->tx_mcast_count);
  printf("tx_success: %" PRIu32 "\n", ns->tx_success);
  printf("tx_failed: %" PRIu32 "\n", ns->tx_failed);
  printf("tx_bytes: %" PRIu32 "\n", ns->tx_bytes);
  printf("rx_count: %" PRIu32 "\n", ns->rx_count);
  printf("rx_bytes: %" PRIu32 "\n", ns->rx_bytes);
  printf("commfail_count: %" PRIu32 "\n", ns->commfail_count);
  printf("reset_count: %" PRIu32 "\n", ns->reset_count);
  printf("activation_count: %" PRIu32 "\n", ns->activation_count);
  printf("activation_fail_count: %" PRIu32 "\n", ns->activation_fail_count);
  {
    extern uint32_t sim7020_activation_usecs;
    if (sim7020_active()) {
      printf("activation_time: %" PRIu32 "\n", sim7020_activation_usecs/1000);
    }
  }
  {
    extern uint64_t sim7020_prev_active_duration_usecs;
    if (sim7020_prev_active_duration_usecs != 0) {
      printf("prev_duration: %" PRIu32 "\n", (uint32_t) (sim7020_prev_active_duration_usecs/1000));
    }
  }
  extern uint32_t longest_send;
  printf("Longest send %" PRIu32 "\n", longest_send);

  return 0;
}

static void recv_callback(void *p, const uint8_t *data, uint16_t datalen) {
    (void) p; (void) data;
    printf("recv_callback: got %d bytes\n", datalen);
}

int sim7020cmd_udp_socket(int argc, char **argv) {
  
(void) argc; (void) argv;

  int res = sim7020_udp_socket(recv_callback, NULL);
  if (res < 0)
    printf("Error %d\n", res);
  else
      printf("Socket %d", res);
  return res;
}

int sim7020cmd_close(int argc, char **argv) {
  uint8_t sockid;
  
  if (argc != 2) {
    printf("Usage: %s sockid\n", argv[0]);
    return 1;
  }
  sockid = atoi(argv[1]);
  int res = sim7020_close(sockid);
  if (res < 0)
    printf("Error %d\n", res);
  else
    printf("OK");
  return res;
}

int sim7020cmd_connect(int argc, char **argv) {
    uint8_t sockid;
    sock_udp_ep_t remote;

  
    if (argc < 4) {
        printf("Usage: %s sockid ipaddr port\n", argv[0]);
        return 1;
    }
    sockid = atoi(argv[1]);

    remote.family = AF_INET6;
    ipv6_addr_t *v6addr = (ipv6_addr_t *) remote.addr.ipv6;
    /* Build ipv4-mapped address */
    v6addr->u32[0].u32 = 0; v6addr->u32[1].u32 = 0;
    v6addr->u16[4].u16 = 0; v6addr->u16[5].u16 = 0xffff;
    if (NULL == ipv4_addr_from_str((ipv4_addr_t *) &v6addr->u32[3], argv[2])) {
        printf("Usage: %s sockid ipaddr port\n", argv[0]);
        return 1;
    }
    remote.netif = SOCK_ADDR_ANY_NETIF;
    remote.port = atoi(argv[3]);
    
    int res = sim7020_connect(sockid, &remote);
    if (res < 0)
        printf("Error %d\n", res);
    else
        printf("OK");
    return res;
}

int sim7020cmd_send(int argc, char **argv) {
  uint8_t sockid;
  char *data;
  
  if (argc < 3) {
    printf("Usage: %s sockid data\n", argv[0]);
    return 1;
  }
  sockid = atoi(argv[1]);
  data = argv[2];
  int res = sim7020_send(sockid, (uint8_t *) data, strlen(data));
  if (res < 0)
    printf("Error %d\n", res);
  else
    printf("OK");
  return res;
}

/**
 * test code for invoking one-stop publishing by calling mqpub_pubtopic(). Code from Frej & Axel
*/

int sim7020cmd_topic(int argc, char **argv) {
  printf("argv[1]: %s\nargv[2]: %s\nargv[3]: %s\n", argv[1], argv[2], argv[3]);
  printf("atoi: %d", atoi(argv[3]));

  if (argc < 4) {
      printf("Usage: %s topicstr data len\n", argv[0]);
      return 1;
  }

  return mqpub_pubtopic(argv[1], (uint8_t*) argv[2], atoi(argv[3]));
}


int sim7020_test(void);
int sim7020cmd_test(int argc, char **argv) {
  (void) argc; (void) argv;

  int res = sim7020_test();
  if (res < 0)
    printf("Error %d\n", res);
  else
    printf("OK");
  return res;
}

int sim7020cmd_at(int argc, char **argv) {
    char cmd[256];
  
    uint8_t first = 1;
    int i;
    cmd[0] = '\0';
    for (i = 1; i < argc; i++) {
        if (first != 1) {
            (void) strlcat(cmd, " ", sizeof(cmd));
        }
        else
            first = 0;
        (void) strlcat(cmd, argv[i], sizeof(cmd));
    }
    return sim7020_at(cmd);
}

int sim7020cmd_reset(int argc, char **argv) {
    (void) argc; (void) argv;
    return sim7020_reset();
}

int sim7020cmd_stop(int argc, char **argv) {
    (void) argc; (void) argv;
    return sim7020_stop();
}

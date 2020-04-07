/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Example application for demonstrating RIOT's MQTT-SN library
 *              emCute
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"

#include "mqtt_publisher.h"
#include "at24mac.h"

#define MQPUB_PRIO         (THREAD_PRIORITY_MAIN - 1)

#define NUMOFSUBS           (16U)
#define TOPIC_MAXLEN        (64U)

#define MQPUB_TOPIC_LENGTH  64

/* State machine interval in secs */
#define MQPUB_STATE_INTERVAL 2

int rpl_report(uint8_t *buf, size_t len);

typedef enum {
    MQTTSN_NOT_CONNECTED,
    MQTTSN_CONNECTED,
    MQTTSN_PUBLISHING,
} mqttsn_state;

static char stack[THREAD_STACKSIZE_DEFAULT];

static char topicstr[MQPUB_TOPIC_LENGTH];
emcute_topic_t emcute_topic;

static int get_nodeid(char *buf, size_t size) {
    int n = 0;
    eui64_t e64;

    if (at24mac_get_eui64(0, &e64) != 0) {
        printf("Can't get nodeid\n");
        return 0;
    }
    for (unsigned i = 0; i < sizeof(e64.uint8); i++) {
      n += snprintf(buf + n, size - n, "%02x", e64.uint8[i]);
    }
    return n;
}

static void _init_topic(void) {
  
    //static char nodeid[sizeof(e64.uint8)*2+1];
    int n; 

    n = snprintf(topicstr, sizeof(topicstr), "%s/", MQTT_TOPIC_BASE);
    n += get_nodeid(topicstr + n, sizeof(topicstr) - n);
    n += snprintf(topicstr + n, sizeof(topicstr) - n, "/sensor");
}

static int seq_nr_value = 0;

size_t publish_data(uint8_t *buffer, size_t len) {
    char *str = (char *) buffer;
    size_t n = 0;
    n += snprintf(str + n, len - n, "[");

    n += snprintf(str + n, len - n, "{\"bn\":\"urn:dev:mac:");
    n += get_nodeid(str + n, len - n);
    n += snprintf(str + n, len - n, ";\"");    

    n += snprintf(str + n, len - n, ",\"bu\":\"count\",\"bt\":%lu}", (uint32_t) (xtimer_now_usec()/1000000));

    n += snprintf(str + n, len - n, ",{\"n\":\"seq_no\",\"u\":\"count\",\"v\":%d}", 9000+seq_nr_value++);

    n += rpl_report((uint8_t *) str + n, len - n);
    n += snprintf(str + n, len - n, "]");
    return n;
}

uint8_t publish_buffer[MQTTSN_BUFFER_SIZE];

static int mqpub_pub(void) {
    unsigned flags = EMCUTE_QOS_0;
    size_t publen;

    publen = publish_data(publish_buffer, sizeof(publish_buffer));
    printf("Publish %d: \"%s\"\n", publen, publish_buffer);
    if (emcute_pub(&emcute_topic, publish_buffer, publen, flags) != EMCUTE_OK) {
        printf("error: unable to publish data to topic '%s [%i]'\n",
               emcute_topic.name, (int)emcute_topic.id);
        return 1;
    }
    return 0;
    
}
static int mqpub_con(void) {
    sock_udp_ep_t gw = { .family = AF_INET6, .port = MQTTSN_GATEWAY_PORT };

    printf("Conneect\n");

    /* parse address */
    if (ipv6_addr_from_str((ipv6_addr_t *)&gw.addr.ipv6, MQTTSN_GATEWAY_HOST) == NULL) {
        printf("error parsing IPv6 address\n");
        return 1;
    }
    if (emcute_con(&gw, true, NULL, NULL, 0, 0) != EMCUTE_OK) {
        printf("error: unable to connect to gateway [%s]:%d\n", MQTTSN_GATEWAY_HOST, MQTTSN_GATEWAY_PORT);
        return 1;
    }
    printf("Successfully connected to gateway [%s]:%d\n", MQTTSN_GATEWAY_HOST, MQTTSN_GATEWAY_PORT);
    return 0;
}

static int mqpub_reg(void) {
    emcute_topic.name = topicstr;
    if (emcute_reg(&emcute_topic) != EMCUTE_OK) {
        printf("error: unable to obtain topic ID for \"%s\"\n", topicstr);
        return 1;
    }
    printf("Obtained topic ID %d for \"%s\"\n", (int)emcute_topic.id, topicstr);
    return 0;
}

static mqttsn_state state;

static void *mqpub_thread(void *arg)
{
    uint32_t sleepsecs;
    (void)arg;
    
    printf("Here is mqtt_publisher thread\n");
 again:
    state = MQTTSN_NOT_CONNECTED;
    sleepsecs = MQPUB_STATE_INTERVAL;
    while (1) {
      switch (state) {
      case MQTTSN_NOT_CONNECTED:
        if (mqpub_con() == 0)
          state = MQTTSN_CONNECTED;
        break;
      case MQTTSN_CONNECTED:
        if (mqpub_reg() == 0) {
          state = MQTTSN_PUBLISHING;
        }
        break;
      case MQTTSN_PUBLISHING:
        if (mqpub_pub() != 0) {
          state = MQTTSN_NOT_CONNECTED;
          goto again;
        }
        sleepsecs = MQTTSN_PUBLISH_INTERVAL;
        break;
      default:
        printf("Do nothin in state %d\n", state);
        break;
      }
      xtimer_sleep(sleepsecs);
    }
    return NULL;    /* should never be reached */
}

void mqtt_publisher_init(void) {
    _init_topic();
    /* start publisher thread */
    thread_create(stack, sizeof(stack), MQPUB_PRIO, 0,
                  mqpub_thread, NULL, "emcute");
}

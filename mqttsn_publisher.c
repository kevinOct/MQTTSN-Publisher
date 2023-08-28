/*
 * Copyright (C) 2020 Peter Sjödin, KTH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "mbox.h"
#include "msg.h"
#include "net/emcute.h"
#include "net/ipv6/addr.h"
#if defined(MODULE_SOCK_DNS)
#include "net/sock/dns.h"
#else
#include "net/sock/dns.h"
#endif
#include "xtimer.h"

#include "at24mac.h"

#include "mqttsn_publisher.h"
#include "report.h"
#ifdef SNTP_SYNC
#include "sync_timestamp.h"
#endif /* SNTP_SYNC */

#ifdef APP_WATCHDOG
#include "app_watchdog.h"
#endif /* APP_WATCHDOG */

#include "dns_resolve.h"

#ifdef MODULE_SIM7020
#include "net/sim7020.h"
#endif /* MODULE_SIM7020 */

#ifdef BOARD_AVR_RSS2
#include "pstr_print.h"
#endif
#define EMCUTE_PORT         (1883U)
#define EMCUTE_PRIO         (THREAD_PRIORITY_MAIN + 1)

#define MQPUB_PRIO         (THREAD_PRIORITY_MAIN + 1)

#define NUMOFSUBS           (16U)

#define LEDON LED1_ON
#define LEDOFF LED1_OFF

/* State machine interval in secs */
#define MQPUB_STATE_INTERVAL 2
/* Interval between DNS lookup attempts */
#define MQPUB_RESOLVE_INTERVAL 30

mqttsn_stats_t mqttsn_stats;

#ifdef MQTTSN_PUBLISHER_THREAD
static char mqpub_stack[THREAD_STACKSIZE_DEFAULT + 384];
#endif
static char emcute_stack[THREAD_STACKSIZE_DEFAULT + 128];

static char default_topicstr[MQPUB_TOPIC_LENGTH];
static char default_basename[MQPUB_BASENAME_LENGTH];
emcute_topic_t emcute_topic;
static emcute_sub_t *subscriptions[MQTTSN_MAX_SUBSCRIPTIONS];

static int client_id(char *id, int idlen, char *prefix) {

    char nid[sizeof(eui64_t)*2+1]; /* EUI-64 in hex + NULL */
    get_nodeid(nid, sizeof(nid));
    assert(strlen(nid) > 12);
    int n = snprintf(id, idlen, "%s%s", prefix != NULL ? prefix : "", &nid[12]);
    return n;
}
static void *emcute_thread(void *arg)
{
    char *prefix = (char *) arg;
    char cli_id[24];

    client_id(cli_id, sizeof(cli_id), prefix);
    emcute_run(EMCUTE_PORT, cli_id);
    return NULL;    /* should never be reached */
}

int get_nodeid(char *buf, size_t size) {
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

static mqpub_topic_t mqpub_topics[MQTTSN_MAX_TOPICS];

static inline void _reset_topic(mqpub_topic_t *topic) {
    topic->name = NULL;
}

static void _init_topics(void) {
    mqpub_topic_t *topic, *last;

    last = &mqpub_topics[MQTTSN_MAX_TOPICS-1];
    for (topic = mqpub_topics; topic < last; topic++)
        _reset_topic(topic);
}

static mqpub_topic_t *_alloc_topic(const char *topicstr) {
    mqpub_topic_t *topic, *last;

    last = &mqpub_topics[MQTTSN_MAX_TOPICS-1];
    for (topic = mqpub_topics; topic < last; topic++)
        if (topic->name == NULL) {
            topic->name = topicstr;
            return topic;
        }
    return NULL;
}

static mqpub_topic_t *_lookup_topic(const char *topicstr) {
    mqpub_topic_t *topic, *last;

    last = &mqpub_topics[MQTTSN_MAX_TOPICS-1];
    for (topic = mqpub_topics; topic < last; topic++)
        if (topic->name != NULL && 
            strncmp(topic->name, topicstr, sizeof(topic->name)) == 0)
            return topic;
    return NULL;
}


size_t mqpub_init_topic(char *topic, size_t topiclen, char *nodeid, char *suffix) {
    char *buf = topic;
    size_t len = topiclen;
    int n;
    
    n = snprintf(buf, len, "%s/%s", MQTT_TOPIC_BASE, nodeid);
    if (suffix != NULL) {
        n = strlcat(buf, suffix, len);
    }
    return n;
}

static void _init_default_topicstr(void) {
    char nodeidstr[20];
    (void) get_nodeid(nodeidstr, sizeof(nodeidstr));
    mqpub_init_topic(default_topicstr, sizeof(default_topicstr), nodeidstr, "/sensors");
}

size_t mqpub_init_basename(char *basename, size_t basenamelen, char *nodeid) {
    char *buf = basename;
    size_t len = basenamelen;
    int n;
    
    n = snprintf(buf, len, MQPUB_BASENAME_FMT, nodeid);
    return n;
}

static void _init_default_basename(void) {
    char nodeidstr[20];
    (void) get_nodeid(nodeidstr, sizeof(nodeidstr));
    (void) mqpub_init_basename(default_basename, sizeof(default_basename), nodeidstr);
}

void mqpub_init(void) {
    _init_topics();
}

uint8_t publish_buffer[MQTTSN_BUFFER_SIZE];

int mqpub_pub(mqpub_topic_t *topic, void *data, size_t len) {
    unsigned flags = EMCUTE_QOS_1;
    int errno;
    
    printf("mqpub: publish  %d to %s: \"%s\"\n", len, topic->name, (char *) data);

    LEDON;
    if ((errno = emcute_pub((emcute_topic_t *) topic, data, len, flags)) != EMCUTE_OK) {
        printf("\n\nerror: unable to publish data to topic '%s [%i]' (error %d)\n",
               topic->name, (int)topic->id, errno);
        mqttsn_stats.publish_fail += 1;
    }
    else {
        mqttsn_stats.publish_ok += 1;
    }
    LEDOFF;
#ifdef APP_WATCHDOG
    app_watchdog_update(errno == EMCUTE_OK);
#endif /* APP_WATCHDOG */
    return errno;
}

static int _resolve_v6addr(char *host, ipv6_addr_t *result) {
    return dns_resolve_inetaddr(host, result);
}

static char mqttsn_gateway_host[] = MQTTSN_GATEWAY_HOST;

int mqpub_con(char *host, uint16_t port) {
    sock_udp_ep_t gw = { .family = AF_INET6, .port = port};
    int errno;
    
    /* parse address */
    if ((errno = _resolve_v6addr(mqttsn_gateway_host, (ipv6_addr_t *) &gw.addr.ipv6)) < 0)
        return errno;
    printf("mqpub: Connect to [");
    ipv6_addr_print((ipv6_addr_t *) &gw.addr.ipv6);
    printf("]:%d\n", gw.port);
    LEDON;
    if ((errno = emcute_con(&gw, true, NULL, NULL, 0, 0)) != EMCUTE_OK) {
        printf("error: unable to connect to gateway [%s]:%d (error %d)\n", host, port, errno);
        mqttsn_stats.connect_fail += 1;
    }
    else {
        printf("MQTT-SN: Connect to gateway [%s]:%d\n", host, port);
        mqttsn_stats.connect_ok += 1;
    }
    LEDOFF;
#ifdef APP_WATCHDOG
    app_watchdog_update(errno == EMCUTE_OK);
#endif /* APP_WATCHDOG */
    return errno;
}

int mqpub_reg(mqpub_topic_t *topic, char *topicstr) {
    int errno;
    topic->name = topicstr;
    if ((errno = emcute_reg((emcute_topic_t *) topic)) != EMCUTE_OK) {
        mqttsn_stats.register_fail += 1;
        printf("error: unable to obtain topic ID for \"%s\" (error %d)\n", topicstr, errno);
    }
    else {
        printf("Register topic %d for \"%s\"\n", (int)topic->id, topicstr);
        mqttsn_stats.register_ok += 1;
    }
#ifdef APP_WATCHDOG
    app_watchdog_update(errno == EMCUTE_OK);
#endif /* APP_WATCHDOG */
    return errno;
}

mqpub_topic_t *mqpub_reg_topic(char *topicstr) {
    mqpub_topic_t *tp;
    int errno;
    if ((tp = _lookup_topic(topicstr)) != NULL)
        return tp;
    if ((tp = _alloc_topic(topicstr)) == NULL) {
        return NULL;
    }
    printf("mqpub: register %s\n", tp->name);
    LEDON;
    if ((errno = emcute_reg((emcute_topic_t *) tp)) != EMCUTE_OK) {
        mqttsn_stats.register_fail += 1;
        printf("error: unable to obtain topic ID for \"%s\" (error %d)\n", tp->name, errno);
        _reset_topic(tp);
        tp = NULL;
    }
    else {
        printf("Register topic %d for \"%s\"\n", (int)tp->id, tp->name);
        mqttsn_stats.register_ok += 1;
    }
    LEDOFF;
#ifdef APP_WATCHDOG
    app_watchdog_update(errno == EMCUTE_OK);
#endif /* APP_WATCHDOG */
    return tp;
}

int mqpub_start_subscription(char *topic, emcute_cb_t cb) {
    emcute_sub_t *sub = malloc(sizeof(emcute_sub_t));
    if (sub == NULL)
        return ENOMEM;
    int i;
    for (i = 0; i < MQTTSN_MAX_SUBSCRIPTIONS; i++) {
        if (subscriptions[i] == NULL) {
            subscriptions[i] = sub;
            sub->topic.name = topic;
            sub->cb = cb;
            return 0;
        }
    }
    return ENOMEM;
}


int mqpub_discon(void) {
    LEDON;
    int errno = emcute_discon();
    LEDOFF;
#ifdef APP_WATCHDOG
    app_watchdog_update(errno == 0);
#endif /* APP_WATCHDOG */
    return errno;
}

int mqpub_reset(void) {
    LEDON;
    int errno = mqpub_discon();
    LEDOFF;
    mqttsn_stats.reset += 1;
    if (errno != EMCUTE_OK) {
        printf("MQTT-SN: disconnect failed %d\n", errno);
    }
#ifdef APP_WATCHDOG
    app_watchdog_update(errno == 0);
#endif /* APP_WATCHDOG */
    return errno;
}

/*
 * One-stop publishing -- connect, register topic, publish and disconnect
 */
int mqpub_pubtopic(char *topicstr, uint8_t *data, size_t datalen) {
    int res;
    mqpub_topic_t topic;

    res = mqpub_con(MQTTSN_GATEWAY_HOST, MQTTSN_GATEWAY_PORT);
    printf("*con %d\n", res);
    if (res != 0)
        return res;
    res = mqpub_reg(&topic, topicstr);
    printf("*reg %d\n", res);
    if (res != 0)
        return res;
    res = mqpub_pub(&topic, data, datalen);
    printf("*pub %d\n", res);
    mqpub_reset();
    return res;
}

#ifdef MQTTSN_PUBLISHER_THREAD

enum {
  MSG_EVT_ASYNC,
  MSG_EVT_PERIODIC,
};

#define EVT_QUEUE_SIZE 4
static msg_t evt_msg_queue[EVT_QUEUE_SIZE];
static mbox_t evt_mbox = MBOX_INIT(evt_msg_queue, EVT_QUEUE_SIZE);

static void _periodic_callback(void *arg)
{
    msg_t msg = { .type = MSG_EVT_PERIODIC };
    mbox_t *mbox = arg;
    mbox_put(mbox, &msg);
}

void mqpub_report_ready(void) {
    msg_t msg = { .type = MSG_EVT_ASYNC };
    mbox_t *mbox = &evt_mbox;
    mbox_put(mbox, &msg);
}

static mqttsn_state_t state = MQTTSN_NOT_CONNECTED;

static void _publish_all(int subscribe) {
    uint32_t linger = 0;
again:
    while (1) {
        switch (state) {
        case MQTTSN_NOT_CONNECTED:
            mqpub_init();
            int res = mqpub_con(MQTTSN_GATEWAY_HOST, MQTTSN_GATEWAY_PORT);
            //printf("mqpub_connect: %d\n", res);
            if (res != 0)
                break;
            state = MQTTSN_CONNECTED;
            /* fall through */
        case MQTTSN_CONNECTED:
            /* Now we check for each publish if the topic needs
             * registration, so nothing to do here anymore.
             */
            state = MQTTSN_PUBLISHING;
            /* fall through */
        case MQTTSN_PUBLISHING:
        {
            static uint8_t finished;
            do {
                size_t publen;
                mqpub_topic_t *tp;
                char *topicstr = default_topicstr;
                char *basename = default_basename;

                publen = makereport(publish_buffer, sizeof(publish_buffer), &finished, &topicstr, &basename);
                //  if (i != 3)
                //  {
                //      puts("current buffer:");
                //      printf("current report length: %d\n", publen);
                //      for (size_t i = 0; i < sizeof(publish_buffer); i++)
                //      {
                //          printf("%c", publish_buffer[i]);
                //      }
                //      printf("\n");

                //     i++;
                //     goto again;
                // }
                if ((tp = mqpub_reg_topic(topicstr)) == NULL) {
                    mqpub_reset();
                    state = MQTTSN_NOT_CONNECTED;
                    goto again;
                }
                if (mqpub_pub(tp, publish_buffer, publen) != 0) {
                    mqpub_reset();
                    state = MQTTSN_NOT_CONNECTED;
                    goto again;
                }

            } while (!finished);
            if (subscribe) {
                emcute_sub_t **sub;
                for (sub = &subscriptions[0]; sub <= &subscriptions[MQTTSN_MAX_SUBSCRIPTIONS-1]; sub++) {
                    if (*sub) {
                        if ((res = emcute_sub(*sub, EMCUTE_QOS_1)) < 0) {
                            mqpub_reset();
                            state = MQTTSN_NOT_CONNECTED;
                            return;
                        }
                    }
                }
                linger = xtimer_now_usec();
                state = MQTTSN_LINGER;
            }
            else {
                mqpub_discon();
                state = MQTTSN_DISCONNECTED;
                return;
            }
            break;
        }
#define LINGER_SEC 6
        case MQTTSN_LINGER:
        {
            uint32_t now = xtimer_now_usec();
            if ((now - linger)/US_PER_SEC >= LINGER_SEC) {
                mqpub_discon();
                state = MQTTSN_DISCONNECTED;
                return;
            }
        }
        break;
        case MQTTSN_DISCONNECTED:
            state = MQTTSN_NOT_CONNECTED;
            continue;
        default:
            break;
        }
        /* End up here if not successful. 
         * Wait a while and try again.
         */
        xtimer_sleep(MQPUB_STATE_INTERVAL);
    }
}


/*
 * Time for periodic publish?
 */
/*
 * Time of last sync 
 */
static timex_t last_periodic;

static int timeforperiodic(void) {

#ifdef MODULE_SIM7020
    if (!sim7020_active()) {
        return 0;
    }
#endif

    /* Make the first publish asap */
    static uint8_t first = 0;
    if (!first) {
        first = 1;
        return 1;
    }
    /* Has timer expired? */
    timex_t now;
    xtimer_now_timex(&now);
    timex_t periodic_due = timex_add(last_periodic, timex_set(MQTTSN_PUBLISH_INTERVAL, 0));
    return timex_cmp(now, periodic_due) >= 0;
}


#define MQPUB_THREAD_MAX_INTERVAL_SEC 60
static void *mqpub_thread(void *arg)
{
    (void)arg;
    xtimer_t interval_timer;
    uint32_t interval_secs = 1;
    last_periodic = timex_set(0, 0);
    interval_timer.callback = _periodic_callback;
    interval_timer.arg = &evt_mbox;
    
    xtimer_set(&interval_timer, interval_secs*US_PER_SEC);

    state = MQTTSN_NOT_CONNECTED;
    while (1) {
        msg_t msg;
        mbox_get(&evt_mbox, &msg);

        switch (msg.type) {
        case MSG_EVT_ASYNC:
            puts("async_publish");
            _publish_all(0);
            break;
        case MSG_EVT_PERIODIC:
            puts("periodic_publish");
#ifdef SNTP_SYNC
            puts("sycn periodic");
            sync_periodic();
#endif /* SNTP_SYNC */
#ifdef DNS_CACHE_REFRESH
            puts("A");
            dns_resolve_refresh();
#endif /* DNS_CACHE_REFRESH */
            if (timeforperiodic()) {
                puts("B");
                _publish_all(1);
                xtimer_now_timex(&last_periodic);
            }
            else 
            if (interval_secs < MQPUB_THREAD_MAX_INTERVAL_SEC) {
                puts("C");
                interval_secs <<= 1;
            }
            puts("D");
            xtimer_set(&interval_timer, interval_secs*US_PER_SEC);
            break;
        default:
            printf("mqttsn_state: bad type %d\n", msg.type);
        }
    }
    return NULL;
}
#endif /* MQTTSN_PUBLISHER_THREAD */


kernel_pid_t emcute_pid;

#ifdef MQTTSN_PUBLISHER_THREAD
kernel_pid_t mqpub_pid;
#endif /* MQTTSN_PUBLISHER_THREAD */

void mqttsn_publisher_init(void) {

    _init_default_topicstr();
    _init_default_basename();
    dns_resolve_init();
#ifdef APP_WATCHDOG
    app_watchdog_init();
#endif /* APP_WATCHDOG */    

    /* start emcute thread */
    emcute_pid = thread_create(emcute_stack, sizeof(emcute_stack), EMCUTE_PRIO, THREAD_CREATE_STACKTEST,
                               emcute_thread, NULL, "emcute");
    puts("a");
#ifdef MQTTSN_PUBLISHER_THREAD
    /* start publisher thread */
    puts("b");
    mqpub_pid = thread_create(mqpub_stack, sizeof(mqpub_stack), MQPUB_PRIO, THREAD_CREATE_STACKTEST,
                              mqpub_thread, NULL, "mqpub");
    puts("c");
    printf("start mqpub: pid %d\n", mqpub_pid);
#endif /* MQTTSN_PUBLISHER_THREAD */
}

typedef enum {
    s_gateway, s_connect, s_register, s_publish, s_reset} mqttsn_report_state_t;

int mqttsn_report(uint8_t *buf, size_t len, uint8_t *finished, 
                  __attribute__((unused)) char **topicp, __attribute__((unused)) char **basenamep) {
     char *s = (char *) buf;
     size_t l = len;
     static mqttsn_report_state_t state = s_gateway;
     int nread = 0;
     
     *finished = 0;
     
     switch (state) {
     case s_gateway:
          RECORD_START(s + nread, l - nread);
          PUTFMT(",{\"n\": \"mqtt_sn;gateway\",\"vs\":\"[%s]:%d\"}",MQTTSN_GATEWAY_HOST, MQTTSN_GATEWAY_PORT);
          RECORD_END(nread);
          state = s_connect;

     case s_connect:
          RECORD_START(s + nread, l - nread);
          PUTFMT(",{\"n\":\"mqtt_sn;stats;connect\",\"vj\":[");
          PUTFMT("{\"n\":\"ok\",\"u\":\"count\",\"v\":%d},", mqttsn_stats.connect_ok);
          PUTFMT("{\"n\":\"fail\",\"u\":\"count\",\"v\":%d}", mqttsn_stats.connect_fail);
          PUTFMT("]}");
          RECORD_END(nread);
          state = s_register;

     case s_register:
          RECORD_START(s + nread, l - nread);
          PUTFMT(",{\"n\":\"mqtt_sn;stats;register\",\"vj\":[");
          PUTFMT("{\"n\":\"ok\",\"u\":\"count\",\"v\":%d},", mqttsn_stats.register_ok);
          PUTFMT("{\"n\":\"fail\",\"u\":\"count\",\"v\":%d}", mqttsn_stats.register_fail);
          PUTFMT("]}");
          RECORD_END(nread);
          state = s_publish;
     
     case s_publish:
          RECORD_START(s + nread, l - nread);
          PUTFMT(",{\"n\":\"mqtt_sn;stats;publish\",\"vj\":[");
          PUTFMT("{\"n\":\"ok\",\"u\":\"count\",\"v\":%d},", mqttsn_stats.publish_ok);
          PUTFMT("{\"n\":\"fail\",\"u\":\"count\",\"v\":%d}", mqttsn_stats.publish_fail);
          PUTFMT("]}");
          RECORD_END(nread);
          state = s_reset;
          
     case s_reset:
          RECORD_START(s + nread, l - nread);
          PUTFMT(",{\"n\":\"mqtt_sn;stats;reset\",\"u\":\"count\",\"v\":%d}", mqttsn_stats.reset);
          PUTFMT(",{\"n\":\"mqtt_sn;stats;commreset\",\"u\":\"count\",\"v\":%d}", mqttsn_stats.commreset);
          RECORD_END(nread);

          state = s_gateway;
     }
     *finished = 1;

     return nread;
}

int sensor_report(uint8_t *buf, size_t len, uint8_t *finished,
                  __attribute__((unused)) char **topicp, __attribute__((unused)) char **basenamep)
{
    char *s = (char *)buf;
    size_t l = len;
    // static mqttsn_report_state_t state = s_gateway;
    int nread = 0;

    *finished = 0;

    RECORD_START(s + nread, l - nread);
    PUTFMT(",{\"n\":\"sensor_data\",\"vj\":[");
    PUTFMT("{\"n\":\"temp\",\"u\":\"celsius\",\"v\":\"value\"},");
    PUTFMT("{\"n\":\"humidity\",\"u\":\"measurement_unit_here\",\"v\":\"value\"},");
    PUTFMT("{\"n\":\"pressure\",\"u\":\"measurement_unit_here\",\"v\":\"value\"}");
    PUTFMT("]}");
    RECORD_END(nread);

    *finished = 1;

    return nread;
}

int mqttsn_stats_cmd(int argc, char **argv) {
    (void) argc; (void) argv;
    puts("MQTT-SN state: ");
#ifdef MQTTSN_PUBLISHER_THREAD
    switch (state) {
      case MQTTSN_NOT_CONNECTED:
          puts("not connected");
          break;
      case MQTTSN_CONNECTED:
          puts("connected");
          break;
      case MQTTSN_PUBLISHING:
          puts("publishing");
      case MQTTSN_LINGER:
          puts("linger");
      case MQTTSN_DISCONNECTED:
          puts("disconnected");
    }
#else
    puts("not started");
#endif /* MQTTSN_PUBLISHER_THREAD */
    puts("\n");
    puts("Statistics:");
    mqttsn_stats_t *st = &mqttsn_stats;
    printf("  connect: success %d, fail %d\n", st->connect_ok, st->connect_fail);
    printf("  register: success %d, fail %d\n", st->register_ok, st->register_fail);
    printf("  publish: success %d, fail %d\n", st->publish_ok, st->publish_fail);
    printf("  reset: %d\n", st->reset);
    printf("  commreset: %d\n", st->commreset);
    return 0;
}

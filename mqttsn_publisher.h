#ifndef MQTTSN_PUBLISHER_T
#define MQTTSN_PUBLISHER_T

#include "net/emcute.h"

typedef enum {
    MQTTSN_NOT_CONNECTED,
    MQTTSN_CONNECTED,
    MQTTSN_PUBLISHING,
    MQTTSN_LINGER,
    MQTTSN_DISCONNECTED,
} mqttsn_state_t;

typedef struct mqttsn_stats {
  uint16_t connect_ok;
  uint16_t register_ok;
  uint16_t publish_ok;
  uint16_t connect_fail;
  uint16_t register_fail;
  uint16_t publish_fail;
  uint16_t reset;
  uint16_t commreset;  
} mqttsn_stats_t;

extern mqttsn_stats_t mqttsn_stats;

typedef emcute_topic_t mqpub_topic_t;

void mqttsn_publisher_init(void);
mqttsn_state_t mqttsn_publisher_state(void);

#ifndef MQPUB_TOPIC_LENGTH
#define MQPUB_TOPIC_LENGTH  (64U)
#endif /* MQPUB_TOPIC_LENGTH */

#ifndef MQTT_TOPIC_BASE
#define MQTT_TOPIC_BASE "KTH/avr-rss2"
#endif

#ifndef MQPUB_BASENAME_LENGTH
#define MQPUB_BASENAME_LENGTH  (32U)
#endif /* MQPUB_BASENAME_LENGTH */

#ifndef MQPUB_BASENAME_FMT
#define MQPUB_BASENAME_FMT "urn:dev:mac:%s"
#endif /* MQPUB_BASENAME_FMT */

#ifndef MQTTSN_GATEWAY_HOST
//#define  MQTTSN_GATEWAY_HOST "::ffff:c010:7de8"
#define MQTTSN_GATEWAY_HOST "lab-pc.ssvl.kth.se"
#endif /* MQTTSN_GATEWAY_HOST */
#ifndef MQTTSN_GATEWAY_PORT
//#define MQTTSN_GATEWAY_PORT 1884
#define MQTTSN_GATEWAY_PORT 10000
#endif /* MQTTSN_GATEWAY_PORT */

#ifndef MQTTSN_BUFFER_SIZE
#if defined(CONFIG_EMCUTE_BUFSIZE)
#define MQTTSN_BUFFER_SIZE (CONFIG_EMCUTE_BUFSIZE - 16)
#elif defined(EMCUTE_BUFSIZE)
#define MQTTSN_BUFFER_SIZE (EMCUTE_BUFSIZE - 16)
#endif /* defined(CONFIG_EMCUTE_BUFSIZE) */
#endif  /* MQTTSN_BUFFER_SIZE */

#ifndef MQTTSN_PUBLISH_INTERVAL
/* Period publish interval (seconds) */
#define MQTTSN_PUBLISH_INTERVAL 600 //1200
#endif /* MQTTSN_PUBLISH_INTERVAL */

#ifndef INTERVAL_SEC
/* Buffer data generator interval (seconds)*/
#define INTERVAL_SEC 360
#endif /*INTERVAL_SEC*/

#ifndef MQTTSN_MAX_TOPICS
/* Max no of topics during a connection  */
#define MQTTSN_MAX_TOPICS 8
#endif /* MQTTSN_MAX_TOPICS */

#ifndef MQTTSN_MAX_SUBSCRIPTIONS
#define MQTTSN_MAX_SUBSCRIPTIONS 1
#endif /* MQTTSN_MAX_SUBSCRIPTIONS */

void mqttsn_publisher_init(void);

int get_nodeid(char *buf, size_t size);

int mqttsn_stats_cmd(int argc, char **argv);

int mqpub_pub(mqpub_topic_t *topic, void *data, size_t len);
int mqpub_con(char *host, uint16_t port);
int mqpub_reg(mqpub_topic_t *topic, char *topicstr);
int mqpub_discon(void);
int mqpub_reset(void);
size_t mqpub_init_topic(char *topic, size_t topiclen, char *nodeid, char *suffix);
size_t mqpub_init_basename(char *basename, size_t basenamelen, char *nodeid);
int mqpub_pubtopic(char *topicstr, uint8_t *data, size_t datalen);
int mqpub_start_subscription(char *topic, emcute_cb_t cb);

void mqpub_report_ready(void);

int dns_resolve_inetaddr(char *host, ipv6_addr_t *result);

int sensor_status(int argc, char **argv);
int controller_status(int argc, char **argv);

#endif /* MQTTSN_PUBLISHER_T */


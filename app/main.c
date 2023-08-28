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
#include "net/emcute.h"
#include "net/ipv6/addr.h"
#include "net/ipv4/addr.h"

#include "sync_timestamp.h"
#ifdef MODULE_MQTTSN_PUBLISHER
#include "mqttsn_publisher.h"
#endif

#if 0
static int run_mqttsn(int argc, char **argv)
{
    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        return 1;
    }
    mqttsn_publisher_init();
    return 0;
}
#endif

#ifdef MODULE_SIM7020
int sim7020cmd_init(int argc, char **argv);
int sim7020cmd_register(int argc, char **argv);
int sim7020cmd_activate(int argc, char **argv);
int sim7020cmd_status(int argc, char **argv);
int sim7020cmd_stats(int argc, char **argv);
int sim7020cmd_udp_socket(int argc, char **argv);
int sim7020cmd_close(int argc, char **argv);
int sim7020cmd_connect(int argc, char **argv);
int sim7020cmd_send(int argc, char **argv);
int sim7020cmd_test(int argc, char **argv);
int sim7020cmd_at(int arg, char **argv);
int sim7020cmd_recv(int arg, char **argv);
int sim7020cmd_reset(int arg, char **argv);
#endif /* MODULE_SIM7020 */

static const shell_command_t shell_commands[] = {
#ifdef MODULE_SIM7020
    { "init", "Init SIM7020", sim7020cmd_init },
    { "act", "Activate SIM7020", sim7020cmd_activate },
    { "register", "Register SIM7020", sim7020cmd_register },
    { "reg", "Register SIM7020", sim7020cmd_register },
    { "reset", "Reset SIM7020", sim7020cmd_reset },
    { "status", "Report SIM7020 status", sim7020cmd_status },
    { "stats", "Report SIM7020 statistics", sim7020cmd_stats },
    { "usock", "Create SIM7020 UDP socket", sim7020cmd_udp_socket },        
    { "ucon", "Connect SIM7020 socket", sim7020cmd_connect },
    { "usend", "Send on SIM7020 socket", sim7020cmd_send },
    { "uclose", "Close SIM7020 socket", sim7020cmd_close },
    { "utest", "repeat usend", sim7020cmd_test },                
    { "at", "run AT command, for example \"at AT+CSQ\"", sim7020cmd_at },
#endif /* MODULE_SIM7020 */
#ifdef MODULE_MQTTSN_PUBLISHER
    { "mqstat", "print MQTT status", mqttsn_stats_cmd},
#endif /* MODULE_MQTTSN_PUBLISHER */
    { NULL, NULL, NULL }
};

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static char line_buf[SHELL_DEFAULT_BUFSIZE];

int main(void)
{
    ///* the main thread needs a msg queue to be able to run `ping6`*/
    puts("1");
    msg_init_queue(_main_msg_queue, ARRAY_SIZE(_main_msg_queue));

    puts("2");
    sync_init();
#ifdef AUTO_INIT_MQTTSN
    puts("3");
    mqttsn_publisher_init();
#endif /* AUTO_INIT_MQTTSN */
    /* start shell */
    puts("4");
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
    /* should never be reached */
    return 0;
}

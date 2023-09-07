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

#include "msg.h"
#include "net/ipv6/addr.h"
#include "xtimer.h"

#include "mqttsn_publisher.h"
#include "report.h"
#include "sync_timestamp.h"

#ifdef EPCGW
#include "../epcgw.h"
#endif /* EPCGW */
#define ENABLE_DEBUG    (0)
#include "debug.h"

#ifdef BOARD_AVR_RSS2
#include "pstr_print.h"
#endif

static int seq_nr_value = 0;

#if defined(MODULE_GNRC_RPL)
int rpl_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
#endif
#if defined(MODULE_SIM7020)
int sim7020_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
#endif
#ifdef EPCGW
int epcgwstats_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
#endif /* EPCGW */
#ifdef MODULE_NETSTATS
int if_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
#endif /* MODULE_NETSTATS */
#ifdef APP_WATCHDOG
int app_watchdog_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
#endif /* APP_WATCHDOG */
int mqttsn_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
int boot_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);

// added
int sensor_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
int sensor_spec_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);
int controller_spec_report(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep);

static size_t preamble(uint8_t *buf, size_t len, char *basename) {
     char *s = (char *) buf;
     size_t l = len;
     int nread = 0;
     
     RECORD_START(s + nread, l - nread);
     PUTFMT("{\"bn\":\"%s;\"", basename);

     uint64_t basetime = sync_basetime();
     uint32_t utime_sec = basetime/1000000;
     uint32_t utime_msec = (basetime/1000) % 1000;
     PUTFMT(",\"bt\":%" PRIu32 ".%03" PRIu32 "}", utime_sec, utime_msec);

     PUTFMT(",{\"n\":\"seq_no\",\"v\":%d}", seq_nr_value++);
     RECORD_END(nread);

     return (nread);
}


report_state_t report_gen_state = s_sensor_report;

report_gen_t next_report_gen(void)
{
     //     static unsigned int reportno = 0;
     static uint8_t done_once = 0;

#ifdef EPCGW
     /* EPC reports have priority. If there is an epc report
      * waiting, send it now
      */
     report_gen_t epcgen = epcgw_report_gen();
     if (epcgen != NULL)
          return epcgen;
#endif /* EPCGW */

     if (done_once == 0)
     {
          done_once = 1;
          puts("calling boot");
          return (boot_report);
     }

     switch (report_gen_state)
     {
     case s_sensor_report:
          return (sensor_report);
     
     case s_sensor_spec_report:
          report_gen_state = s_sensor_report;
          puts("return sensor boot report here");
          return (sensor_spec_report);
     
     case s_controller_spec_report:
          report_gen_state = s_sensor_report;
          puts("returns controller boot report here");
          return (controller_spec_report);

     default:
          return (sensor_report);

     }

     /**
          switch (reportno++ % s_max_report) {
     #if defined(MODULE_GNRC_RPL)
          case s_rpl_report:
               return(rpl_report);
     #endif
     #if defined(MODULE_SIM7020)
          case s_sim7020_report:
               return(sim7020_report);
     #endif
     #if defined(EPCGW)
          case s_epcgwstats_report:
              return epcgwstats_report;
     #endif
     #if defined(MODULE_NETSTATS)
          case s_if_report:
              return if_report;
     #endif
     #if defined(APP_WATCHDOG)
          case s_app_watchdog_report:
              return app_watchdog_report;
     #endif
          case s_mqttsn_report:
               return(mqttsn_report);
          default:
              printf("Bad report no %d\n", reportno);
          } */
     return NULL;
}

#if ENABLE_DEBUG
static char *reportfunstr(report_gen_t fun) {
  if (fun == NULL)
    return "NUL";
  else if (fun == boot_report)
    return "boot";
#if defined(MODULE_GNRC_RPL)
  else if (fun == rpl_report)
    return("rpl");
#endif
#if defined(MODULE_SIM7020)
  else if (fun == sim7020_report)
    return("sim7020");
#endif
#if defined(EPCGW)
  else if (fun == epcgwstats_report)
    return("epcgwstats");
#endif
#if defined(APP_GATEWAY)
  else if (fun == app_gateway_report)
    return("app_gateway");
#endif
  else if (fun == mqttsn_report)
    return("mqttsn");
  else
    return("???");
}
#else
static char *reportfunstr(__attribute__((unused)) report_gen_t fun) {
    return NULL;
}
#endif /* ENABLE_DEBUG */

/*
 * Reports -- build report by writing records to buffer 
 * Call report function generator to schedule next report function.
 * The report function fills the report with sensor data
 */
size_t xreports(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep) {
     char *s = (char *) buf;
     size_t l = len;
     size_t nread = 0;
     static report_gen_t reportfun = NULL;
     //static uint8_t finished;

     if (reportfun == NULL) {
          reportfun = next_report_gen();
     }
     do {
         int n = reportfun((uint8_t *) s + nread, l - nread, finished, topicp, basenamep);
         DEBUG("reportfun '%s', n %d (tot %d) finished %d\n", reportfunstr(reportfun), n, nread, (int) finished);
         if (n == 0)
             return (nread);
         else
             nread += n;
     } while (!finished);
     reportfun = NULL;
     return (nread);
}

static size_t reports(uint8_t *buf, size_t len, uint8_t *finished, char **topicp, char **basenamep) {
     char *s = (char *) buf;
     size_t l = len;
     size_t nread = 0;
     static report_gen_t reportfun = NULL;

     if (reportfun == NULL) {
          puts("calling next report generator");
          reportfun = next_report_gen();
     }
     /* Call reportfun with null buffer to set basename first */
     (void) reportfun(NULL, 0, finished, topicp, basenamep);
     
     int n = preamble((uint8_t *) s + nread, l - nread, *basenamep); /* Save one for last bracket */
     if (n == 0)
         return (nread);
     else
         nread += n;

     do {
         int n = reportfun((uint8_t *) s + nread, l - nread, finished, topicp, NULL);
         DEBUG("reportfun '%s', n %d (tot %d) finished %d\n", reportfunstr(reportfun), n, nread, (int) finished);
         if (n == 0)
             return (nread);
         else
             nread += n;
     } while (!finished);
     reportfun = NULL;
     return (nread);
}

/*
 * make a sensor report. Write senml preamble to buffer, then call report function to fill
 * with data, and then write senml finish
 */
size_t makereport(uint8_t *buffer, size_t len, uint8_t *finished, char **topicp, char **basenamep) {
     char *s = (char *) buffer;
     size_t l = len;
     size_t n;
     int nread = 0;
     
     RECORD_START(s + nread, l - nread);
     PUTFMT("[");
     n = reports((uint8_t *) RECORD_STR(), RECORD_LEN()-1, finished, topicp, basenamep); /* Save one for last bracket */
     RECORD_ADD(n);
     PUTFMT("]");
     RECORD_END(nread);

     return (nread);
}

/*
 * make a sensor report. Write senml preamble to buffer, then call report function to fill
 * with data, and then write senml finish
 */
size_t xmakereport(uint8_t *buffer, size_t len, uint8_t *finished, char **topicstrp) {
     char *s = (char *) buffer;
     size_t l = len;
     size_t n;
     int nread = 0;
     
     RECORD_START(s + nread, l - nread);
     PUTFMT("[");
     n = preamble((uint8_t *) RECORD_STR(), RECORD_LEN()-1, NULL); /* Save one for last bracket */
     RECORD_ADD(n);
     n = reports((uint8_t *) RECORD_STR(), RECORD_LEN()-1, finished, topicstrp, NULL); /* Save one for last bracket */
     RECORD_ADD(n);
     PUTFMT("]");
     RECORD_END(nread);

     return (nread);
}


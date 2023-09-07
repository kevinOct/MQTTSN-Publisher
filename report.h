/*
 * Copyright (C) 2020 Peter Sjödin, KTH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */


/*
 * Macros for writing sensor records 
 * A record is a unit of text that must be kept together 
 * in a report, eg to represent a senml element
 */

#ifdef BOARD_AVR_RSS2
#include "pstr_print.h" 
#endif

//#define WARN printf
#define WARN(...)

/*
 * Mark start of a record. 
 * Save current state as starting state (string pointer and length).
 */
#define RECORD_START(STR, LEN) {                                        \
   __label__ _full, _notfull;                                           \
   char *_str = (STR);                                                  \
   size_t _len = (LEN), _nread = 0;                                     \

/*
 * Attempt to write printf-formatted text to a record
 */

#define PUTFMT(...) {                                                   \
    size_t _n = snprintf(_str + _nread, _len - _nread, __VA_ARGS__);    \
    if (_n < _len - _nread) {                                           \
        _nread += _n;                                                   \
    }                                                                   \
    else {                                                              \
         goto _full;                                                    \
    }                                                                   \
  }

/*
 * End of record -- commit if succeeded, else abort and revert to
 * starting state.
 * Causes current function to return. The return value is NREAD
 * plus the number of chars successfully written.
 */
#define RECORD_END(NREAD)                                               \
   goto _notfull;                                                       \
  _full:                                                                \
    WARN("%s: %d: no space left\n", __FILE__, __LINE__);                \
    *(_str) = '\0';                                                     \
    return ((NREAD));                                                   \
  _notfull:                                                             \
  (NREAD) += _nread;                                                    \
  }

/*
 * For calling external functions to place data in buffer:
 *
 * - Get current buffer pointer and len
 */
#define RECORD_STR() (_str + _nread)
#define RECORD_LEN() (_len - _nread)
/*
 * - Advance no of bytes written to buffer by N
 */
#define RECORD_ADD(N)                                                   \
  if ((N) < _len - _nread) {                                            \
      _nread += (N);                                                    \
   }                                                                    \
   else {                                                               \
      goto _full;                                                       \
   }                                                                    \
   

/*
 * Report generator.
 * Write a report into buffer buf of size len.
 * Return the number of bytes written (zero if the report did not fit).
 * Upon return, *finished is set to 0 if there are more reports that
 * should be sent, but did not fit. Otherwise *finished is set to 1.
 * topicstrp is a pointer to a topic string. The report generator
 * can use it to set the topic for each report. The topic string is
 * initialized to a default topic.
 */
typedef int (* report_gen_t)(uint8_t *buf, size_t len, uint8_t *finished, char **topicsp, char **basenamep);

size_t makereport(uint8_t *buffer, size_t len, uint8_t *finished, char **topicp, char **basenamep);

/*
 * Report scheduler -- return report generator function to use next
 */

typedef enum {
#if defined(MODULE_GNRC_RPL)
  s_rpl_report,
#endif
#if defined(MODULE_SIM7020)
  s_sim7020_report,
#endif
#if defined(EPCGW)
  s_epcgwstats_report,
#endif
#if defined(MODULE_NETSTATS)
  s_if_report,
#endif
#if defined(APP_WATCHDOG)
  s_app_watchdog_report,
#endif
  s_mqttsn_report,
  s_max_report,
  s_sensor_report, // added
  s_sensor_spec_report,
  s_controller_spec_report,
} report_state_t;

extern report_state_t report_gen_state;

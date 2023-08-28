192.16.125.234:1883
broker.ssvl.kth.se

uping broker.ssvl.kth.se 1883 2
in `sim7020_sock_udp.c` at line `202`:
```c
if (_mbox_get(&sock->mbox, &msg, blocking) == 0)
```
mailbox has no messages ?

at 'AT+CIPRECVMODE?'
at 'AT+CIPSTART=1,TCP,"216.58.207.238",80'
at 'AT+CIPSEND=1,5'
at 'AT+CIPRECVDATA=1,100'
at 'AT+PING="216.58.207.238"'
at 'AT+CIPRECVLEN?'
tcpping
=======

Sequentially creates tcp connections to the specified host and measures the latency.


Usage:
$ tcpping oioki.ru
  OK   Connected to 78.47.82.174:22, seq=1, time=44.004 ms
  OK   Connected to 78.47.82.174:22, seq=2, time=44.307 ms
  OK   Connected to 78.47.82.174:22, seq=3, time=44.165 ms
  OK   Connected to 78.47.82.174:22, seq=4, time=44.077 ms
^C
--- oioki.ru tcpping statistics ---
4 packets transmitted, 4 received, 0% packet loss, time 3240ms
rtt min/avg/max/mdev = 44.004/44.138/44.307/0.186 ms

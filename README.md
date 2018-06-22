This repository is collection of multicast tools written in a multitude of languages. As of now, it contains only multicast receivers, but in the future it might be extended to include multicast senders as well.

I tried to keep similar code structure for all programs, but some languages have so much different flow that it just doesn't make sense to force it just for the sake of consistency.

Command line API and output is always the same for all programs and it looks something like this:

```
$ mrecv.py --address 224.0.25.66 --port 21318 --interface eth0 --timeout 2
subscriber=10.1.21.56 destination=224.0.25.66:21318
2017-12-01 03:56:58.307648 source=69.50.112.16:21318 destination=224.0.25.66:21318 size=1232
2017-12-01 03:56:58.357749 source=69.50.112.16:21318 destination=224.0.25.66:21318 size=1318
2017-12-01 03:56:58.407947 source=69.50.112.16:21318 destination=224.0.25.66:21318 size=1276
2017-12-01 03:56:58.458121 source=69.50.112.16:21318 destination=224.0.25.66:21318 size=1181
2017-12-01 03:56:58.508293 source=69.50.112.16:21318 destination=224.0.25.66:21318 size=1325
[..]

Exiting after 2 seconds and 34 messages.
```

- `address` is the multicast group IP address
- `port` is multicast channel port
- `interface` is a local network interface to use for subscription and data receiving
- `timeout` is an optional argument to tell the program to exit after `n` number of seconds

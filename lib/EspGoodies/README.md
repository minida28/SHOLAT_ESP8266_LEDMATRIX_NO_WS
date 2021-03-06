
esp8266/Arduino goodies
-----------------------

* PingAlive  
  * Test library for [esp8266/Arduino#2330](https://github.com/esp8266/Arduino/issues/2330#issuecomment-387709161)
  * Pings the gateway every 5 seconds, and calls `pingFault()` after 5 minutes of unsuccessful pings
    * define your own `void pingFault (void)`
    * call `startPingAlive()` from `setup()` after wifi STA is connected.
    * status can be checked by reading `ping_seq_num_send` and `ping_seq_num_recv` values
    * setting `ping_should_stop` to 1 will stop ping (effectively stopped when it is reset to 0)

* NetDump (lwip2)  
  Packet sniffer library to help study network issues, check example-sketches  
  Log examples on serial console:
```
14:07:01.854 ->  in 0  ARP who has 10.43.1.117 tell 10.43.1.254
14:07:01.854 -> out 0  ARP 10.43.1.117 is at 5c:cf:7f:c3:ad:51

[...] hello-world, dumped in packets:
14:07:46.227 ->  in 0  IPv4 10.43.1.254>10.43.1.117 TCP 54546>2[P.] seq:1945448681..1945448699 ack:6618 win:29200 len=18
14:07:46.260 -> 5c cf 7f c3 ad 51 74 da 38 3a 1f 61 08 00 45 10 \..Qt.8:.a..E.
14:07:46.260 -> 00 3a b2 bc 40 00 40 06 70 29 0a 2b 01 fe 0a 2b .:..@.@.p).+...+
14:07:46.260 -> 01 75 d5 12 00 02 73 f5 30 e9 00 00 19 da 50 18 .u....s.0.....P.
14:07:46.260 -> 72 10 f8 da 00 00 70 6c 20 68 65 6c 6c 6f 2d 77 r.....pl hello-w
14:07:46.260 -> 6f 72 6c 64 20 31 0d 0a                         orld 1..
14:07:46.294 -> out 0  IPv4 10.43.1.117>10.43.1.254 TCP 2>54546[P.] seq:6618..6619 ack:1945448699 win:2126 len=1
14:07:46.326 -> 00 20 00 00 00 00 aa aa 03 00 00 00 08 00 45 00 . ............E.
14:07:46.326 -> 00 29 00 0d 00 00 ff 06 a3 f9 0a 2b 01 75 0a 2b .).........+.u.+
14:07:46.327 -> 01 fe 00 02 d5 12 00 00 19 da 73 f5 30 fb 50 18 ..........s.0.P.
14:07:46.327 -> 08 4e 93 d5 00 00 68                            .N....h
14:07:46.327 ->  in 0  IPv4 10.43.1.254>10.43.1.117 TCP 54546>2[.] seq:1945448699 ack:6619 win:29200
14:07:46.327 -> 5c cf 7f c3 ad 51 74 da 38 3a 1f 61 08 00 45 10 \..Qt.8:.a..E.
14:07:46.360 -> 00 28 b2 bd 40 00 40 06 70 3a 0a 2b 01 fe 0a 2b .(..@.@.p:.+...+
14:07:46.360 -> 01 75 d5 12 00 02 73 f5 30 fb 00 00 19 db 50 10 .u....s.0.....P.
14:07:46.360 -> 72 10 92 1b 00 00                               r.....
14:07:46.360 -> out 0  IPv4 10.43.1.117>10.43.1.254 TCP 2>54546[P.] seq:6619..6630 ack:1945448699 win:2126 len=11
14:07:46.360 -> 00 20 00 00 00 00 aa aa 03 00 00 00 08 00 45 00 . ............E.
14:07:46.360 -> 00 33 00 0e 00 00 ff 06 a3 ee 0a 2b 01 75 0a 2b .3.........+.u.+
14:07:46.393 -> 01 fe 00 02 d5 12 00 00 19 db 73 f5 30 fb 50 18 ..........s.0.P.
14:07:46.393 -> 08 4e 16 a1 00 00 65 6c 6c 6f 2d 77 6f 72 6c 64 .N....ello-world
14:07:46.393 -> 0a                                              .

[...] help protocol decoding from inside the esp
14:08:11.715 ->  in 0  IPv4 10.43.1.254>239.255.255.250 UDP 50315>1900 len=172
14:08:11.716 -> 01 00 5e 7f ff fa 74 da 38 3a 1f 61 08 00 45 00 ....t.8:.a..E.
14:08:11.716 -> 00 c8 9b 40 40 00 01 11 e1 c1 0a 2b 01 fe ef ff ...@@......+....
14:08:11.749 -> ff fa c4 8b 07 6c 00 b4 9c 28 4d 2d 53 45 41 52 .....l...(M-SEAR
14:08:11.749 -> 43 48 20 2a 20 48 54 54 50 2f 31 2e 31 0d 0a 48 CH * HTTP/1.1..H
14:08:11.749 -> 4f 53 54 3a 20 32 33 39 2e 32 35 35 2e 32 35 35 OST: 239.255.255
14:08:11.749 -> 2e 32 35 30 3a 31 39 30 30 0d 0a 4d 41 4e 3a 20 .250:1900..MAN: 
14:08:11.749 -> 22 73 73 64 70 3a 64 69 73 63 6f 76 65 72 22 0d "ssdp:discover".
14:08:11.749 -> 0a 4d 58 3a 20 31 0d 0a 53 54 3a 20 75 72 6e 3a .MX: 1..ST: urn:
14:08:11.782 -> 64 69 61 6c 2d 6d 75 6c 74 69 73 63 72 65 65 6e dial-multiscreen
14:08:11.782 -> 2d 6f 72 67 3a 73 65 72 76 69 63 65 3a 64 69 61 -org:service:dia
14:08:11.782 -> 6c 3a 31 0d 0a 55 53 45 52 2d 41 47 45 4e 54 3a l:1..USER-AGENT:
14:08:11.782 -> 20 47 6f 6f 67 6c 65 20 43 68 72 6f 6d 65 2f 36  Google Chrome/6
14:08:11.782 -> 36 2e 30 2e 33 33 35 39 2e 31 31 37 20 4c 69 6e 6.0.3359.117 Lin
14:08:11.782 -> 75 78 0d 0a 0d 0a                               ux....

[...] ping
14:08:32.258 ->  in 0  IPv4 10.43.1.254>10.43.1.117 ICMP ping request
14:08:32.291 -> out 0  IPv4 10.43.1.117>10.43.1.254 ICMP ping reply
```

* accurate TZ and DST available to your ESP with https://github.com/nayarsystems/posix_tz_db  
  example: `configTZ(TZ_Asia_Shanghai);`

* `wifi_on()` / `wifi_off()` helpers

* uart loopback enabler

* hard reset checker (can't soft-reset after serial programming checker)

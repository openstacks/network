# How to compile the program

$ gcc cli.c -o cli
$ gcc ser.c -o ser

# How to use it
The tun/tap interface must already exist, be up and configured with an
IP address, and owned by the user who runs ser and cli That user must also
have read/write permission on /dev/net/tun

##server side
```
$ ip tuntap add mode tun dev tun0
$ ip link set tun0 up
$ ip addr add 192.168.0.2/24 dev tun0
$ ser -i tun0
```

##client side
```
$ ip tuntap add mode tun dev tun0
$ ip link set tun0 up
$ ip addr add 192.168.0.3/24 dev tun0
$ cli -i tun0 -r 10.0.2.15
$ ping 192.168.0.2
```


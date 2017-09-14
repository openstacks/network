
from pytun import TunTapDevice,IFF_TAP 
from select import select

tap1 = TunTapDevice(name='tap01',flags=IFF_TAP)
tap2 = TunTapDevice(name='tap02',flags=IFF_TAP)

print "Name %s" % tap1.name
tap1.addr = '192.168.100.10'
tap1.dstaddr = '192.168.100.20'
tap1.netmask = '255.255.255.0'
tap1.mtu = 1500
tap1.up()

print "Name %s" % tap2.name
tap2.addr = '192.168.100.20'
tap2.dstaddr = '192.168.100.10'
tap2.netmask = '255.255.255.0'
tap2.mtu = 1500
tap2.up()

while True:
    r = select([tap1, tap2], [], [])[0][0]
    try:
        buf = r.read(r.mtu)
        if r == tap1:
            read = tap1.name
            tap2.write(buf)
        else:
            read = tap2.name
            tap1.write(buf)
        print "Read from %s: %s" % (read, buf.encode('hex'))
    except:
        tap1.close()
        tap2.close()
        exit()

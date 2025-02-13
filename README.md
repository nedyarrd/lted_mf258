# lted_mf258
Simple openwrt daemon to connect via ZTE MF258 LTE antenna to internet

It uses ZTE MF258 POE powered antenna to connect to internet. What You will need to make it work:
- POE 24V injector
- create network covering antena connected interface on OpenWRT
- create vlan on that interface with VLAN ID 100
- compiler toolchain to make executable (I used https://github.com/openwrt/openwrt to build my own)
  - make CC=mipsel-openwrt-linux-gcc LD=mipsel-openwrt-linux-musl-ld
- install executable by sftp'ing it to router
  - scp lted root@192.168.X.Y:lted
- edit config file

# Example config file
```
config 'lte' config
        option sms 1
        option pin "0000"
        option lte_network_interface "wan"
        option allow_roaming 1
        option change_dns 1
        option change_dns1 1
        option change_dns2 1
```

- run lted or lted-strip on router. It will maintain connection whole time

 # TODO
 - cleanup code
 - make sure that ActivatePDPContext is needed
 - use configured in antenna APN profiles
 - add code to manage SMS
 - make Luci for for configuration and SMS

# Goals
Now I'm using it on CUDY X6 Wifi6 router with Ubiquti POE injector whithout any problems

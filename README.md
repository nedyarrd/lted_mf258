# lted_mf258
Simple openwrt daemon to connect via ZTE MF258 LTE antenna to internet

It uses ZTE MF258 POE powered antenna to connect to internet. What You will need to make it work:
- POE 24V injector
- create network covering antena connected interface on OpenWRT
- create vlan on that interface with VLAN ID 100
- compiler toolchain to make executable (I used https://github.com/openwrt/openwrt to build my own)
- install executable by stp'ing it to router
- edit config file

# Example config file
config 'lte' config
        option sms 1
        option pin "0000"
        option lte_network_interface "wan"
        option allow_roaming 1
        option change_dns 1
        option change_dns1 1
        option change_dns2 1

- run lted or lted-strip on router. It will maintain connection whole time

  

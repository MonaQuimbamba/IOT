# IOT

![image](https://user-images.githubusercontent.com/75567246/197397769-2711c2a1-72b9-47dc-9368-dac558237462.png)





<details><summary> Raspberry Pi: network boot and NFS mount</summary>
 

## Preparing for bootp, PXE boot


* We will create the RASPI directory, dedicated to the “filesystem” of the Raspberry PI, the directory will occupy around 3GB in use:

```bash
$ mkdir RASPI
$ cd RASP
```

* A client directory containing the entire Raspbian system of the Raspberry (directories /etc,/home, /bin, etc.) which will be accessible by the NFS protocol;

```bash
~/RASP $ mkdir client

```

*  A boot directory containing the kernel and “low-level” files for the Raspberry Pi itself,
which will be accessible by the bootp protocol;

```nash
~/RASP $ mkdir boot
```


We will download the “Raspbian lite” distribution from the official Raspberry PI website and put it in
your RASPI directory.

```bash
$ wget https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2021-11-08/2021-10-30-raspios-bullseye-armhf-lite.zip
$ unzip 2021-10-30-raspios-bullseye-armhf-lite.zip
```

We will retrieve the contents of the two partitions of this distribution to fill our two directories:

* the raspbian filesystem in the client directory from partition #2:

```bash
$ unzip 2021-10-30-raspios-bullseye-armhf-lite.zip
$ sudo losetup -fP 2021-10-30-raspios-bullseye-armhf-lite.img
$ losetup -a | grep rasp
/dev/loop39: []: (/home/pef/2021-10-30-raspios-bullseye-armhf-lite.img)
$ sudo mount /dev/loop39p2 /mnt

~RASPI $ sudo rsync -xa --progress /mnt/ client/
~RASPI $ sudo umount /mnt
```

* the "boot" files from partition #1:

```bash
~RASPI $ sudo mount /dev/loop39p1 /mnt
~RASPI $ cp -r /mnt/* boot/
```



# NFS Server

### installation 

```bash
 $ sudo apt install nfs-kernel-server
```

### Configuring the NFS share in the /etc/exports file:

```bash
:/etc $ cat exports
# /etc/exports: the access control list for filesystems which may be exported
# to NFS clients. See exports(5).
#
# Example for NFSv2 and NFSv3:
# /srv/homes hostname1(rw,sync,no_subtree_check)
hostname2(ro,sync,no_subtree_check)
#
# Example for NFSv4:
# /srv/nfs4 gss/krb5i(rw,sync,fsid=0,crossmnt,no_subtree_check)
# /srv/nfs4/homes gss/krb5i(rw,sync,no_subtree_check)
#
~/RASPI/client *(rw,sync,no_subtree_check,no_root_squash)
~/RASPI/boot *(rw,sync,no_subtree_check,no_root_squash)
```

### Enable the NFS and RPCBind service:

```bash
$ sudo systemctl enable nfs-kernel-server
$ sudo systemctl enable rpcbind
```

### If you modify the configuration of an export, you must restart the NFS service:

```bash
$ sudo systemctl restart nfs-kernel-server
```

### To see the mount points offered by an NFS server:

```bash
$ showmount -e 127.0.0.1
Export list for 127.0.0.1:
~/RASPI/boot *
~/RASPI/client *
```
 
 
 ### Setting up the TFTP, DNS, DHCP server
 
 We will use the dnsmasq command in the script_boot_rpi script:
 
 ```bash
 # interface du PC connexion Raspberry
IF=enx000ec6885a80
# pour un menu de sélection de l'interface avec fzf
# IF=$(command -v fzf > /dev/null 2>&1 && ip l | awk '/^[0-9]+/ { print substr($2, 1,length($2)-1)}' | fzf)
sudo nmcli device set $IF managed no
PREFIX=10.20.30
sudo sysctl -w net.ipv4.ip_forward=1
sudo ip link set dev $IF down
sudo ip link set dev $IF up
sudo ip address add dev $IF $PREFIX.1/24
sudo iptables -t nat -A POSTROUTING -s $PREFIX.0/24 -j MASQUERADE
sudo rm /tmp/leases
sudo dnsmasq -d -z -i $IF -F $PREFIX.100,$PREFIX.150,255.255.255.0,12h -O 3,$PREFIX.1 -O
6,8.8.8.8 --pxe-service=0,"Raspberry Pi Boot" --enable-tftp --tftp-root=$HOME/PINFS/boot -l /tmp/leases
 ```
 
 
 
 ### Mounting NFS on the Raspberry PI
 
 We modify the mount point of the Raspberry Pi for its filesystem, by editing the file
 ~/RASPI/boot/cmdline.txt
 
 ```bash
 console=serial0,115200 console=tty1 root=/dev/nfs nfsroot=10.20.30.1:/home/pef/RASPI/client,tcp,vers=3 rw ip=dhcp rootwait
 ```
 <span style="color:yellow">Warning</span>: ***it's all on one line***
 
 We modify the etc/fstab file of the Raspberry Pi in:
 
 
 ```
 /RASPI/client/etc/fstab
 ```
 
 
 ```bash
proc /proc proc defaults 0 0
10.20.30.1:/home/pef/RASPI/boot /boot nfs defaults,vers=3 0 0
 ```
 <span style="color:yellow">Warning</span>: ***It should only contain these two lines***
 
 ### Activation of the SSH service on the Raspberry PI
 
 we go through the NFS mount point, i.e. the local directory corresponding to the NFS filesystem
 
 ```bash
 $ cd RASPI/client
 ~/RASPI/client $ sudo vi lib/systemd/system/sshswitch.service
 ```
 and change this file to:
 
 ```bash
 Unit]
Description=Turn on SSH if /boot/ssh is present
After=regenerate_ssh_host_keys.service
[Service]
Type=oneshot
ExecStart=/bin/sh -c "systemctl enable --now ssh"
[Install]
WantedBy=multi-user.target
 ```

 
create a symbolic link to the ssh service file
 
 ```bash
 cd RASP/client/etc/systemd/system
 ```
 
 and 
 
 ```bash
 sudo ln -s /lib/systemd/system/ssh.service ssh.service
 ```
 
 ### Check the password of the user ***pi***
 
 if the use pi on /etc/shadow file does not has a password then you can create and next add it on this file
 
 ```bash
 $ mkpasswd -m sha-512 -S x5dKFLDOE -s raspberry
 
 $6$x5dKFLDOE$HetDNj50LFJt4Ts8dnj5fIjHCymxhtUshKtJvu.giYdyUyHlOYZ2duON1hg3weGRhIgtPcyFb14u7WnUyD4HD/
 ```
 
 shadow file
 
 ```
 pi: $6$x5dKFLDOE$HetDNj50LFJt4Ts8dnj5fIjHCymxhtUshKtJvu.giYdyUyHlOYZ2duON1hg3weGRhIgtPcyFb14u7WnUyD4HD/:18930:0:99999:7:::

 ```
 
 ## Start Raspberry
 
 ```bash
 ./script_boot_rpi                                                                                                                                        
net.ipv4.ip_forward = 1                                                                                                                                    
rm: impossible de supprimer '/tmp/leases': Aucun fichier ou dossier de ce type                                                                             
dnsmasq: demarré, version 2.79 (taille de cache 150)                                                                                                
dnsmasq: options à la compilation : IPv6 GNU-getopt DBus i18n IDN DHCP DHCPv6 no-Lua TFTP conntrack ipset auth nettlehash DNSSEC loop-detect inotify       
dnsmasq-dhcp: DHCP, IP range 10.20.30.100 -- 10.20.30.150, lease time 12h                                                                           
dnsmasq-dhcp: DHCP, sockets bound exclusively to interface enp0s31f6                                                                              
dnsmasq-tftp: TFTP root est /home/quimbamba/TMC/boot
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/fixup.dat to 10.20.30.127
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/recovery.elf non trouvé
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/dt-blob.bin non trouvé
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/recovery.elf non trouvé
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/bootcfg.txt non trouvé
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/start.elf to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/fixup.dat to 10.20.30.127
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/recovery.elf non trouvé
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/dt-blob.bin non trouvé
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/recovery.elf non trouvé
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/bootcfg.txt non trouvé
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/bcm2710-rpi-3-b.dtb to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/bcm2710-rpi-3-b.dtb to 10.20.30.127
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/overlays/overlay_map.dtb to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/overlays/overlay_map.dtb to 10.20.30.127
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/config.txt to 10.20.30.127
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/overlays/vc4-kms-v3d.dtbo to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/overlays/vc4-kms-v3d.dtbo to 10.20.30.127
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/cmdline.txt to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/cmdline.txt to 10.20.30.127
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/recovery8.img non trouvé
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/kernel8.img to 10.20.30.127
dnsmasq-tftp: fichier /home/quimbamba/TMC/boot/armstub8.bin non trouvé
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/kernel8.img to 10.20.30.127
dnsmasq-tftp: error 0 Early terminate received from 10.20.30.127
dnsmasq-tftp: failed sending /home/quimbamba/TMC/boot/kernel8.img to 10.20.30.127
dnsmasq-tftp: sent /home/quimbamba/TMC/boot/kernel8.img to 10.20.30.127
dnsmasq-dhcp: DHCPDISCOVER(enp0s31f6) b8:27:eb:73:ff:53
dnsmasq-dhcp: DHCPOFFER(enp0s31f6) 10.20.30.127 b8:27:eb:73:ff:53
dnsmasq-dhcp: DHCPDISCOVER(enp0s31f6) b8:27:eb:73:ff:53
dnsmasq-dhcp: DHCPOFFER(enp0s31f6) 10.20.30.127 b8:27:eb:73:ff:53
dnsmasq-dhcp: DHCPREQUEST(enp0s31f6) 10.20.30.127 b8:27:eb:73:ff:53
dnsmasq-dhcp: DHCPACK(enp0s31f6) 10.20.30.127 b8:27:eb:73:ff:53
dnsmasq-dhcp: DHCPREQUEST(enp0s31f6) 10.20.30.127 b8:27:eb:73:ff:53
dnsmasq-dhcp: DHCPACK(enp0s31f6) 10.20.30.127 b8:27:eb:73:ff:53 raspberrypi
 ```
 
 
 ```bash
 ssh pi@10.20.30.127
 ```
 
</details>




<details><summary>hostapd and dnsmasq</summary>

 ## configuring the raspberry pi.
 
### Country configuration for WiFi:
 
 ```bash
pi@raspberrypi:~ $ rfkill unblock all
pi@raspberrypi:~ $ wpa_cli -i wlan0 set country FR
pi@raspberrypi:~ $ wpa_cli -i wlan0 save_config
 ```
 ### For access point configuration
```bash
pi@raspberrypi:~ $ sudo apt update
pi@raspberrypi:~ $ sudo apt-get install hostapd
pi@raspberrypi:~ $ sudo apt-get install dnsmasq
```
 
 ### Config Dnsmasq /etc/dnsmasq.conf :
 
 ```bash
pi@raspberrypi:~ $ sudo mv /etc/dnsmasq.conf /etc/dnsmasq.conf.bak       #backup original file
pi@raspberrypi:~ $ sudo vim /etc/dnsmasq.conf                           #create new file

pi@raspberrypi:~ $ cat /etc/dnsmasq.conf
interface=wlan0        #choose the interfac
dhcp-range=10.33.33.100,10.33.33.150,255.255.255.0,12h
domain=wlan
address=/mqtt.com/10.33.33.101        #allow the dns to resolve a domain in mqtt.com
 ```
 ### Config hostapd /etc/hostapd/hostapd.conf
 
 ```
pi@raspberrypi:~ $ cat /etc/hostapd/hostapd.conf 
country_code=FR
interface=wlan0
ssid=iot_claudio
hw_mode=g
channel=7
wmm_enabled=0
macaddr_acl=0
auth_algs=1
ignore_broadcast_ssid=0
wpa=2
wpa_passphrase=123456789
wpa_key_mgmt=WPA-PSK
wpa_pairwise=TKIP
rsn_pairwise=CCMP
 ```
### Config ipv4 forwarding
 
 ```
 pi@raspberrypi:~ $ sudo nano /etc/sysctl.conf

net.ipv4.ip_forward=1     #uncomment this line

 ```
 
 ## Static and manual config

###  Add nameserver in resolvconf.conf for dnsmasq
 
 ```
 pi@raspberrypi:~ $ cat /etc/resolvconf.conf 
# Configuration for resolvconf(8)
# See resolvconf.conf(5) for details

resolv_conf=/etc/resolv.conf
# If you run a local name server, you should uncomment the below line and
# configure your subscribers configuration files below.
name_servers=127.0.0.56

# Mirror the Debian package defaults for the below resolvers
# so that resolvconf integrates seemlessly.
dnsmasq_resolv=/var/run/dnsmasq/resolv.conf
pdnsd_conf=/etc/pdnsd.conf
unbound_conf=/var/cache/unbound/resolvconf_resolvers.conf

 ```
 
 ### Set Static IP and allow-hotplug for wlan0 to UP.
 
 ```
pi@raspberrypi:~ $ cat /etc/network/interfaces
# interfaces(5) file used by ifup(8) and ifdown(8)
# Include files from /etc/network/interfaces.d:
source /etc/network/interfaces.d
#/*
allow-hotplug wlan0
iface wlan0 inet static
        address 10.33.33.101
        netmask 255.255.255.0
        gateway 10.33.33.101

 ```
 
 ### Enable hostapd
 
 ```
pi@raspberrypi:~ $ sudo systemctl unmask hostapd
pi@raspberrypi:~ $ sudo systemctl enable hostapd
pi@raspberrypi:~ $ sudo systemctl enable dnsmasq

 ```
 
 Reboot the Raspberry Pi
 
 ```bash
sudo reboot
 ```
 
 
 ### test ping to mqtt.com to see if the DSN works
 
 ```bash

pi@raspberrypi:~ $ ping -c 3 mqtt.com
PING mqtt.com (10.33.33.101) 56(84) bytes of data.
64 bytes from 10.33.33.101 (10.33.33.101): icmp_seq=1 ttl=64 time=0.161 ms
64 bytes from 10.33.33.101 (10.33.33.101): icmp_seq=2 ttl=64 time=0.106 ms
64 bytes from 10.33.33.101 (10.33.33.101): icmp_seq=3 ttl=64 time=0.090 ms

--- mqtt.com ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2002ms
rtt min/avg/max/mdev = 0.090/0.119/0.161/0.030 ms
 ```

</details>








<details><summary>Certicates</summary>

## Generation of private keys for the CA, the server and the client.
```
pi@raspberrypi:~/CERTIFICATES $ openssl ecparam -out ecc.ca.key.pem -name prime256v1 -genkey 
pi@raspberrypi:~/CERTIFICATES $ openssl ecparam -out ecc.raspberry.key.pem -name prime256v1 -genkey 
pi@raspberrypi:~/CERTIFICATES $ openssl ecparam -out ecc.esp8266.key.pem -name prime256v1 -genkey

```
## Generation self-signed certificate of the CA which will be used to sign those of the server and client

```
pi@raspberrypi:~/CERTIFICATES $
openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:TRUE") -new -nodes -subj "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=ACTMC" -x509 -extensions ext -sha256 -key ecc.ca.key.pem -text -out ecc.ca.pem
```

## Generation and signing of the certificate for the server (Raspberry Pi)

```
pi@raspberrypi:~/CERTIFICATES $
$ openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:FALSE") -new -subj "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=mqtt.com" -reqexts ext -sha256 -key ecc.raspberry.key.pem -text -out ecc.raspberry.csr.pem

$ openssl x509 -req -days 3650 -CA ecc.ca.pem -CAkey ecc.ca.key.pem -CAcreateserial -extfile <(printf "basicConstraints=critical,CA:FALSE") -in
ecc.raspberry.csr.pem -text -out ecc.raspberry.pem -addtrust clientAuth
```

## Generating and signing the certificate for the client (Esp8266)

```
pi@raspberrypi:~/CERTIFICATES $
$ openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:FALSE") -new -subj "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=esp8266" -reqexts ext -sha256 -key ecc.esp8266.key.pem -text -out ecc.esp8266.csr.pem

$ openssl x509 -req -days 3650 -CA ecc.ca.pem -CAkey ecc.ca.key.pem -CAcreateserial -extfile <(printf "basicConstraints=critical,CA:FALSE") -in ecc.esp8266.csr.pem -text -out ecc.esp8266.pem -addtrust clientAuth
```
                                                                                                      
</details>
 
 
<details><summary> Mosquitto</summary>
 
### installation 
 
```bash
pi@raspberrypi:~ $ sudo apt-get install mosquitto 
pi@raspberrypi:~ $ sudo apt-get install mosquitto-clients

```

We copy ecc.ca.pem ecc.raspberry.pem and ecc.raspberry.key.pem to the  ***/etc/mosquitto/*** directories:

```
pi@raspberrypi:~ $
$ sudo cp /CERTIFICATES/ecc.ca.pem /etc/mosquitto/ca_certificates/
$ sudo cp /CERTIFICATES/ecc.raspberry.pem /etc/mosquitto/certs/
$ sudo cp /CERTIFICATES/ecc.raspberry.key.pem /etc/mosquitto/certs/

```

### and we change the ownership of the certificates to the user "mosquitto" and the group "mosquitto".
```
sudo chown mosquitto:mosquitto  /etc/mosquitto/ca_certificates/ecc.ca.pem
sudo chown mosquitto:mosquitto  /etc/mosquitto/certs/ecc.raspberry.pem
sudo chown mosquitto:mosquitto  /etc/mosquitto/certs/ecc.raspberry.key.pem

```

 They are referenced in the ***/etc/mosquitto/mosquitto.conf*** file like this:

```
allow_anonymous false
password_file /etc/mosquitto/mosquitto_passwd

listener 8883
cafile /etc/mosquitto/ca_certificates/ecc.ca.pem
certfile /etc/mosquitto/certs/ecc.raspberry.pem
keyfile /etc/mosquitto/certs/ecc.raspberry.key.pem
require_certificate true

```
 
### It will enable the user authentication by password and certificate for mosquitto.
 
Then we use mosquitto_passwd to generate the user tmc in file mosquitto_passwd:

 ```bash 
 pi@raspberrypi:~ $ sudo mosquitto_passwd -c /etc/mosquitto/mosquitto_passwd tmc  

 ```

After copying the files, modifying the ***mosquitto.conf*** file and adding new user, we must restart the server:

```
pi@raspberrypi:~ $ sudo systemctl restart mosquitto.service
```

### Test MQTT server TLS connection

To publish a topic using the username nguyen.nguyen.doan and pass 1234 and a client certificate (certificate for esp8266)

```
pi@raspberrypi:~ $ mosquitto_pub -h mqtt.com -p 8883 -u tmc -P iot -t '/esp8266' --cafile ecc.ca.pem --cert ecc.esp8266.pem --key ecc.esp8266.key.pem -m 'Hey'

```

To subcribe a topic using the username nguyen.nguyen.doan and pass 1234 and a server certificate (certificate for raspberry)

```
pi@raspberrypi:~ $ mosquitto_sub -h mqtt.com -p 8883 -u tmc -P iot -t '/esp8266' --cafile ecc.ca.pem --cert ecc.raspberry.pem --key ecc.raspberry.key.pem

```


</details>



<details><summary>Mongoose os</summary>
 
 
## Install Mongoose OS
 
Configure ESP8266 using Mongoose OS to generate a flash for ESP8266
 
```bash
$ sudo add-apt-repository ppa:mongoose-os/mos
$ sudo apt-get update
$ sudo apt-get install mos
$ mos --help
```
## To generate a flash, install docker and set the execution right
 
 ```bash
$ sudo apt install docker.io
$ sudo groupadd docker
$ sudo usermod -aG docker $USER
```
 
## New MQTT app
 
Install new app and config file mos.yml like following:
 
```
$ git clone https://github.com/mongoose-os-apps/empty my-app
$ cd my-app
$ cat mos.yml

 cat mos.yml
author: mongoose-os
description: A Mongoose OS app skeleton
version: 1.0

libs_version: ${mos.version}
modules_version: ${mos.version}
mongoose_os_version: ${mos.version}

# Optional. List of tags for online search.
tags:
  - c

# List of files / directories with C sources. No slashes at the end of dir names.
sources:
  - src

# List of dirs. Files from these dirs will be copied to the device filesystem
filesystem:
  - fs



build_vars:
        MGOS_MBEDTLS_ENABLE_ATCA: 1
config_schema:
    - ["debug.level", 3]
    - ["sys.atca.enable", "b", true, {title: "Enable the chip"}]
    - ["i2c.enable", "b", true, {title: "Enable I2C"}]
    - ["sys.atca.i2c_addr", "i", 0x60, {title: "I2C address of the chip"}]
    - ["mqtt.enable", "b", true, {title: "Enable MQTT"}]
    - ["mqtt.server", "s", "mqtt.com:8883", {title: "MQTT server"}]
    - ["mqtt.pub", "s", "/esp8266", {title: "Publish topic"}]
    - ["mqtt.user", "s", "tmc", {title: "User name"}]
    - ["mqtt.pass", "s", "iot", {title: "Password"}]
    - ["mqtt.ssl_ca_cert", "s", "ecc.ca.pem", {title: "Verify server certificate using this CA bundle"}]
    - ["mqtt.ssl_cert", "s", "ecc.esp8266.pem", {title: "Client certificate to present to the      server"}]
    - ["mqtt.ssl_key", "ATCA:0"]
    - ["wifi.ap.enable", "b", false, {title: "Enable"}]
    - ["wifi.sta.enable", "b", true, {title: "Connect to existing WiFi"}]
    - ["wifi.sta.ssid", "s", "iot_claudio", {title: "SSID"}]
    - ["wifi.sta.pass", "s", "123456789", {title: "Password", type: "password"}]
    
cdefs:
   MG_ENABLE_MQTT: 1

build_vars:   
   # Override to 0 to disable ATECCx08 support.    
   # Set to 1 to enable ATECCx08 support.
   # MGOS_MBEDTLS_ENABLE_ATCA: 0#
   MGOS_MBEDTLS_ENABLE_ATCA: 1


libs:
    - origin: https://github.com/mongoose-os-libs/ca-bundle
    - origin: https://github.com/mongoose-os-libs/boards
    - origin: https://github.com/mongoose-os-libs/rpc-service-config
    - origin: https://github.com/mongoose-os-libs/rpc-mqtt
    - origin: https://github.com/mongoose-os-libs/rpc-uart
    - origin: https://github.com/mongoose-os-libs/wifi
    - origin: https://github.com/mongoose-os-libs/rpc-service-i2c
    - origin: https://github.com/mongoose-os-libs/mbedtls
    - origin: https://github.com/mongoose-os-libs/atca
    - origin: https://github.com/mongoose-os-libs/rpc-service-fs
    - origin: https://github.com/mongoose-os-libs/rpc-service-atca
                                                
# Used by the mos tool to catch mos binaries incompatible with this file format
manifest_version: 2017-09-29

```
 
 ### Copy certificate file ecc.ca.pem, ecc.ca.key.pem to my-app folder 
 
 * create  a certificate for the  ***ESP8266*** as mentioned on the certificate section 
 
 * and then modify the certificates to bi like that
 
 ```bash
 cat ecc.ca.pem
-----BEGIN CERTIFICATE-----
MIIBqTCCAU6gAwIBAgIUZvV4MFZwpjQOQDY2+VAo854viQgwCgYIKoZIzj0EAwIw
SzELMAkGA1UEBhMCRlIxEDAOBgNVBAcMB0xpbW9nZXMxDDAKBgNVBAoMA1RNQzEM
MAoGA1UECwwDSU9UMQ4wDAYDVQQDDAVBQ1RNQzAeFw0yMjEyMjYxMjE4MTlaFw0y
MzAxMjUxMjE4MTlaMEsxCzAJBgNVBAYTAkZSMRAwDgYDVQQHDAdMaW1vZ2VzMQww
CgYDVQQKDANUTUMxDDAKBgNVBAsMA0lPVDEOMAwGA1UEAwwFQUNUTUMwWTATBgcq
hkjOPQIBBggqhkjOPQMBBwNCAAQfPmdpqouUrgoa2dh3SkCN1hsidembc0weGKvH
X8M165stzB4BZr7pOMngEmR4tXVfp3SeeS/y5Vp9SzOCqENVoxAwDjAMBgNVHRME
BTADAQH/MAoGCCqGSM49BAMCA0kAMEYCIQCr7I5ji3TlyUnoFwsHhLKWNZoW8RjU
2QCXciWSx+r/dAIhANii/EhpqvvFEZpMCVezmxuVotnO8FEJg+VhEfyeIEIJ
-----END CERTIFICATE-----

 ```
 
 and 
 
 ```bash
 cat ecc.esp8266.pem
 
 -----BEGIN CERTIFICATE-----
MIIBqjCCAVCgAwIBAgIULt5GDXJYM/gs+AGfgJA+XXI1wUswCgYIKoZIzj0EAwIw
SzELMAkGA1UEBhMCRlIxEDAOBgNVBAcMB0xpbW9nZXMxDDAKBgNVBAoMA1RNQzEM
MAoGA1UECwwDSU9UMQ4wDAYDVQQDDAVBQ1RNQzAeFw0yMjEyMjYxMjM5MjdaFw0z
MjEyMjMxMjM5MjdaME0xCzAJBgNVBAYTAkZSMRAwDgYDVQQHDAdMaW1vZ2VzMQww
CgYDVQQKDANUTUMxDDAKBgNVBAsMA0lPVDEQMA4GA1UEAwwHZXNwODI2NjBZMBMG
ByqGSM49AgEGCCqGSM49AwEHA0IABNg6ah44kFNPH08liyiMY10sWaqFAROup1fr
928AusR2NRWTH0VoeimrPITvUfos1WvqkzWB/GzKSuycc5odzSOjEDAOMAwGA1Ud
EwEB/wQCMAAwCgYIKoZIzj0EAwIDSAAwRQIhAN+B80BkA9ybWaPTfc823sNgeDaJ
rBMYkwyc7wxV3+QBAiASLd6hUbaLmFHwN45w68npFrvlIBRrv6fq8WrJf7OzdzAM
MAoGCCsGAQUFBwMC
-----END CERTIFICATE-----

 ```
 
 
 next move the two certificate to fs folder inside pof my-app folder 
 
 Set up the ESP8266 with 
 
 ```
my-app $ sudo mos build --local --platform esp8266
my-app $ sudo mos flash 

 ```
 
 ## Install private key into ATECC508:
 
 ```bash
 $ openssl rand -hex 32 > slot4.key
 $ sudo mos -X atca-set-key 4 slot4.key --dry-run=false
 $ sudo mos -X atca-set-key 0 ecc.esp8266.key.pem --write-key=slot4.key --dry-run=false
 ```
 
 ## test 
 
 ```bash
 mos console
 ```
 
 ### Output for ACCESS point Connected  
 
 ```
 [Dec 26 16:19:13.816] mgos_wifi.c:294         WiFi scan done, num_res 6
[Dec 26 16:19:13.821] mgos_wifi_sta.c:404     WiFi scan result: 6 entries
[Dec 26 16:19:13.833] mgos_wifi_sta.c:280       0: SSID eduroam                          BSSID 78:72:5d:c2:8b:a1 auth 0, ch  1, RSSI -84 - no cfg cfg -1 att -1
[Dec 26 16:19:13.845] mgos_wifi_sta.c:280       1: SSID iot_claudio                      BSSID b8:27:eb:26:aa:06 auth 3, ch  7, RSSI -52 - ok cfg 0 att 1
[Dec 26 16:19:13.857] mgos_wifi_sta.c:280       2: SSID iPhone de Nouraouia              BSSID 42:10:84:ff:69:5e auth 3, ch  6, RSSI -74 - no cfg cfg -1 att -1
[Dec 26 16:19:13.869] mgos_wifi_sta.c:280       3: SSID eduroam                          BSSID 78:72:5d:67:cd:d1 auth 0, ch  6, RSSI -73 - no cfg cfg -1 att -1
[Dec 26 16:19:13.881] mgos_wifi_sta.c:280       4: SSID Freebox-680089                   BSSID 22:66:cf:91:00:14 auth 3, ch  6, RSSI -68 - no cfg cfg -1 att -1
[Dec 26 16:19:13.896] mgos_wifi_sta.c:280       5: SSID eduroam                          BSSID 78:0c:f0:ce:b7:51 auth 0, ch 11, RSSI -82 - no cfg cfg -1 att -1
[Dec 26 16:19:13.896] mgos_wifi_sta.c:380     AP queue:
[Dec 26 16:19:13.908] mgos_wifi_sta.c:388       0: SSID iot_claudio                     , BSSID b8:27:eb:26:aa:06 ch  7 RSSI -52 cfg 0 att 1 wc 0 age 8
[Dec 26 16:19:13.913] mgos_wifi_sta.c:478     State 5 ev -1 timeout 0
[Dec 26 16:19:13.921] mgos_wifi_sta.c:611     Trying iot_claudio AP b8:27:eb:26:aa:06 ch 7 RSSI -52 cfg 0 att 2
[Dec 26 16:19:13.934] esp_wifi.c:177          Set rate_limit_11b 0 - 3
[Dec 26 16:19:13.934] esp_wifi.c:193          Set rate_limit_11g 0 - 10
[Dec 26 16:19:13.934] esp_wifi.c:209          Set rate_limit_11n 0 - 11
[Dec 26 16:19:13.939] esp_main.c:138          SDK: sleep disable
[Dec 26 16:19:13.945] mgos_wifi_sta.c:478     State 6 ev 1464224001 timeout 0
[Dec 26 16:19:13.950] mgos_event.c:134        ev WFI1 triggered 0 handlers
[Dec 26 16:19:13.955] mgos_wifi_sta.c:478     State 6 ev -1 timeout 0
[Dec 26 16:19:13.963] mgos_net.c:89           WiFi STA: connecting
[Dec 26 16:19:13.963] mgos_event.c:134        ev NET1 triggered 1 handlers
[Dec 26 16:19:14.043] esp_main.c:138          SDK: scandone
[Dec 26 16:19:14.047] esp_main.c:138          SDK: state: 0 -> 2 (b0)
[Dec 26 16:19:14.053] esp_main.c:138          SDK: state: 2 -> 3 (0)
[Dec 26 16:19:14.071] esp_main.c:138          SDK: state: 3 -> 5 (10)
[Dec 26 16:19:14.077] esp_main.c:138          SDK: add 0
[Dec 26 16:19:14.077] esp_main.c:138          SDK: aid 1
[Dec 26 16:19:14.080] esp_main.c:138          SDK: cnt 
[Dec 26 16:19:14.093] esp_main.c:138          SDK: 
[Dec 26 16:19:14.099] esp_main.c:138          SDK: connected with iot_claudio, channel 7
[Dec 26 16:19:14.103] esp_main.c:138          SDK: dhcp client start...
[Dec 26 16:19:14.112] mgos_wifi.c:83          WiFi STA: Connected, BSSID b8:27:eb:26:aa:06 ch 7 RSSI -54
[Dec 26 16:19:14.121] mgos_wifi_sta.c:478     State 6 ev 1464224002 timeout 0
[Dec 26 16:19:14.121] mgos_event.c:134        ev WFI2 triggered 0 handlers
[Dec 26 16:19:14.127] mgos_net.c:93           WiFi STA: connected

 ```
### Output for certificate verify ok:
 
 ```
 gos_wifi_sta.c:388       0: SSID iot_claudio                     , BSSID b8:27:eb:26:aa:06 ch  7 RSSI -54 cfg 0 att 0 wc 0 age 1
[Dec 26 16:19:15.623] mgos_event.c:134        ev WFI3 triggered 0 handlers
[Dec 26 16:19:15.630] mgos_mongoose.c:66      New heap free LWM: 45432
[Dec 26 16:19:15.644] mgos_net.c:103          WiFi STA: ready, IP 10.33.33.145, GW 10.33.33.101, DNS 10.33.33.101, NTP 0.0.0.0
[Dec 26 16:19:15.644] mgos_net.c:208          Setting DNS server to 10.33.33.101
[Dec 26 16:19:15.654] mgos_mqtt_conn.c:442    MQTT0 connecting to mqtt.com:8883
[Dec 26 16:19:15.654] mgos_event.c:134        ev MOS6 triggered 0 handlers
[Dec 26 16:19:15.662] mongoose.c:3136         0x3fff0414 mqtt.com:8883 ecc.esp8266.pem,ATCA:0,ecc.ca.pem
[Dec 26 16:19:15.670] mgos_vfs.c:280          ecc.esp8266.pem -> /ecc.esp8266.pem pl 1 -> 1 0x3ffef7b4 (refs 1)
[Dec 26 16:19:15.684] mgos_vfs.c:375          open ecc.esp8266.pem 0x0 0x1b6 => 0x3ffef7b4 ecc.esp8266.pem 1 => 257 (refs 1)
[Dec 26 16:19:15.690] mgos_vfs.c:535          fstat 257 => 0x3ffef7b4:1 => 0 (size 656)
[Dec 26 16:19:15.707] mgos_vfs.c:535          fstat 257 => 0x3ffef7b4:1 => 0 (size 656)
[Dec 26 16:19:15.707] mgos_vfs.c:563          lseek 257 0 1 => 0x3ffef7b4:1 => 0
[Dec 26 16:19:15.707] mgos_vfs.c:563          lseek 257 0 0 => 0x3ffef7b4:1 => 0
[Dec 26 16:19:15.713] mgos_vfs.c:409          close 257 => 0x3ffef7b4:1 => 0 (refs 0)
[Dec 26 16:19:15.871] mgos_vfs.c:280          ecc.ca.pem -> /ecc.ca.pem pl 1 -> 1 0x3ffef7b4 (refs 1)
[Dec 26 16:19:15.884] mgos_vfs.c:375          open ecc.ca.pem 0x0 0x1b6 => 0x3ffef7b4 ecc.ca.pem 1 => 257 (refs 1)
[Dec 26 16:19:15.890] mgos_vfs.c:409          close 257 => 0x3ffef7b4:1 => 0 (refs 0)
[Dec 26 16:19:15.896] mongoose.c:3136         0x3fff0f54 udp://10.33.33.101:53 -,-,-
[Dec 26 16:19:15.901] mongoose.c:3006         0x3fff0f54 udp://10.33.33.101:53
[Dec 26 16:19:15.906] mgos_event.c:134        ev NET3 triggered 2 handlers
[Dec 26 16:19:15.914] mongoose.c:3020         0x3fff0f54 udp://10.33.33.101:53 -> 0
[Dec 26 16:19:15.919] mgos_mongoose.c:66      New heap free LWM: 41360
[Dec 26 16:19:15.933] mongoose.c:3006         0x3fff0414 tcp://10.33.33.101:8883
[Dec 26 16:19:15.939] mgos_mongoose.c:66      New heap free LWM: 41088
[Dec 26 16:19:15.954] mongoose.c:3020         0x3fff0414 tcp://10.33.33.101:8883 -> 0
[Dec 26 16:19:15.968] mgos_mongoose.c:66      New heap free LWM: 40392
[Dec 26 16:19:15.980] mgos_vfs.c:280          ecc.ca.pem -> /ecc.ca.pem pl 1 -> 1 0x3ffef7b4 (refs 1)
[Dec 26 16:19:15.989] mgos_vfs.c:375          open ecc.ca.pem 0x0 0x1b6 => 0x3ffef7b4 ecc.ca.pem 1 => 257 (refs 1)
[Dec 26 16:19:15.995] mgos_vfs.c:535          fstat 257 => 0x3ffef7b4:1 => 0 (size 635)
[Dec 26 16:19:16.150] ATCA ECDSA verify ok, verified
[Dec 26 16:19:16.156] mgos_vfs.c:409          close 257 => 0x3ffef7b4:1 => 0 (refs 0)
[Dec 26 16:19:16.215] ATCA ECDSA verify ok, verified
[Dec 26 16:19:16.272] ATCA:16 ECDH gen pubkey ok
[Dec 26 16:19:16.329] ATCA:16 ECDH ok
[Dec 26 16:19:16.427] ATCA:0 ECDSA sign ok
 ```
 
### Output for MQTT publish:
 
 ```
 [Dec 26 16:19:16.586] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:16.586] mgos_mqtt_conn.c:118    MQTT0 ack 1
[Dec 26 16:19:16.593] mgos_mqtt_conn.c:154    MQTT0 pub -> 2 /esp8266 @ 1 DUP (7): [Hello !]
[Dec 26 16:19:16.609] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:16.609] mgos_mqtt_conn.c:118    MQTT0 ack 2
[Dec 26 16:19:16.616] mgos_mqtt_conn.c:154    MQTT0 pub -> 3 /esp8266 @ 1 DUP (7): [Hello !]
[Dec 26 16:19:16.627] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:16.627] mgos_mqtt_conn.c:118    MQTT0 ack 3
[Dec 26 16:19:16.634] mgos_mqtt_conn.c:154    MQTT0 pub -> 4 /esp8266 @ 1 DUP (7): [Hello !]
[Dec 26 16:19:16.646] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:16.646] mgos_mqtt_conn.c:118    MQTT0 ack 4
[Dec 26 16:19:16.653] mgos_mqtt_conn.c:154    MQTT0 pub -> 5 /esp8266 @ 1 DUP (7): [Hello !]
[Dec 26 16:19:16.665] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:16.665] mgos_mqtt_conn.c:118    MQTT0 ack 5
[Dec 26 16:19:16.669] mgos_mqtt_conn.c:322    MQTT0 queue drained
[Dec 26 16:19:16.996] mgos_mqtt_conn.c:154    MQTT0 pub -> 8 /esp8266 @ 1 (7): [Hello !]
[Dec 26 16:19:17.014] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:17.014] mgos_mqtt_conn.c:118    MQTT0 ack 8
[Dec 26 16:19:18.996] mgos_mqtt_conn.c:154    MQTT0 pub -> 9 /esp8266 @ 1 (7): [Hello !]
[Dec 26 16:19:19.039] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:19.039] mgos_mqtt_conn.c:118    MQTT0 ack 9
[Dec 26 16:19:20.996] mgos_mqtt_conn.c:154    MQTT0 pub -> 10 /esp8266 @ 1 (7): [Hello !]
[Dec 26 16:19:21.013] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:21.013] mgos_mqtt_conn.c:118    MQTT0 ack 10
[Dec 26 16:19:22.996] mgos_mqtt_conn.c:154    MQTT0 pub -> 11 /esp8266 @ 1 (7): [Hello !]
[Dec 26 16:19:23.013] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:23.013] mgos_mqtt_conn.c:118    MQTT0 ack 11
[Dec 26 16:19:24.081] esp_main.c:138          SDK: pm open,type:0 0
[Dec 26 16:19:24.997] mgos_mqtt_conn.c:154    MQTT0 pub -> 12 /esp8266 @ 1 (7): [Hello !]
[Dec 26 16:19:25.013] mgos_mqtt_conn.c:180    MQTT0 event: 204
[Dec 26 16:19:25.013] mgos_mqtt_conn.c:118    MQTT0 ack 12
[Dec 26 16:19:26.997] mgos_mqtt_conn.c:154    MQTT0 pub -> 13 /esp8266 @ 1 (7): [Hello !]

 ```
</details>


<details><summary>LoRa communication on the Raspberry Pi</summary>

 ### To activate the SPI bus used by the LoRa component, you will modify the follow file at the end.
 
***RASPI/boot/config.txt***
 
 
 ![image](https://user-images.githubusercontent.com/75567246/209676562-4bb90d39-51ec-4a91-83ff-a9efa2667327.png)

 
### For the use of the GPIOs pins and the SPI bus you will need the bcm2835 library:

 ***http://www.airspayce.com/mikem/bcm2835/***
 
 ```
 $ wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz
$ tar zxvf bcm2835-1.71.tar.gz
$ cd bcm2835-1.71
$ ./configure
$ make
$ sudo make check
$ sudo make install
 ```
 
 ### For the use of LoRa, we will use the following library:
 
 ```bash
 $ git clone https://github.com/hallard/RadioHead
 ```
 
 You will go to the following library directory:
 
 
 ```bash
 $ cd RadioHead/examples/raspi/rf95
 ```
 
 You will modify the two source files: "rf95_server.cpp" and "rf95_client.cpp", to select the dragino:
 
 
 
 ![image](https://user-images.githubusercontent.com/75567246/209677143-21f62c10-b462-4445-8e5f-ef193eceacec.png)

 
 Then you will compile and run one of the versions of the software:
 
 ```bash
 $ make
$ sudo ./rf95_client
 ```
 
 
 ## For the MQTT client we goin to use python pho_mqtt library
 

To use MQTT in Python, you can use the paho-mqtt library, which is a client implementation of the MQTT protocol.
Here is an example of how to use the paho-mqtt library to connect to our ESP8266 server :



```python
#!/bin/python3                                                                                                                                             
import paho.mqtt.client as mqtt                                                                                                                              import os, ssl, binascii,jwt, base64, subprocess                                                                                                            import aes as AES                                                                                                                                                                      
                                                                                                                                                                                       
# generate the key with this cmd line :  xxd -p -l 16 -c 16 /dev/urandom                                                                                                               
SECRET_KEY=b'b68eca6ee927b3c9a3f133c0a069bb3a'
iv = b"b845a927fe81d320"

 
 
def on_message(client, obj, msg):
    ## clair data from mosquitoo_pub
    data=msg.topic + " " + str(msg.qos) + " " + str(msg.payload)
    data = str(msg.payload) # encode the data 
    data = AES.encrypt_AES_GCM(data,SECRET_KEY,iv) ## encrypt with AES
    ## use rf95 programm to send the Data to onther endPoint LORA
    data =jwt.encode( {'data':data.decode('utf-8') }, "TMC", algorithm='HS256')
    cmd = subprocess.Popen("sudo ./RadioHead/examples/raspi/rf95/rf95_client %s "%data, shell=True,stdout=subprocess.PIPE)
    (resultat, ignorer) = cmd.communicate()
    #print(resultat)  Data sent 

    
    

client_mqtt = mqtt.Client()
# Assign event callbacks
client_mqtt.on_message = on_message
# variables for connexion 
rasp_key  = "/home/pi/CERTIFICATES/ecc.raspberry.key.pem"
rasp_cert = "/home/pi/CERTIFICATES/ecc.raspberry.pem"
ca_cert   = "/home/pi/CERTIFICATES/ecc.ca.pem"
url = 'mqtt.com'
port=8883
topic =  '/esp8266'
# set uo the variables 
client_mqtt.username_pw_set("tmc", "iot")
client_mqtt.tls_set(ca_certs=ca_cert, certfile=rasp_cert, keyfile=rasp_key, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLS, ciphers=None)
# connect to 
client_mqtt.connect(url,port)
client_mqtt.subscribe(topic, 0)

# Network loop 
while True:
    client_mqtt.loop()

```
 
 
 The aes.py file use to encrypt and decrypt the data 
 
 
 
 ```python
 #!/usr/bin/python3                                                                                                                                                                     
from Crypto.Cipher import AES 
import base64

def pad_string(s: str, block_size: int = 16) -> str:
    # Calculate the number of padding bytes needed
    num_padding_bytes = block_size - (len(s) % block_size)
    # Add the padding bytes to the string
    padded_s = s + (chr(num_padding_bytes) * num_padding_bytes)
    return padded_s


def unpad_string(s: str) -> str:
    # Get the number of padding bytes from the last byte of the string
    num_padding_bytes = ord(s[-1])
    # Remove the padding bytes from the string
    unpadded_s = s[:-num_padding_bytes]
    return unpadded_s


def encrypt_AES_GCM(msg, secretkey,iv):
      aes = AES.new(secretkey, AES.MODE_CBC, iv)
      padded_data = pad_string(msg,32)
      encrypted_data = aes.encrypt(padded_data)
      return base64.b64encode(encrypted_data)
      #return encrypted_data

def decrypt_AES_GCM(encryptedMsg, secretkey,iv):
     aes = AES.new(secretkey, AES.MODE_CBC, iv)     
     encrypted_data = base64.b64decode(encryptedMsg)
     decrypted_data = aes.decrypt(encrypted_data)
     decrypted_data = unpad_string(decrypted_data.decode())
     return decrypted_data

 
 ```

</details>

* https://github.com/cesanta/mongoose-os-docs/blob/master/mongoose-os/userguide/security.md

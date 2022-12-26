# IOT

![image](https://user-images.githubusercontent.com/75567246/197397769-2711c2a1-72b9-47dc-9368-dac558237462.png)





<details><summary> Raspberry Pi: network boot and NFS mount</summary>

## Preparing for bootp, PXE boot


* We will create the RASPI directory, dedicated to the “filesystem” of the Raspberry PI, the directory will occupy around 3GB in use:

```
$ mkdir RASPI
$ cd RASP
```

* a client directory containing the entire Raspbian system of the Raspberry (directories /etc,
/home, /bin, etc.) which will be accessible by the NFS protocol;

```
~/RASP $ mkdir client

```

*  a boot directory containing the kernel and “low-level” files for the Raspberry Pi itself,
which will be accessible by the bootp protocol;

```
~/RASP $ mkdir boot
```


We will download the “Raspbian lite” distribution from the official Raspberry PI website and put it in
your RASPI directory.

```
$ wget https://downloads.raspberrypi.org/raspios_lite_armhf/images/raspios_lite_armhf-2021-11-08/2021-10-30-raspios-bullseye-armhf-lite.zip
$ unzip 2021-10-30-raspios-bullseye-armhf-lite.zip
```

We will retrieve the contents of the two partitions of this distribution to fill our two directories:

* the raspbian filesystem in the client directory from partition #2:

```
$ unzip 2021-10-30-raspios-bullseye-armhf-lite.zip
$ sudo losetup -fP 2021-10-30-raspios-bullseye-armhf-lite.img
$ losetup -a | grep rasp
/dev/loop39: []: (/home/pef/2021-10-30-raspios-bullseye-armhf-lite.img)
$ sudo mount /dev/loop39p2 /mnt

~RASPI $ sudo rsync -xa --progress /mnt/ client/
~RASPI $ sudo umount /mnt
```

* the "boot" files from partition #1:

```
~RASPI $ sudo mount /dev/loop39p1 /mnt
~RASPI $ cp -r /mnt/* boot/
```



# NFS Server

### installation 

```
 $ sudo apt install nfs-kernel-server
```

### Configuring the NFS share in the /etc/exports file:

```
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

```
$ sudo systemctl enable nfs-kernel-server
$ sudo systemctl enable rpcbind
```

### If you modify the configuration of an export, you must restart the NFS service:

```
$ sudo systemctl restart nfs-kernel-server
```

### To see the mount points offered by an NFS server:

```
$ showmount -e 127.0.0.1
Export list for 127.0.0.1:
~/RASPI/boot *
~/RASPI/client *
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

$ openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:TRUE") -new -nodes -subj "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=ACTMC" -x509 -extensions ext -sha256 -key ecc.ca.key.pem -text -out ecc.ca.cert.crt

ou 

openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:TRUE") -new -nodes -subj "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=ACTMC" -x509 -extensions ext -sha256 -key ecc.ca.key.pem -text -out ecc.ca.pem

```

## Generation and signing of the certificate for the server (Raspberry Pi)

```
pi@raspberrypi:~/CERTIFICATES $

$ openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:FALSE") -new -subj   "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=mqtt.com" -reqexts ext -sha256 -key ecc.raspberry.key.pem -text -out ecc.raspberry.csr.pem

$ openssl x509 -req -days 3650 -CA ecc.ca.cert.crt -CAkey ecc.ca.key.pem -CAcreateserial -extfile <(printf   "basicConstraints=critical,CA:FALSE") -in ecc.raspberry.csr.pem -text -out ecc.raspberry.cert.crt -addtrust clientAuth

ou


$ openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:FALSE") -new -subj "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=mqtt.com" -reqexts ext -sha256 -key ecc.raspberry.key.pem -text -out ecc.raspberry.csr.pem

$ openssl x509 -req -days 3650 -CA ecc.ca.pem -CAkey ecc.ca.key.pem -CAcreateserial -extfile <(printf "basicConstraints=critical,CA:FALSE") -in
ecc.raspberry.csr.pem -text -out ecc.raspberry.pem -addtrust clientAuth

```

## Generating and signing the certificate for the client (Esp8266)

```
pi@raspberrypi:~/CERTIFICATES $

$ openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:FALSE") -new -subj   "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=esp8266" -reqexts ext -sha256 -key ecc.esp8266.key.pem -text -out ecc.esp8266.csr.pem

$ openssl x509 -req -days 3650 -CA ecc.ca.cert.crt -CAkey ecc.ca.key.pem -CAcreateserial -extfile <(printf   "basicConstraints=critical,CA:FALSE") -in ecc.esp8266.csr.pem -text -out ecc.esp8266.cert.crt -addtrust clientAuth


ou

$ openssl req -config <(printf "[req]\ndistinguished_name=dn\n[dn]\n[ext]\nbasicConstraints=CA:FALSE") -new -subj "/C=FR/L=Limoges/O=TMC/OU=IOT/CN=esp8266" -reqexts ext -sha256 -key ecc.esp8266.key.pem -text -out ecc.esp8266.csr.pem

$ openssl x509 -req -days 3650 -CA ecc.ca.pem -CAkey ecc.ca.key.pem -CAcreateserial -extfile <(printf "basicConstraints=critical,CA:FALSE") -in ecc.esp8266.csr.pem -text -out ecc.esp8266.pem -addtrust clientAuth


```
# Mosquitto conf

We copy ecc.ca.pem ecc.raspberry.pem and ecc.raspberry.key.pem to the /etc/mosquitto/ directories:

```
pi@raspberrypi:~ $
$ sudo cp /CERTIFICATES/ecc.ca.pem /etc/mosquitto/ca_certificates/
$ sudo cp /CERTIFICATES/ecc.raspberry.pem /etc/mosquitto/certs/
$ sudo cp /CERTIFICATES/ecc.raspberry.key.pem /etc/mosquitto/certs/

```

### add mosquitto user 
```
sudo chown mosquitto:mosquitto  /etc/mosquitto/ca_certificates/ecc.ca.pem
sudo chown mosquitto:mosquitto  /etc/mosquitto/certs/ecc.raspberry.pem
sudo chown mosquitto:mosquitto  /etc/mosquitto/certs/ecc.raspberry.key.pem

```

### They are referenced in the /etc/mosquitto/mosquitto.conf file like this:

```
allow_anonymous false
password_file /etc/mosquitto/mosquitto_passwd

listener 8883
cafile /etc/mosquitto/ca_certificates/ecc.ca.pem
certfile /etc/mosquitto/certs/ecc.raspberry.pem
keyfile /etc/mosquitto/certs/ecc.raspberry.key.pem
require_certificate true

```

### After copying the files, modifying the mosquitto.conf file and adding new user, we must restart the server:

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



# Mongoose os



* https://github.com/cesanta/mongoose-os-docs/blob/master/mongoose-os/userguide/security.md

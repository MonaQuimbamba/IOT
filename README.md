# IOT

![image](https://user-images.githubusercontent.com/75567246/197397769-2711c2a1-72b9-47dc-9368-dac558237462.png)

# Certicates

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











* https://github.com/cesanta/mongoose-os-docs/blob/master/mongoose-os/userguide/security.md

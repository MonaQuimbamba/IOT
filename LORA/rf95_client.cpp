// rf95_client.cpp
//
// Example program showing how to use RH_RF95 on Raspberry Pi
// Uses the bcm2835 library to access the GPIO pins to drive the RFM95 module
// Requires bcm2835 library to be already installed
// http://www.airspayce.com/mikem/bcm2835/
// Use the Makefile in this directory:
// cd example/raspi/rf95
// make
// sudo ./rf95_client
//
// Contributed by Charles-Henri Hallard based on sample RH_NRF24 by Mike Poublon

#include <bcm2835.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <RH_RF69.h>
#include <RH_RF95.h>
#include <nlohmann/json.hpp>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
//#include "jwt/jwt.hpp"
// define hardware used change to fit your need
// Uncomment the board you have, if not listed 
// uncommment custom board and set wiring tin custom section

// LoRasPi board 
// see https://github.com/hallard/LoRasPI
//#define BOARD_LORASPI

// iC880A and LinkLab Lora Gateway Shield (if RF module plugged into)
// see https://github.com/ch2i/iC880A-Raspberry-PI
//#define BOARD_IC880A_PLATE

// Raspberri PI Lora Gateway for multiple modules 
// see https://github.com/hallard/RPI-Lora-Gateway
//#define BOARD_PI_LORA_GATEWAY

// Dragino Raspberry PI hat
// see https://github.com/dragino/Lora
#define BOARD_DRAGINO_PIHAT

// Now we include RasPi_Boards.h so this will expose defined 
// constants with CS/IRQ/RESET/on board LED pins definition
#include "../RasPiBoards.h"

// Our RFM95 Configuration 
#define RF_FREQUENCY  868.00
#define RF_GATEWAY_ID 1 
#define RF_NODE_ID    10

// Create an instance of a driver
RH_RF95 rf95(RF_CS_PIN, RF_IRQ_PIN);
//RH_RF95 rf95(RF_CS_PIN);

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = false;

void sig_handler(int sig)
{
  printf("\n%s Break received, exiting!\n", __BASEFILE__);
  force_exit=true;
}

void handleErrors(void)
{
	    ERR_print_errors_fp(stderr);
	        abort();
}


int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
		            unsigned char *iv, unsigned char *ciphertext)
{
	    EVP_CIPHER_CTX *ctx;

	        int len;

		    int ciphertext_len;

		        /* Create and initialise the context */
		        if(!(ctx = EVP_CIPHER_CTX_new()))
				        handleErrors();

			    /*
			     *      * Initialise the encryption operation. IMPORTANT - ensure you use a key
			     *           * and IV size appropriate for your cipher
			     *                * In this example we are using 256 bit AES (i.e. a 256 bit key). The
			     *                     * IV size for *most* modes is the same as the block size. For AES this
			     *                          * is 128 bits
			     *                               */
			    if(1!= EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
				            handleErrors();

			        /*
				 *      * Provide the message to be encrypted, and obtain the encrypted output.
				 *           * EVP_EncryptUpdate can be called multiple times if necessary
				 *                */
			        if(1!= EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
					        handleErrors();
				    ciphertext_len = len;

				        /*
					 *      * Finalise the encryption. Further ciphertext bytes may be written at
					 *           * this stage.
					 *                */
				        if(1!= EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
						        handleErrors();
					    ciphertext_len += len;

					        /* Clean up */
					        EVP_CIPHER_CTX_free(ctx);

						    return ciphertext_len;
}


//Main Function
int main (int argc, const char* argv[] )
{
  static unsigned long last_millis;
  static unsigned long led_blink = 0;
  
  signal(SIGINT, sig_handler);
  printf( "%s\n", __BASEFILE__);

  if (!bcm2835_init()) {
    fprintf( stderr, "%s bcm2835_init() Failed\n\n", __BASEFILE__ );
    return 1;
  }
 
 
  printf( "RF95 CS=GPIO%d", RF_CS_PIN);

#ifdef RF_LED_PIN
  pinMode(RF_LED_PIN, OUTPUT);
  digitalWrite(RF_LED_PIN, HIGH );
#endif

#ifdef RF_IRQ_PIN
  printf( ", IRQ=GPIO%d", RF_IRQ_PIN );
  // IRQ Pin input/pull down 
  pinMode(RF_IRQ_PIN, INPUT);
  bcm2835_gpio_set_pud(RF_IRQ_PIN, BCM2835_GPIO_PUD_DOWN);
#endif
  
#ifdef RF_RST_PIN
  printf( ", RST=GPIO%d", RF_RST_PIN );
  // Pulse a reset on module
  pinMode(RF_RST_PIN, OUTPUT);
  digitalWrite(RF_RST_PIN, LOW );
  bcm2835_delay(150);
  digitalWrite(RF_RST_PIN, HIGH );
  bcm2835_delay(100);
#endif

#ifdef RF_LED_PIN
  printf( ", LED=GPIO%d", RF_LED_PIN );
  digitalWrite(RF_LED_PIN, LOW );
#endif

  if (!rf95.init()) {
    fprintf( stderr, "\nRF95 module init failed, Please verify wiring/module\n" );
  } else {
    printf( "\nRF95 module seen OK!\r\n");

#ifdef RF_IRQ_PIN
    // Since we may check IRQ line with bcm_2835 Rising edge detection
    // In case radio already have a packet, IRQ is high and will never
    // go to low so never fire again 
    // Except if we clear IRQ flags and discard one if any by checking
    rf95.available();

    // Now we can enable Rising edge detection
    bcm2835_gpio_ren(RF_IRQ_PIN);
#endif

    // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

    // The default transmitter power is 13dBm, using PA_BOOST.
    // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
    // you can set transmitter powers from 5 to 23 dBm:
    //rf95.setTxPower(23, false); 
    // If you are using Modtronix inAir4 or inAir9,or any other module which uses the
    // transmitter RFO pins and not the PA_BOOST pins
    // then you can configure the power transmitter power for -1 to 14 dBm and with useRFO true. 
    // Failure to do that will result in extremely low transmit powers.
    //rf95.setTxPower(14, true);

    rf95.setTxPower(14, false); 

    // You can optionally require this module to wait until Channel Activity
    // Detection shows no activity on the channel before transmitting by setting
    // the CAD timeout to non-zero:
    //rf95.setCADTimeout(10000);

    // Adjust Frequency
    rf95.setFrequency( RF_FREQUENCY );

    // This is our Node ID
    rf95.setThisAddress(RF_NODE_ID);
    rf95.setHeaderFrom(RF_NODE_ID);
    
    // Where we're sending packet
    rf95.setHeaderTo(RF_GATEWAY_ID);  

    printf("RF95 node #%d init OK @ %3.2fMHz\n", RF_NODE_ID, RF_FREQUENCY );

    last_millis = millis();

    //Begin the main body of code
    while (!force_exit) {

      //printf( "millis()=%ld last=%ld diff=%ld\n", millis() , last_millis,  millis() - last_millis );

      // Send every 5 seconds
      if ( millis() - last_millis > 5000 ) {
        last_millis = millis();

#ifdef RF_LED_PIN
        led_blink = millis();
        digitalWrite(RF_LED_PIN, HIGH);
#endif
        
        // Send a message to rf95_server
  /*      uint8_t data[] = "Hi Raspi!";
        uint8_t len = sizeof(data);
        
        printf("Sending %02d bytes to node #%d => ", len, RF_GATEWAY_ID );
        printbuffer(data, len);
        printf("\n" );
        rf95.send(data, len);
        rf95.waitPacketSent();
*/

	/* A 256 bit key */
	 unsigned char *key = (unsigned char *)"01234567890123456789012345678901";
	 /* A 128 bit IV */
	 unsigned char *iv = (unsigned char *)"0123456789012345";
	// get data from input 
	const char* data;
	unsigned char * plaintext =(unsigned char *) argv[1]; 
	//data = plaintext.c_str();
        //uint8_t len = sizeof(data);

	 /* Buffer for the decrypted text */
	 unsigned char ciphertext[128];
         int decryptedtext_len, ciphertext_len;
        /* Encrypt the plaintext */
       ciphertext_len = encrypt (plaintext, strlen ((char *)plaintext), key, iv, ciphertext);
 
//	printf(" data encrypted in AES is %s\n",ciphertext_len);
  //      printf("\n");

         uint8_t* data2send = new uint8_t [ciphertext_len];
	for(int i =0 ; i < ciphertext_len; i++) data2send[i]=ciphertext[i];
											        
        printf("Sending %02d bytes to node #%d => ", ciphertext_len, RF_GATEWAY_ID );
        printbuffer(data2send, ciphertext_len);
        printf("\n\n\n" );
        rf95.send(data2send, ciphertext_len);
        rf95.waitPacketSent();
        exit(1);

/*
        // Now wait for a reply
        uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);

        if (rf95.waitAvailableTimeout(1000)) { 
          // Should be a reply message for us now   
          if (rf95.recv(buf, &len)) {
            printf("got reply: ");
            printbuffer(buf,len);
            printf("\nRSSI: %d\n", rf95.lastRssi());
          } else {
            printf("recv failed");
          }
        } else {
          printf("No reply, is rf95_server running?\n");
        }
*/
        
      }

#ifdef RF_LED_PIN
      // Led blink timer expiration ?
      if (led_blink && millis()-led_blink>200) {
        led_blink = 0;
        digitalWrite(RF_LED_PIN, LOW);
      }
#endif
      
      // Let OS doing other tasks
      // Since we do nothing until each 5 sec
      bcm2835_delay(100);
    }
  }

#ifdef RF_LED_PIN
  digitalWrite(RF_LED_PIN, LOW );
#endif
  printf( "\n%s Ending\n", __BASEFILE__ );
  bcm2835_close();
  return 0;
}


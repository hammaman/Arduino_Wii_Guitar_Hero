/*
 * Wii Guitar Hero functions  -- Talk to the Guitar Hero instruments
 *
 * Developed from Tod E. Kurt Wii nunchuck library
 * Supplemented from wiibrew.org
 *
 * Initialisation of devices requires:
 * you must first write 0x55 to 0x(4)a400f0, then 0 to 0x(4)a400fb
 */ 
 
/*
   FUNCTIONS created:
	Wii_GH_setpowerpins()
	Wii_GH_init()
	Wii_GH_request_type()
	Wii_GH_send_request()
	Wii_GH_get_data()
	Wii_GH_print_raw_data_HEX()
	Wii_GH_print_raw_data_BIN()
	
	GHdrumhit()
	    GH_DRUMPED
        GH_DRUMRED
        GH_DRUMYEL 
        GH_DRUMBLU
        GH_DRUMORA
        GH_DRUMGRE
	
	GHdrumhitstrength()
	
	GHjoyx()
	GHjoyy()
	GHbutplus()
	GHbutminus()

	GHguitarbutton()
        GH_GUITORA
        GH_GUITRED
        GH_GUITBLU
        GH_GUITGRE
        GH_GUITYEL 
	
	GHguitarstrumup()
	GHguitarstrumdn()
	GHguitarwhammy()
	GHguitartouchbar()
	(GHguitartb contains raw output)
	
*/ 
 
#include <Arduino.h>

// array to store 6 bytes of data back from the GH instrument
static uint8_t Wii_GH_buf[6];

// Created a new function to send a byte to a particular address
// the aim is to make the code clearer
static void wire_send_byte(byte address, byte data)
{
	Wire.write(address);
	Wire.write(data);
}

static void Wii_GH_setpowerpins()
{
#define pwrpin PORTC3
#define gndpin PORTC2
    DDRC |= _BV(pwrpin) | _BV(gndpin);
    PORTC &=~ _BV(gndpin);
    PORTC |=  _BV(pwrpin);
    delay(100);  // wait for things to stabilize        
}

// initialize the I2C system, join the I2C bus,
// and tell the instrument we're talking to it
static void Wii_GH_init()
{
    Wire.begin();                // join i2c bus as master
    #define TWI_FREQ_NUNCHUCK 200000L // doesn't seem to work any faster!
	TWBR = ((F_CPU / TWI_FREQ_NUNCHUCK) - 16) / 2;
	Wire.beginTransmission(0x52);// transmit to device 0x52
    wire_send_byte(0xf0,0x55);// sends 0x55 to address 0xf0
	wire_send_byte(0xfb,0x00);// sends 0x00 to address 0xfb
	Wire.endTransmission();// stop transmitting
	delay(1);
}

static void Wii_GH_request_type()
{
	Wire.beginTransmission(0x52);
	Wire.write(0xfa);// send byte to point to address 0xfa
	Wire.endTransmission();
}

static void Wii_GH_send_request()
{
    Wire.beginTransmission(0x52);// transmit to device 0x52
    Wire.write(0x00);// send byte to point to address 0x00
    Wire.endTransmission();// stop transmitting
}

// Receive data back from the instrument 
// returns 1 on successful read. returns 0 on failure
static int Wii_GH_get_data()
{
    int cnt=0;
    Wire.requestFrom (0x52, 6);// request data from nunchuck
    while (Wire.available ()) {
        // receive byte as an integer
        Wii_GH_buf[cnt] = Wire.read();
        cnt++;
    }
    Wii_GH_send_request();  // send request for next data payload
    // If we received the 6 bytes, then go print them
    if (cnt >= 5) {
        return 1;   // success
    }
    return 0; //failure
}

// Two functions to print out the raw data received
// in hex or binary format

static void Wii_GH_print_raw_data_HEX()
{
	Serial.print(Wii_GH_buf[0],HEX);
	Serial.print(" ");
	Serial.print(Wii_GH_buf[1],HEX);
	Serial.print(" ");
	Serial.print(Wii_GH_buf[2],HEX);
	Serial.print(" ");
	Serial.print(Wii_GH_buf[3],HEX);
	Serial.print(" ");
	Serial.print(Wii_GH_buf[4],HEX);
	Serial.print(" ");
	Serial.print(Wii_GH_buf[5],HEX);
	Serial.print("\n");
}

static void Wii_GH_print_raw_data_BIN()
{
    Serial.print(Wii_GH_buf[0],BIN);
	Serial.print("\t");
	Serial.print(Wii_GH_buf[1],BIN);
	Serial.print("\t");
	Serial.print(Wii_GH_buf[2],BIN);
	Serial.print("\t");
	Serial.print(Wii_GH_buf[3],BIN);
	Serial.print("\t");
	Serial.print(Wii_GH_buf[4],BIN);
	Serial.print("\t");
	Serial.print(Wii_GH_buf[5],BIN);
	Serial.print("\t");
    Serial.print("\r\n");  // newline
}


/*
 
 Wii Guitar Hero Drums
 
 Send a byte of address 0xfa and the 6 bytes returned should be (in HEX):
 01 00 A4 20 01 03
 
 Data format from http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Guitar_Hero_World_Tour_%28Wii%29_Drums
 
         Bit 7 |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 Byte 0:   0       0    <---------------- SX ------------------------->
 Byte 1:   0       0    <---------------- SY ------------------------->
 Byte 2:  HHP    None   <------------ Which ------------------->  ????
 Byte 3: <--- Softness -------> <------- 0110 ----------------->  ????
 Byte 4: ????      1        1      B-      1       B+      1      ????
 Byte 5: Oran     Red     Yell    Gree    Blue    Bass     1       1     
 
 Red, Blue, and Gree are the drum pads by colour (Red, Blue, and Green).
 Oran and Yell are the cymbals.
 Bass is the pedal.
 B- and B+ are the black - and + buttons behind the Wii Remote. They will be 0 when hit, 1 when not.

 SX and SY are the black analog stick behind the Wii Remote. 0x20 means centered.

 None will be 0 if there is velocity data, and 1 if there is none.
 If there is none, bytes 2 and 3 will be FF FF and contain no data, and all the ??s will be 1.

 If there is velocity data, then "Which" tells you which pad it is for:
    Pedal  =  11011 = 27 dec, 0x1B
    Red    =  11001 = 25 dec, 0x19
    Yellow =  10001 = 17 dec, 0x11
    Blue   =  01111 = 15 dec, 0x0F
    Orange =  01110 = 14 dec, 0x0E
    Green  =  10010 = 18 dec, 0x12

"Softness" is how hard or soft you hit the pad. It ranges from 0 = Very hard to 6 = very soft, with 7 = not hit at all
"HHP" is 0 if the velocity data is for the hi-hat pedal (unmarked 3.5mm jack above bass pedal jack), and 1 otherwise. When hi-hat pedal data is sent, "Which" is set as it is for the bass drum pedal (ie 11011). The velocity varies according to how far the pedal is pressed. The pedal to connect to the jack is not the same as the bass drum pedal; it must be a some sort of variable resistor (varying between 20k Ohms when down and 40 Ohms when up seems to give a good spread of velocity values from 0 to 7). The messages are not always sent and are delayed by around 35ms (probably due to the home-made pedal I'm using working wrong).
"0110" is 0110 if there is velocity information, or 1111 if there is not. Its meaning is unknown.
The data in the "??" bits is also unknown, although they are always 1 if there is no velocity data.

My notes:
The buttons and analog stick data is common to the drums, so I've created a general function
which is not instrument specific.

I've found that the single bits remain triggered for some time after hit, so am using the "Which" information in byte 2
together with the "Softness" information in byte 3 
 */

static byte GH_DRUMPED = 0x1B;
static byte GH_DRUMRED = 0x19;
static byte GH_DRUMYEL = 0x11; 
static byte GH_DRUMBLU = 0x0F;
static byte GH_DRUMORA = 0x0E;
static byte GH_DRUMGRE = 0x12;
 
static byte GHdrumhit()
{
	return ((Wii_GH_buf[2] >> 1) & 0x1F);
}

static byte GHdrumhitstrength()
{
	return ((Wii_GH_buf[3] >> 5) & 0x07);
}

// Returns x position of joystick 0x20 is centered
static byte GHjoyx()
{
	return (Wii_GH_buf[0] & 0x3F);
}

// Returns y position of joystick 0x20 is centered
static byte GHjoyy()
{
	return (Wii_GH_buf[1] & 0x3F);
}

// Returns 0 if button plus is pressed
static byte GHbutplus()
{
	return ((Wii_GH_buf[4] >> 4) & 0x01);
}

// Returns 0 if button minus is pressed
static byte GHbutminus()
{
	return ((Wii_GH_buf[4] >> 2) & 0x01);
}

/*
 
 Wii Guitar Hero Guitar
 
 Send a byte of address 0xfa and the 6 bytes returned should be (in HEX):
 00 00 A4 20 01 03
 
 Data format from http://wiibrew.org/wiki/Wiimote/Extension_Controllers/Guitar_Hero_%28Wii%29_Guitars
 
         Bit 7 |   6   |   5   |   4   |   3   |   2   |   1   |   0   |
 Byte 0:  GH3     GH3   <---------------- SX ------------------------->
 Byte 1:  GH3     GH3   <---------------- SY ------------------------->
 Byte 2:   0       0       0    <------------  Touchbar -------------->
 Byte 3:   0       0       0    <------------  Whammy bar ------------>
 Byte 4:   1    StrumDn    1       B-      1       B+      1       1
 Byte 5: Oran     Red     Blue    Gree    Yell     1       1    StrumUp     
 
 StrumUp and StrumDn are up and down on the strum bar
 Whammy bar is the analog whammy bar

 Touchbar is the analog touchbar on the neck of GH World Tour guitars:
   Not touching it  -  0F
   1st (top) fret   -  04
   1st AND 2nd      -  07
   2nd              -  0A
   2nd AND 3rd      -  0C/0D (keeps changing)
   3rd              -  12/13
   3rd AND 4th      -  14/15
   4th              -  17/18
   4th and 5th      -  1A
   5th (bottom)     -  1F
 You can hold two (touchbar) frets at once, as long as those two (touchbar) frets are adjacent,
 otherwise it will take the value of the lowest fret held (Eg, if you hold the highest and lowest fret, it reads 1F)
 Note that high/low means physically on the guitar neck (in musical terms I think it's the other way around, so I thought I'd better specify) 
 
 B- and B+ are the black - and + buttons behind the Wii Remote. They will be 0 when hit, 1 when not.
 SX and SY are the black analog stick behind the Wii Remote. 0x20 means centered.
 
My notes:
The buttons and analog stick data is common to the drums, so I've created a general function
which is not instrument specific.

The whammy bar is centred on 0x00 and rises as pressed down?

 */

static byte GH_GUITORA = 0x10;
static byte GH_GUITRED = 0x08;
static byte GH_GUITBLU = 0x04;
static byte GH_GUITGRE = 0x02;
static byte GH_GUITYEL = 0x01; 
 
static byte GHguitarbutton()
{
	return ((Wii_GH_buf[5] >> 3) & 0x1F);
} 

static byte GHguitarstrumup()
{
	return (Wii_GH_buf[5] & 0x01);
}

static byte GHguitarstrumdn()
{
	return ((Wii_GH_buf[4] >> 6) & 0x01);
}

static byte GHguitarwhammy()
// Whammy bar seems to fluctuate around B01111 and B10000 at rest
// and increases to B11001 when fully pressed
// so instead of reading 5 bits, I am just reading the last 4
// and setting B1111 to zero
{
	if ((Wii_GH_buf[3] & 0x0F) == 0x0F) return 0;
	return (Wii_GH_buf[3] & 0x0F);
}

// Guitar Touch bar function, but better referenced through next function
static byte GHguitartb()
{
	return (Wii_GH_buf[2] & 0x1F);
}

// Guitar Touch bar better function
static byte GHguitartouchbar()
{	
	if (GHguitartb() == 0x04) return B10000;
	if (GHguitartb() == 0x07) return B11000;
	if (GHguitartb() == 0x0A) return B01000;
	if (GHguitartb() == 0x0C) return B01100;
	if (GHguitartb() == 0x0D) return B01100;
	if (GHguitartb() == 0x0F) return B00000;
	if (GHguitartb() == 0x12) return B00100;
	if (GHguitartb() == 0x13) return B00100;
	if (GHguitartb() == 0x14) return B00110;
	if (GHguitartb() == 0x15) return B00110;
	if (GHguitartb() == 0x17) return B00010;
	if (GHguitartb() == 0x18) return B00010;
	if (GHguitartb() == 0x1A) return B00011;
	if (GHguitartb() == 0x1F) return B00001;
	return 0;
}

// END OF GH FUNCTIONS
/*
 * Wii Guitar Hero Guitar and Drum code
 */

#include <Wire.h>
#include "wii_GH_funcs.h"
#include <MIDI.h>

static int PLAYTIME = 5;
static int HITVOL = 12;
static int HITMIN = 31;
static int MYDEBUG = 1;
int MCHAN = 9;

static int BASDRUM = 36;
static int YELDRUM = 42;
static int REDDRUM = 38;
static int ORADRUM = 51;
static int BLUDRUM = 45;
static int GREDRUM = 50;

int noteplayed[9];
int note2played[9];
int note3played[9];
int notevolume[9];

int gnotetoplay;
int gnote2toplay;
int gnote3toplay;
int gnotetoplayvol;
 
int loop_cnt=0;
int whathit=0;
int howhard=0;
int ledPin = 13;
int instrument = 0;
int notetoplay = 0;
int laststrumstate = 0;
int curstrumstate = 0;

int gnoteplaying = 0;
int gnote2playing = 0;
int gnote3playing = 0;
int gnoteplayingvol = 0;

int guitarstrummed = 0;
int guitarbend = 0;
int targetguitarbend = 0;
String lastbuttonpressed = "None";
String curbuttonpressed = "None";
int whammybar = 0;

static int GRENOTE = 67 - 12; // G
static int GRESTEP1 = 4;
static int GRESTEP2 = 3;

static int REDNOTE = 63 - 12; // E flat
static int REDSTEP1 = 4;
static int REDSTEP2 = 3;

static int YELNOTE = 60 - 12; // Middle C
static int YELSTEP1 = 4;
static int YELSTEP2 = 3;

static int BLUNOTE = 70 - 12; // B flat
static int BLUSTEP1 = 4;
static int BLUSTEP2 = 3;

static int ORANOTE = 72 - 12; // C 1 octave above Middle C
static int ORASTEP1 = 4;
static int ORASTEP2 = 3;

void setup()
{
    //Serial.begin(115200);  //send at MIDI rate of 31250
    Wii_GH_setpowerpins();
    delay(1000);
        
    Wii_GH_init(); // send the initilization handshake
    delay(1000);
    
    Wii_GH_request_type();
    
    Wii_GH_get_data();
    
    if (Wii_GH_buf[0] == 0x00)
    {
      if (Wii_GH_buf[2] == 0xA4)
      {
      instrument = 1;
      MCHAN = 5; //midi channel 6
      if (MYDEBUG == 0) MIDI.begin(MCHAN);
      Serial.begin(115200);
      delay(100);
      if (MYDEBUG == 0) MIDI.sendProgramChange(37, MCHAN); // Select guitar program 37
      // Guitar detected...
      }
    }
    else if (Wii_GH_buf[0] = 0x01)
    {
      instrument = 2;
      MCHAN = 9; //midi channel 10
      MIDI.begin(MCHAN);
      Serial.begin(115200);
      // Drums detected...
    }
    Wii_GH_send_request();
    if (MYDEBUG != 0) Serial.print("Detection completed. Ready to receive input\n");
}

void loop()
{
  
  notetoplay = 0;

  // Get data from the instrument

  Wii_GH_get_data();
  
  // Process data received
  // Drums    
  if (instrument == 2)
  {   
      
      whathit = GHdrumhit();
      howhard = (8 - GHdrumhitstrength()); // 8 = max volume, 1 = min volume
      
      /*
      Serial.print(whathit,BIN);
      Serial.print("\t");
      */
      
      if (whathit == GH_DRUMPED)
      {
      notetoplay = 1;
      noteplayed[loop_cnt] = BASDRUM;
      notevolume[loop_cnt] = howhard * HITVOL + HITMIN;
      }
      
      if (whathit == GH_DRUMRED)
      {
      notetoplay = 1;
      noteplayed[loop_cnt] = REDDRUM;
      notevolume[loop_cnt] = howhard * HITVOL + HITMIN;
      }
      
      if (whathit == GH_DRUMYEL)
      {
      notetoplay = 1;
      noteplayed[loop_cnt] = YELDRUM;
      notevolume[loop_cnt] = howhard * HITVOL + HITMIN;  
      }
      
      if (whathit == GH_DRUMBLU)
      {
      notetoplay = 1;
      noteplayed[loop_cnt] = BLUDRUM;
      notevolume[loop_cnt] = howhard * HITVOL + HITMIN;
      }
      
      if (whathit == GH_DRUMORA)
      {
      notetoplay = 1;  
      noteplayed[loop_cnt] = ORADRUM;
      notevolume[loop_cnt] = howhard * HITVOL + HITMIN;
      }
      
      if (whathit == GH_DRUMGRE)
      {
      notetoplay = 1;  
      noteplayed[loop_cnt] = GREDRUM;
      notevolume[loop_cnt] = howhard * HITVOL + HITMIN;
      }

      // Play note
      if (notetoplay = 1)
      {
      MIDI.sendNoteOn(noteplayed[loop_cnt],notevolume[loop_cnt],MCHAN);
      loop_cnt++;
      if (loop_cnt > 9) loop_cnt = 0;
      if (noteplayed[loop_cnt] > 0)
      {
        MIDI.sendNoteOff(noteplayed[loop_cnt],notevolume[loop_cnt],MCHAN);
        noteplayed[loop_cnt] = 0;
        notevolume[loop_cnt] = 0;
      }  // end of if noteplayed[] > 0
      }  // end of if notetoplay = 1

  }
  // End of If ... drums

  // If guitar    
  
  if (instrument == 1)
     {
     curstrumstate = 0;
     
     if (GHguitarstrumup() == 0) curstrumstate = 1;
     if (GHguitarstrumdn() == 0) curstrumstate = -1;
     if (MYDEBUG != 0) {
     Serial.print("curstrumstate = ");
     Serial.print(curstrumstate,DEC);
     Serial.print(" ... ");
     }
     
     guitarstrummed = 0;
     if ((curstrumstate != laststrumstate) & (curstrumstate != 0)) 
       {
       if (MYDEBUG != 0) {
       Serial.print("Change of state");
       Serial.print("laststrumstate = ");
       Serial.print(laststrumstate,DEC);
       Serial.print(" ... ");
       }
       guitarstrummed = 1;
     }

	 gnotetoplay = 0;
     curbuttonpressed = "None";
     if ((GHguitarbutton() & GH_GUITGRE) == 0) curbuttonpressed = "Green";
     if ((GHguitarbutton() & GH_GUITRED) == 0) curbuttonpressed = "Red";
     if ((GHguitarbutton() & GH_GUITYEL) == 0) curbuttonpressed = "Yellow";
     if ((GHguitarbutton() & GH_GUITBLU) == 0) curbuttonpressed = "Blue";
     if ((GHguitarbutton() & GH_GUITORA) == 0) curbuttonpressed = "Orange";

     // Guitar is strummed
     if ((guitarstrummed == 1) && (curbuttonpressed == "Green"))
         {
         //midi_comment("Green detected");
         gnotetoplay = GRENOTE;
         gnote2toplay = GRENOTE + GRESTEP1;
         gnote3toplay = GRENOTE + GRESTEP1 + GRESTEP2;
         gnotetoplayvol = 96;
         }
       
       if ((guitarstrummed == 1) && (curbuttonpressed == "Red"))
         {
         gnotetoplay = REDNOTE;
         gnote2toplay = REDNOTE + REDSTEP1;
         gnote3toplay = REDNOTE + REDSTEP1 + REDSTEP2;
         gnotetoplayvol = 96;
         }
       
       if ((guitarstrummed == 1) && (curbuttonpressed == "Yellow"))        
         {
         gnotetoplay = YELNOTE;
         gnote2toplay = YELNOTE + YELSTEP1;
         gnote3toplay = YELNOTE + YELSTEP1 + YELSTEP2;
         gnotetoplayvol = 96;
         }
       
       if ((guitarstrummed == 1) && (curbuttonpressed == "Blue"))
         {
         gnotetoplay = BLUNOTE;
         gnote2toplay = BLUNOTE + BLUSTEP1;
         gnote3toplay = BLUNOTE + BLUSTEP1 + BLUSTEP2;
         gnotetoplayvol = 96;
         }
       
       if ((guitarstrummed == 1) && (curbuttonpressed == "Orange")) 
         {
         gnotetoplay = ORANOTE;
         gnote2toplay = ORANOTE + ORASTEP1;
         gnote3toplay = ORANOTE + ORASTEP1 + ORASTEP2;
         gnotetoplayvol = 96;
         }          
       
       //} // end of if strumstate ... 

     laststrumstate = curstrumstate;
     
     if ((curbuttonpressed == "None") && (lastbuttonpressed != "None"))
       {
       // Check that any notes playing are stopped
       if (gnoteplaying > 0)
         {
         if (MYDEBUG != 0) {
         Serial.print("Clearing out old notes of value ");
         Serial.print(gnoteplaying,DEC);
         Serial.print("\n");
         }
         if (MYDEBUG == 0) {
         MIDI.sendNoteOff(gnoteplaying,gnoteplayingvol,MCHAN);
         MIDI.sendNoteOff(gnote2playing,gnoteplayingvol,MCHAN);
         MIDI.sendNoteOff(gnote3playing,gnoteplayingvol,MCHAN);
         }
         gnoteplaying = 0;
         gnote2playing = 0;
         gnote3playing = 0;
         gnoteplayingvol = 0;
         }
       }
     lastbuttonpressed = curbuttonpressed;

     // Play notes
     if (gnotetoplay > 0)
       {
 
       if (MYDEBUG != 0) {
       Serial.print("Should be playing a note of value ");
       Serial.print(gnotetoplay,DEC);
       Serial.print("\n");
       }
       if (MYDEBUG == 0) {
       MIDI.sendNoteOn(gnotetoplay,gnotetoplayvol,MCHAN);
       MIDI.sendNoteOn(gnote2toplay,gnotetoplayvol,MCHAN);
       MIDI.sendNoteOn(gnote3toplay,gnotetoplayvol,MCHAN);
       gnoteplaying = gnotetoplay;
	   gnote2playing = gnote2toplay;
	   gnote3playing = gnote3toplay;
	   gnoteplayingvol = gnotetoplayvol;
       }
       
       }  // end of if notetoplay = 1

      if ((GHguitarwhammy() > 0) && (gnoteplaying > 0))
      {
      targetguitarbend = GHguitarwhammy() * 256; 
      }

      if ((gnoteplaying > 0) and (guitarbend != targetguitarbend))
      {
      MIDI.sendPitchBend(targetguitarbend - guitarbend,MCHAN);
      guitarbend = targetguitarbend;
      }

   } // end of instrument = guitar

delay(1);  // delay for 1 ms

}

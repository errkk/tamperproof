#include <Adafruit_NeoPixel.h>

#ifdef __AVR__
  #include <avr/power.h>
#endif

 
// Pattern types supported:
enum  pattern { NONE, RAINBOW_CYCLE, THEATER_CHASE, COLOR_WIPE, SCANNER, FADE };
// Patern directions supported:
enum  direction { FORWARD, REVERSE };
 
// NeoPattern Class - derived from the Adafruit_NeoPixel class
class NeoPatterns : public Adafruit_NeoPixel
{
    public:
 
    // Member Variables:  
    pattern  ActivePattern;  // which pattern is running
    direction Direction;     // direction to run the pattern
    
    unsigned long Interval;   // milliseconds between updates
    unsigned long lastUpdate; // last update of position
    
    uint32_t Color1, Color2;  // What colors are in use
    uint16_t TotalSteps;  // total number of steps in the pattern
    uint16_t Index;  // current step within the pattern
    
    void (*OnComplete)();  // Callback on completion of pattern
    
    // Constructor - calls base-class constructor to initialize strip
    NeoPatterns(uint16_t pixels, uint8_t pin, uint8_t type, void (*callback)())
    :Adafruit_NeoPixel(pixels, pin, type)
    {
        OnComplete = callback;
    }
    
    // Update the pattern
    void Update()
    {
        if((millis() - lastUpdate) > Interval) // time to update
        {
            lastUpdate = millis();
            ScannerUpdate();
        }
    }
  
    // Increment the Index and reset at the end
    void Increment()
    {
       Index++;
       if (Index >= TotalSteps)
        {
            Index = 0;
            if (OnComplete != NULL)
            {
                OnComplete(); // call the comlpetion callback
            }
        }
    }
    
    
    // Initialize for a SCANNNER
    void Scanner(uint32_t color1, uint8_t interval)
    {
        ActivePattern = SCANNER;
        Interval = interval;
        TotalSteps = numPixels();
        Color1 = color1;
        Index = 0;
    }
 
    // Update the Scanner Pattern
    void ScannerUpdate()
    { 
        for (int i = 0; i < numPixels(); i++)
        {
            if (i == Index)  // Scan Pixel to the right
            {
                 setPixelColor(i, Color1);
            }
            else // Fading tail
            {
                 setPixelColor(i, DimColor(getPixelColor(i)));
            }
        }
        show();
        Increment();
    }
   
    // Calculate 50% dimmed version of a color (used by ScannerUpdate)
    uint32_t DimColor(uint32_t color)
    {
        // Shift R, G and B components one bit to the right
        uint32_t dimColor = Color(Red(color) >> 1, Green(color) >> 1, Blue(color) >> 1);
        return dimColor;
    }
 
    // Set all pixels to a color (synchronously)
    void ColorSet(uint32_t color)
    {
        for (int i = 0; i < numPixels(); i++)
        {
            setPixelColor(i, color);
        }
        show();
    }
 
    // Returns the Red component of a 32-bit color
    uint8_t Red(uint32_t color)
    {
        return (color >> 16) & 0xFF;
    }
 
    // Returns the Green component of a 32-bit color
    uint8_t Green(uint32_t color)
    {
        return (color >> 8) & 0xFF;
    }
 
    // Returns the Blue component of a 32-bit color
    uint8_t Blue(uint32_t color)
    {
        return color & 0xFF;
    }
    
    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
        WheelPos = 255 - WheelPos;
        if(WheelPos < 85)
        {
            return Color(255 - WheelPos * 3, 0, WheelPos * 3);
        }
        else if(WheelPos < 170)
        {
            WheelPos -= 85;
            return Color(0, WheelPos * 3, 255 - WheelPos * 3);
        }
        else
        {
            WheelPos -= 170;
            return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
        }
    }
};

#define PIXELS_PIN 2
NeoPatterns Ring(12, PIXELS_PIN, NEO_GRB + NEO_KHZ800, &RingComplete);


#include <Manchester.h>

/*

  Transmitter will send one byte of data and 4bit sender ID per transmittion
  message also contains a checksum and receiver can check if the data has been transmited correctly

*/

#define TX_PIN 0  //pin where your transmitter is connected
#define SENDER_ID 5

#define POT_PIN 4 // updated to match pcb
#define BUZZER_PIN 1


int WheelPos = 0;
bool alarmMode = 0;
bool flipped = 0;
bool sentDistressSignal = 0;
int timeout;
int potVal;
 

// Initialize everything and prepare to start
void setup()
{ 
    Ring.begin();   
    Ring.Scanner(Ring.Color(255,0,128), 60);

    man.workAround1MhzTinyCore(); //add this in order for transmitter to work with 1Mhz Attiny85/84
    man.setupTransmit(TX_PIN, MAN_1200);

    #if defined (__AVR_ATtiny85__)
      if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
    #endif

    // Get timeout value from POT
    pinMode(POT_PIN, INPUT);
    potVal = analogRead(POT_PIN);
    timeout = map(potVal, 0, 254, 10000, 30000);

    pinMode(BUZZER_PIN, OUTPUT);
}

// Main loop
void loop()
{
  Ring.Update(); 

  if(millis() > timeout) {
    alarmMode = 1; 
  }

  if(alarmMode) {
    if(!sentDistressSignal) {
      man.transmit(man.encodeMessage(SENDER_ID, (byte)potVal));
      sentDistressSignal = 1;
    }
    
    // Speed up animation
    Ring.Interval = 20;
  }
}

void RingComplete(void) {
    if(alarmMode) {
      flipped = !flipped;
  
      if(flipped){
        Ring.Color1 = Ring.Color(255, 0, 0);
        
        // play a note on pin  for 200 ms:
        tone(BUZZER_PIN, 440, 500);  
      } else {
        Ring.Color1 = Ring.Color(0, 0, 255);  
        noTone(BUZZER_PIN);
      }
    }
}


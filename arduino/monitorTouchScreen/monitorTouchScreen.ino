#include <Adafruit_GFX.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <SD.h>
#include "TouchScreen.h"
#include "HID-Project.h"
#define YP A2
#define XM A3
#define YM 8
#define XP 9
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940
#define MINPRESSURE 10
#define MAXPRESSURE 1000
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define SD_CS 4

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

String inputString = "";
boolean stringComplete = false;
int lite = 11;
int reset = 13;

int ledLength = 8;
int traySwitch = A1;
int lightStatus = 0;
int trayClosed = 0;
int trayStatus = 0;

#define PIN 5
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, PIN, NEO_GRB + NEO_KHZ800);

void setup(void) {


	strip.begin();
	strip.setBrightness(64);
  	strip.show(); 
  	turnLeds();
  	delay(5000);
  	turnLeds();

	pinMode(lite, OUTPUT);
	pinMode(reset, OUTPUT);
	digitalWrite(reset, LOW);
	delay(1000);
	Serial.begin(9600);
	
	inputString.reserve(200);
	
	tft.begin();
	tft.fillScreen(ILI9341_BLACK);

	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed!");
	};

    tft.setRotation(3);
	
	tft.setCursor(10, 10);
  	tft.setTextColor(ILI9341_DARKGREEN);  
  	tft.setTextSize(3);
  	tft.println("CPU");

  	tft.setCursor(90, 10);
  	tft.setTextColor(ILI9341_ORANGE);
  	tft.setTextSize(3);
  	tft.println("GPU");

  	tft.setCursor(170, 10);
  	tft.setTextColor(ILI9341_DARKCYAN);
  	tft.setTextSize(3);
  	tft.println("RAM");

  	tft.setTextSize(1);
  	tft.setTextColor(ILI9341_WHITE);
  	
  	tft.setCursor(20, 80);
	tft.println("PREV");

	tft.setCursor(100, 80);
	tft.println("NEXT");
	
	tft.setCursor(180, 80);
	tft.println("PLAY");

	tft.setCursor(260, 80);
	tft.println("VOL UP");

	tft.setCursor(260, 215);
	tft.println("VOL DOWN");

	tft.setCursor(180, 215);
	tft.println("MUTE");

	tft.setCursor(20, 215);
	tft.println("LIGHTS");


	analogWrite(lite, 50);

}
unsigned long lastTime = 0;

//vars for mem keeping

int cpuStringStart, cpuStringLimit, cpuDivisor;
String cpuString, cpuTemp, cpuUsage;

int gpuStringStart, gpuStringLimit, gpuDivisor;
String gpuString, gpuTemp, gpuUsage;

int songStringStart, songStringLimit;
String songString, currentSong;

void loop()
{
	
  	TSPoint p = ts.getPoint();
 	serialEvent();
  	antiBurn(millis());



   	if (stringComplete) {
    	Serial.println(inputString);
		tft.setTextSize(2);
		tft.setTextColor(ILI9341_DARKGREEN);

		tft.fillRect(0, 35, 70, 40, ILI9341_BLACK);
		tft.setCursor(10,35);
		cpuStringStart = inputString.indexOf("C");
		cpuStringLimit = inputString.indexOf("|");
		cpuString = inputString.substring(cpuStringStart+1, cpuStringLimit);
		cpuDivisor = cpuString.indexOf(" ");
		cpuTemp =  cpuString.substring(0, cpuDivisor);
		cpuUsage = cpuString.substring(cpuDivisor + 1);
		tft.println(cpuTemp);
		tft.setCursor(10, 55);
		tft.println(cpuUsage);
		
		tft.fillRect(90, 35, 70, 40, ILI9341_BLACK);
		tft.setCursor(90,35);
		tft.setTextColor(ILI9341_ORANGE);
		gpuStringStart = inputString.indexOf("G", cpuStringLimit);
		gpuStringLimit = inputString.indexOf("|", gpuStringStart);
		gpuString = inputString.substring(gpuStringStart+1 ,gpuStringLimit);
		gpuDivisor = gpuString.indexOf(" ");
		gpuTemp =  gpuString.substring(0, gpuDivisor);
		gpuUsage = gpuString.substring(gpuDivisor + 1);
		tft.println(gpuTemp);
		tft.setCursor(90, 55);
		tft.println(gpuUsage);

		
		tft.fillRect(170, 35, 70, 40, ILI9341_BLACK);
		tft.setCursor(170,35);
		tft.setTextColor(ILI9341_DARKCYAN);
		int ramStringStart = inputString.indexOf("R", gpuStringLimit);
		int ramStringLimit = inputString.indexOf("|", ramStringStart);
		String ramString = inputString.substring(ramStringStart+1 ,ramStringLimit);
		int ramDivisor = ramString.indexOf(" ");
		String ramTotal =  ramString.substring(0, ramDivisor);
		String ramUsage = ramString.substring(ramDivisor + 1);
		tft.println(ramString);
		
		
		songStringStart = inputString.indexOf("S", ramStringLimit);
		songStringLimit = inputString.indexOf("|", songStringStart);
		songString = inputString.substring(songStringStart+1, songStringLimit);

		if ( songString != currentSong ){
			tft.fillRect(0, 130, 320, 80, ILI9341_BLACK);
			tft.setTextSize(2);
			tft.setCursor(0,130);
			tft.setTextColor(ILI9341_MAGENTA);
			tft.println(songString);
			currentSong = songString;
		};
		
		
		inputString = "";
		stringComplete = false;
	}
  	if (p.z < MINPRESSURE || p.z > MAXPRESSURE) {
    	return;
  	}

  
  	// Scale from ~0->1000 to tft.width using the calibration #'s
	p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
	p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  
  /*Serial.print("("); Serial.print(p.x);
  Serial.print(", "); Serial.print(p.y);
  Serial.println(")");*/
  
	if ( ( millis() - lastTime ) > 500 ){
	  	lastTime = millis();
	  	if ( p.x < 120 ){
	  		if ( p.y < 57 ){
	  			Consumer.write(MEDIA_VOLUME_UP);
	  		} else if ( p.y > 57 && p.y < 114 ) {
	  			Consumer.write(MEDIA_PLAY_PAUSE);
	  		} else if ( p.y > 114 && p.y < 171 ){
	  			Consumer.write(MEDIA_NEXT);
	  		} else if ( p.y > 171 ){
	  			Consumer.write(MEDIA_PREVIOUS);
	  		};
	  	} else if ( p.x > 120 ) {
	  		if ( p.y < 57 ){
	  			Consumer.write(MEDIA_VOLUME_DOWN);
	  		} else if ( p.y > 57 && p.y < 114 ) {
	  			Consumer.write(MEDIA_VOLUME_MUTE);
	  		} else if ( p.y > 114 && p.y < 171 ){
	  			//Consumer.write(MEDIA_VOLUME_MUTE);
	  		} else if ( p.y > 171 ){
	  			turnLeds();
	  		};
	  	};
	};
};

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '|') {
      stringComplete = true;
    }
  }
}

void antiBurn(long millis){
	if ( (millis - lastTime) > 100000 ){
		analogWrite(lite, 0);
	} else {
		analogWrite(lite, 50);
	}
}

void turnLeds(){
	int ledColor1 = 0;
	int ledColor2 = 0;
	int ledColor3 = 0;
	if ( lightStatus == 0 ){
		ledColor1 = 255;
		ledColor3 = 255;
		lightStatus = 1;
	} else {
		lightStatus = 0;
	};
	for (int i = 0; i < ledLength; ++i)
	{
		strip.setPixelColor(i, ledColor1, ledColor2, ledColor3);
	};
	strip.show(); 
}

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20

void bmpDraw(char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r,g,b));
          } // end pixel
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
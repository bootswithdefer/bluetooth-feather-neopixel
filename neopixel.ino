/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

// This sketch is intended to be used with the NeoPixel control
// surface in Adafruit's Bluefruit LE Connect mobile application.
//
// - Compile and flash this sketch to the nRF52 Feather
// - Open the Bluefruit LE Connect app
// - Switch to the NeoPixel utility
// - Click the 'connect' button to establish a connection and
//   send the meta-data about the pixel layout
// - Use the NeoPixel utility to update the pixels on your device

/* NOTE: This sketch required at least version 1.1.0 of Adafruit_Neopixel !!! */

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <bluefruit.h>

#define NEOPIXEL_VERSION_STRING "Neopixel v2.0"
#define PIN                     30   /* Pin used to drive the NeoPixels */

#define MAXCOMPONENTS  4
uint8_t *pixelBuffer = NULL;
uint8_t width = 0;
uint8_t height = 0;
uint8_t stride;
uint8_t boardType;
uint8_t pixelTypeValue;
uint8_t components = 3;     // only 3 and 4 are valid values

int runCommand = -1;

Adafruit_NeoPixel neopixel = Adafruit_NeoPixel();

// BLE Service
BLEDis  bledis;
BLEUart bleuart;

void setup()
{
  Serial.begin(115200);
  Serial.println("Adafruit Bluefruit Neopixel Test");
  Serial.println("--------------------------------");

  Serial.println();
  Serial.println("Please connect using the Bluefruit Connect LE application");
  
  // Config Neopixels
  neopixel.begin();

  // Init Bluefruit
  Bluefruit.begin();
  // Set max power. Accepted values are: -40, -30, -20, -16, -12, -8, -4, 0, 4
  Bluefruit.setTxPower(4);
  Bluefruit.setName("Bluefruit52");
  Bluefruit.setConnectCallback(connect_callback);

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();  

  // Configure and start BLE UART service
  bleuart.begin();

  // Set up and start advertising
  startAdv();
}

void startAdv(void)
{  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  
  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void connect_callback(uint16_t conn_handle)
{
  char central_name[32] = { 0 };
  Bluefruit.Gap.getPeerName(conn_handle, central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);

  Serial.println("Please select the 'Neopixels' tab, click 'Connect' and have fun");
}

void loop()
{
  if (runCommand > 0) {
    loopCommand();
  }
  
  // Echo received data
  if ( Bluefruit.connected() && bleuart.notifyEnabled() )
  {
    int command = bleuart.read();

    switch (command) {
      case 'V': {   // Get Version
          commandVersion();
          break;
        }
  
      case 'S': {   // Setup dimensions, components, stride...
          commandSetup();
          break;
       }

      case 'C': {   // Clear with color
          commandClearColor();
          break;
      }

      case 'B': {   // Set Brightness
          commandSetBrightness();
          break;
      }
            
      case 'P': {   // Set Pixel
          commandSetPixel();
          break;
      }
  
      case 'I': {   // Receive new image
          commandImage();
          break;
       }

     case '0':
     case '1':
     case '2':
     case '3':
     case '4':
     case '5':
     case '6':
     case '7':
     case '8':
     case '9': {
          startCommand(command - '0');
          break;
       }

    }
  }
}

void swapBuffers()
{
  uint8_t *base_addr = pixelBuffer;
  int pixelIndex = 0;
  for (int j = 0; j < height; j++)
  {
    for (int i = 0; i < width; i++) {
      if (components == 3) {
        neopixel.setPixelColor(pixelIndex, neopixel.Color(*base_addr, *(base_addr+1), *(base_addr+2)));
      }
      else {
        neopixel.setPixelColor(pixelIndex, neopixel.Color(*base_addr, *(base_addr+1), *(base_addr+2), *(base_addr+3) ));
      }
      base_addr+=components;
      pixelIndex++;
    }
    pixelIndex += stride - width;   // Move pixelIndex to the next row (take into account the stride)
  }
  neopixel.show();

}

void commandVersion() {
  Serial.println(F("Command: Version check"));
  sendResponse(NEOPIXEL_VERSION_STRING);
}

void commandSetup() {
  Serial.println(F("Command: Setup"));

  width = bleuart.read();
  height = bleuart.read();
  components = bleuart.read();
  stride = bleuart.read();
  boardType = bleuart.read();
  pixelTypeValue = bleuart.read();

  Serial.printf("\tsize: %dx%d\n", width, height);
  Serial.printf("\tstride: %d\n", stride);
  Serial.printf("\tcomponents: %d\n", components);
  Serial.printf("\tboardType: %d\n", boardType);
  Serial.printf("\tpixelTypeValue: %d\n", pixelTypeValue);
  
  Serial.printf("RGB: %d ", NEO_RGB);
  Serial.printf("RBG: %d ", NEO_RBG);
  Serial.printf("GRB: %d ", NEO_GRB);
  Serial.printf("GBR: %d ", NEO_GBR);
  Serial.printf("BRG: %d ", NEO_BRG);
  Serial.printf("BGR: %d\n", NEO_BGR);

  neoPixelType pixelType;
  pixelType = pixelTypeValue;

  if (pixelBuffer != NULL) {
      delete[] pixelBuffer;
  }

  uint32_t size = width*height;
  pixelBuffer = new uint8_t[size*components];
  neopixel.updateLength(size);
  neopixel.updateType(boardType);
  neopixel.setPin(PIN);

  // Done
  sendResponse("OK");
}

void commandSetBrightness() {
  Serial.println(F("Command: SetBrightness"));

   // Read value
  uint8_t brightness = bleuart.read();

  // Set brightness
  neopixel.setBrightness(brightness);

  // Refresh pixels
  swapBuffers();

  // Done
  sendResponse("OK");
}

void commandClearColor() {
  Serial.println(F("Command: ClearColor"));

  // Read color
  uint8_t color[MAXCOMPONENTS];
  for (int j = 0; j < components;) {
    if (bleuart.available()) {
      color[j] = bleuart.read();
      j++;
    }
  }

  // Set all leds to color
  int size = width * height;
  uint8_t *base_addr = pixelBuffer;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < components; j++) {
      *base_addr = color[j];
      base_addr++;
    }
  }

  // Swap buffers
  Serial.println(F("ClearColor completed"));
  swapBuffers();


  if (components == 3) {
    Serial.printf("\tclear (%d, %d, %d)\n", color[0], color[1], color[2] );
  }
  else {
    Serial.printf("\tclear (%d, %d, %d, %d)\n", color[0], color[1], color[2], color[3] );
  }
  
  // Done
  sendResponse("OK");
}

void blankAll() {
  int size = width * height;
  uint8_t *base_addr = pixelBuffer;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < components; j++) {
      *base_addr = 0;
      base_addr++;
    }
  }

  swapBuffers();
}

void commandSetPixel() {
  // Read position
  uint8_t x = bleuart.read();
  uint8_t y = bleuart.read();

  Serial.printf("Command: SetPixel (%d, %d)", x, y);

  // Read colors
  uint32_t pixelOffset = y*width+x;
  uint32_t pixelDataOffset = pixelOffset*components;
  uint8_t *base_addr = pixelBuffer+pixelDataOffset;
  for (int j = 0; j < components;) {
    if (bleuart.available()) {
      *base_addr = bleuart.read();
      Serial.printf(" %d", *base_addr);
      base_addr++;
      j++;
    }
  }

  // Set colors
  uint32_t neopixelIndex = (y*stride)+x;
  Serial.printf("\tindex (%d)", neopixelIndex);
  uint8_t *pixelBufferPointer = pixelBuffer + pixelDataOffset;
  uint32_t color;
  if (components == 3) {
    color = neopixel.Color( *pixelBufferPointer, *(pixelBufferPointer+1), *(pixelBufferPointer+2) );
    Serial.printf("\tcolor (%d, %d, %d) %d\n",*pixelBufferPointer, *(pixelBufferPointer+1), *(pixelBufferPointer+2), color );
  }
  else {
    color = neopixel.Color( *pixelBufferPointer, *(pixelBufferPointer+1), *(pixelBufferPointer+2), *(pixelBufferPointer+3) );
    Serial.printf("\tcolor (%d, %d, %d, %d) %d\n", *pixelBufferPointer, *(pixelBufferPointer+1), *(pixelBufferPointer+2), *(pixelBufferPointer+3), color );    
  }
  neopixel.setPixelColor(neopixelIndex, color);
  neopixel.show();

  // Done
  sendResponse("OK");
}

void commandImage() {
  Serial.printf("Command: Image %dx%d, %d, %d\n", width, height, components, stride);
  
  // Receive new pixel buffer
  int size = width * height;
  uint8_t *base_addr = pixelBuffer;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < components;) {
      if (bleuart.available()) {
        *base_addr = bleuart.read();
        base_addr++;
        j++;
      }
    }

/*
    if (components == 3) {
      uint32_t index = i*components;
      Serial.printf("\tp%d (%d, %d, %d)\n", i, pixelBuffer[index], pixelBuffer[index+1], pixelBuffer[index+2] );
    }
    */
  }

  // Swap buffers
  Serial.println(F("Image received"));
  swapBuffers();

  // Done
  sendResponse("OK");
}

void startCommand(int cmd) {
  Serial.printf("Command: StartCommand %d\n", cmd);
  runCommand = cmd;

  switch (runCommand) {
    case -1:
      break;
    case 1:
      setupCommandOne();
      break;
    case 2:
      setupCommandTwo();
      break;
    case 3:
      setupCommandThree();
      break;
    case 4:
      setupCommandFour();
      break;
    case 5:
      setupCommandFive();
      break;
    case 6:
      setupCommandSix();
      break;
    case 7:
      setupCommandSeven();
      break;
    case 8:
      setupCommandEight();
      break;
    case 9:
      setupCommandNine();
      break;
    case 0:
      setupCommandTen();
      break;
    default:
      Serial.printf("runCommand unknown %d", runCommand);
      break;
  }
  
  sendResponse("OK");
}

void loopCommand() {
  switch (runCommand) {
    case -1:
      break;
    case 1:
      loopCommandOne();
      break;
    case 2:
      loopCommandTwo();
      break;
    case 3:
      loopCommandThree();
      break;
    case 4:
      loopCommandFour();
      break;
    case 5:
      loopCommandFive();
      break;
    case 6:
      loopCommandSix();
      break;
    case 7:
      loopCommandSeven();
      break;
    case 8:
      loopCommandEight();
      break;
    case 9:
      loopCommandNine();
      break;
    case 0:
      loopCommandTen();
      break;
    default:
      Serial.printf("runCommand unknown %d", runCommand);
      break;
  }
}

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(4, 8, PIN,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB            + NEO_KHZ800);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };

int x;
int pass;

void setupCommandOne() {
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(30);
  matrix.setTextColor(colors[0]);
  pass = 0;
  x = matrix.width();
}

void loopCommandOne() {
  matrix.fillScreen(0);
  matrix.setCursor(x, 0);
  matrix.print(F("A S U"));
  if(--x < -36) {
    x = matrix.width();
    if(++pass >= 3) pass = 0;
    matrix.setTextColor(colors[pass]);
  }
  matrix.show();
  delay(70);
}

void setupCommandTwo() {
  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(30);
  matrix.setTextColor(colors[0]);
  pass = 0;
  x = matrix.width();
}

void loopCommandTwo() {
    matrix.fillScreen(0);
  matrix.setCursor(x, 0);
  matrix.print(F("reInvent"));
  if(--x < -36) {
    x = matrix.width();
    if(++pass >= 3) pass = 0;
    matrix.setTextColor(colors[pass]);
  }
  matrix.show();
  delay(50);
}

void setupCommandThree() {
  blankAll();
}

void loopCommandThree() {
}

void setupCommandFour() {
  blankAll();
}

void loopCommandFour() {
}

void setupCommandFive() {
  blankAll();
}

void loopCommandFive() {
}

void setupCommandSix() {
  blankAll();
}

void loopCommandSix() {
}

void setupCommandSeven() {
  blankAll();
}

void loopCommandSeven() {
}

void setupCommandEight() {
  blankAll();
}

void loopCommandEight() {
}

void setupCommandNine() {
  blankAll();
}

void loopCommandNine() {
}

void setupCommandTen() {
  blankAll();
}

void loopCommandTen() {
}

void sendResponse(char const *response) {
    Serial.printf("Send Response: %s\n", response);
    bleuart.write(response, strlen(response)*sizeof(char));
}


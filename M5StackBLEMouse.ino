#include <M5Stack.h>
#include "BLEDevice.h"
#include "BLEServer.h"
#include "BLEUtils.h"
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDKeyboardTypes.h"
#include "sdkconfig.h"

static BLEHIDDevice* hid;
BLECharacteristic* input2;
BLECharacteristic* output2;
BLECharacteristic* input1;
BLECharacteristic* output1;
bool isConnected = false;

//to handle single...triple click
unsigned long tsStart = 0;
unsigned char sequentialCount = 0;

//move mode, 1: Up-Down mode, -1: Right-Left mode.
char movingMode = -1;

void DisplayConnectionText() {
  int yStart = 20;
  int height = 10;
  const char *text = isConnected ? "Connected" : "Not connected.";
  M5.Lcd.fillRect(0, yStart, 320, height, BLACK);
  M5.Lcd.drawString(text, 10, yStart);
}

void DisplayStatusText(String text) {
  int yStart = 60;
  int height = 10;
  M5.Lcd.fillRect(0, yStart, 320, height, BLACK);
  M5.Lcd.drawString(text, 10, yStart);  
}

void DisplayGuide() {
  int yStart = 180;
  int height = 10;
  M5.Lcd.fillRect(0, yStart, 320, height, BLACK);
  M5.Lcd.drawString("[BTN A]", 46, yStart);
  M5.Lcd.drawString("[BTN B]", 138, yStart);
  M5.Lcd.drawString("[BTN C]", 231, yStart);
  yStart = 195;
  const String text = movingMode == 1 ? "Up-Down" : "Left-Right";
  M5.Lcd.fillRect(0, yStart, 320, height, BLACK);
  M5.Lcd.drawString(movingMode == 1 ? "1Clk: Up-Down" : "1Clk: Left-Right", 10, yStart);
  M5.Lcd.drawString(movingMode == 1 ? "Up" : "Left", 150, yStart);
  M5.Lcd.drawString(movingMode == 1 ? "Down" : "Right", 240, yStart);

  yStart = 210;
  M5.Lcd.drawString("2Clk: Left Click", 10, yStart);
  yStart = 225;
  M5.Lcd.drawString("3Clk: Right Click", 10, yStart);
}

class MyCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    BLE2902* desc2;
    desc2 = (BLE2902*) input2->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc2->setNotifications(true);
    BLE2902* desc1;
    desc1 = (BLE2902*) input1->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc1->setNotifications(true);

    isConnected = true;
    DisplayConnectionText();
    DisplayStatusText("Connecting process completed.");
  }

  void onDisconnect(BLEServer* pServer){
    isConnected = false;
    DisplayConnectionText();
  }
};

void StartBLEServer() {
  BLEServer *pServer;
  pServer = BLEDevice::createServer();
  DisplayStatusText("Creating server...");
  DisplayConnectionText();
  
  //Instantiate hid device
  hid = new BLEHIDDevice(pServer);

  input2 = hid->inputReport(2); // <-- input REPORTID from report map
  output2 = hid->outputReport(2); // <-- output REPORTID from report map
  input1 = hid->inputReport(1); // <-- input REPORTID from report map
  output1 = hid->outputReport(1); // <-- output REPORTID from report map

  //set server callbacks
  pServer->setCallbacks(new MyCallbacks());

  /*
   * Set manufacturer name (OPTIONAL)
   * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.manufacturer_name_string.xml
   */
  std::string name = "esp-community"; //yes!
  hid->manufacturer()->setValue(name);

  /*
   * Set pnp parameters (MANDATORY)
   * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFilef2
   */
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);

  /*
   * Set hid informations (MANDATORY)
   * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
   */
  hid->hidInfo(0x00,0x01);

  /*
   * Mouse
   */
  const uint8_t reportMap[] = {

    // Mouse descriptor
    0x05, 0x01,                     // Usage Page (Generic Desktop)
    0x09, 0x02,                     // Usage (Mouse)
    0xA1, 0x01,                     // Collection (Application)
    
    // Report ID 1: Mouse motion
    0x85, 0x01,                     //      Report ID (MOUSE_XY_IN_REP_ID)
    0x09, 0x01,                     //      Usage (Pointer)
    0xA1, 0x00,                     //      Collection (Physical)
    0x75, 0x0C,                     //          Report Size (12)
    0x95, 0x02,                     //          Report Count (2)
    0x05, 0x01,                     //          Usage Page (Generic Desktop)
    0x09, 0x30,                     //          Usage (X)
    0x09, 0x31,                     //          Usage (Y)
    0x16, 0x01, 0xF8,               //          Logical maximum (2047)
    0x26, 0xFF, 0x07,               //          Logical minimum (-2047)
    0x81, 0x06,                     //          Input (Data, Variable, Relative)
    0xC0,                           //      End Collection (Physical)

    // Report ID 2: Mouse buttons + scroll/pan
    0x85, 0x02,                     //      Report ID (MOUSE_BTN_IN_REP_ID)
    0x09, 0x01,                     //      Usage (Pointer)
    0xA1, 0x00,                     //      Collection (Physical)
    0x95, 0x05,                     //          Report Count (5)
    0x75, 0x01,                     //          Report Size (1)
    0x05, 0x09,                     //          Usage Page (Buttons)
    0x19, 0x01,                     //          Usage Minimum (01)
    0x29, 0x05,                     //          Usage Maximum (05)
    0x15, 0x00,                     //          Logical Minimum (0)
    0x25, 0x01,                     //          Logical Maximum (1)
    0x81, 0x02,                     //          Input (Data, Variable, Absolute)
    0x95, 0x01,                     //          Report Count (1)
    0x75, 0x03,                     //          Report Size (3)
    0x81, 0x01,                     //          Input (Constant) for padding
    0x75, 0x08,                     //          Report Size (8)
    0x95, 0x01,                     //          Report Count (1)
    0x05, 0x01,                     //          Usage Page (Generic Desktop)
    0x09, 0x38,                     //          Usage (Wheel)
    0x15, 0x81,                     //          Logical Minimum (-127)
    0x25, 0x7F,                     //          Logical Maximum (127)
    0x81, 0x06,                     //          Input (Data, Variable, Relative)
    0xC0,                           //      End Collection (Physical)
    
    0xC0,                           // End Collection (Application)
  };
  
  /*
   * Set report map (here is initialized device driver on client side) (MANDATORY)
   * https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.report_map.xml
   */
  hid->reportMap((uint8_t*)reportMap, sizeof(reportMap));

  /*
   * We are prepared to start hid device services. Before this point we can change all values and/or set parameters we need.
   * We can setup characteristics authorization
   */
  hid->startServices();

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  delay(1500);

  /*
   * setup advertising by providing appearance and advertised service.
   */
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_MOUSE);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->start();
  DisplayStatusText("Advertising started.");
}

void setup() {
  M5.begin();                   // M5STACK INITIALIZE
  M5.Lcd.setBrightness(120);    // BRIGHTNESS = MAX 255
  M5.Lcd.fillScreen(BLACK);     // CLEAR SCREEN
  M5.Lcd.setTextSize(1);
  DisplayStatusText("Initializing...");
  DisplayConnectionText();
  DisplayGuide();

  // your setup code, to run once:
  BLEDevice::init("M5Stack-Mouse");
  StartBLEServer();
}

void MouseClick(unsigned char button) {
  uint8_t a[] = {button,0x0};
  input2->setValue(a, sizeof(a));
  input2->notify();
  uint8_t b[] = {0x0,0x0};
  input2->setValue(b, sizeof(b));
  input2->notify();   
}

void MouseMove(short xRaw, short yRaw){
  //convert 12bit two's complement
  unsigned int xDiff = (xRaw ^ 0xfff + 1) & 0xfff;
  unsigned int yDiff = (yRaw ^ 0xfff + 1) & 0xfff;

  //set report value 
  uint8_t a[] = {0x0,0x0,0x0};        
  a[0] = (unsigned char)(xDiff & 0x0ff);
  a[1] = (unsigned char)((yDiff & 0x00f) * 16 + (xDiff >> 8));
  a[2] = (unsigned char)(yDiff >> 4);

  input1->setValue(a,sizeof(a));
  input1->notify();
}

void loop() {
  unsigned long ts = millis();
  if(isConnected && M5.BtnA.wasPressed()) {
    //remember first button push time
    if(sequentialCount == 0)
    {
      tsStart = ts;
    }
    sequentialCount++;
  }

  //time after first button push
  if(ts - tsStart > 600 && sequentialCount > 0)
  {
    if(sequentialCount == 1){
      movingMode*= -1;
      DisplayGuide();
    }
    if(sequentialCount == 2){
      MouseClick(0x1);
    }
    if(sequentialCount == 3){
      MouseClick(0x2);
    }

    M5.Lcd.drawString("Click status, ts:" + String(tsStart) + ", count:" + String(sequentialCount), 10, 100);
    sequentialCount = 0;
  }

  if(isConnected && M5.BtnB.wasPressed()) {
    const short xRaw = movingMode == 1 ? 0 : -20;
    const short yRaw = movingMode == 1 ? -20 : 0;
    MouseMove(xRaw, yRaw);
  }
  
  if (isConnected && M5.BtnC.wasPressed()) {
    const short xRaw = movingMode == 1 ? 0 : 20;
    const short yRaw = movingMode == 1 ? 20 : 0;
    MouseMove(xRaw, yRaw);
  }

  //update button information
  M5.update();
  delay(30);
}

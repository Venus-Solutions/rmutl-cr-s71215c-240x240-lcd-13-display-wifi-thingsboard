#include <TonyS_X1.h>
#include <TonyS_X1_ExternalModule.h>
#include <WiFi.h>
#include <ThingsBoard.h>

#define WIFI_SSID             "Your SSID"
#define WIFI_PASSWORD         "Your Password"
#define TOKEN                 "Your Token"
#define THINGSBOARD_SERVER    "vsmqtt.space"

//==================================================Thingsboard==================================================//

uint16_t THINGSBOARD_PORT PROGMEM = 8080U;

// Maximum size packets will ever be sent or received by the underlying MQTT client,
// if the size is to small messages might not be sent or received messages will be discarded
constexpr uint32_t MAX_MESSAGE_SIZE PROGMEM = 128U;
WiFiClient espClient;                                       // Initialize underlying client, used to establish a connection
ThingsBoardSized<MAX_MESSAGE_SIZE> tb(espClient);           // Initialize ThingsBoard instance with the maximum needed buffer size

#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))    // Helper macro to calculate array size

//==================================================Optocoupler==================================================//

#define OPT0 IO0                                            //  Config IO for optocoupler modules
#define OPT1 IO2
#define OPT2 IO4
#define OPT3 IO6
#define OPT4 IO8
#define OPT5 IO10
#define OPT6 IO3
#define OPT7 IO5

bool q00 = 0;                                               //  Init q00 - q07 for reading PLC outputs Q0.0 - Q0.7
bool q01 = 0;
bool q02 = 0;
bool q03 = 0;
bool q04 = 0;
bool q05 = 0;
bool q06 = 0;
bool q07 = 0;

//==================================================LCD==================================================//

Adafruit_ST7789 tft = Adafruit_ST7789(SLOT1);             //  Init 1.3inch TFT display at SLOT#1 2nd Floor

//==================================================Wi-Fi==================================================//

void initilizeWiFi(void) {                                        // Connecting to Wi-Fi Access Point
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  tft.println("Connecting to ");
  tft.print(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      tft.print(".");
  }
  Serial.println("Wi-Fi conected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  tft.println("");
  tft.println("WiFi conected");
  tft.println("");
}

const bool reconnect(void) {                                      // Reconnecting Wi-Fi Access Point
  const wl_status_t status = WiFi.status();                       // Check to ensure we aren't connected yet
  if (status == WL_CONNECTED) {
      return true;
  }
  initilizeWiFi();                                                // If we aren't establish a new connection to the given WiFi network
  return true;
}

//==================================================Relay==================================================//

RPC_Response setRelay1(const RPC_Data &data) {                    // setRelay1 callback function
  Serial.println("Received the set GPIO RPC method");
  
  int state = data;
  Serial.print("Setting I1.0 to state ");
  Serial.println(state);
  Tony.digitalWrite(Relay_1, state ? HIGH : LOW);
  
  return RPC_Response(NULL, data);
}

RPC_Response setRelay2(const RPC_Data &data) {                    // setRelay2 callback function
  Serial.println("Received the set GPIO RPC method");
  
  int state = data;
  Serial.print("Setting I1.1 to state ");
  Serial.println(state);
  Tony.digitalWrite(Relay_2, state ? HIGH : LOW);
  
  return RPC_Response(NULL, data);
}

RPC_Callback callbacks[] = {                                //  RPC handlers
  { "setValueRelay1",   setRelay1 },
  { "setValueRelay2",   setRelay2 },
};

bool subscribed = false;                                    // Set to true if application is subscribed for the RPC messages.

//==================================================Millis==================================================//

unsigned long lastMeasure = 0;                              //  Init millis() variable
unsigned long interval = 1000;
unsigned long now = 0;

//==================================================Setup & Loop==================================================//

void setup() 
{
  Serial.begin(115200);
  Tony.begin();  
  delay(10);
  
  Tony.pinMode(OPT0, INPUT);
  Tony.pinMode(OPT1, INPUT);
  Tony.pinMode(OPT2, INPUT);
  Tony.pinMode(OPT3, INPUT);
  Tony.pinMode(OPT4, INPUT);
  Tony.pinMode(OPT5, INPUT);
  Tony.pinMode(OPT6, INPUT);
  Tony.pinMode(OPT7, INPUT);
 
  Tony.pinMode(Relay_1, OUTPUT);
  Tony.pinMode(Relay_2, OUTPUT);

  tft.init(240, 240);                                       //  Init TFT display format
  tft.setRotation(0);
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_CYAN);
}

void loop() 
{
  if (!reconnect()) {
      return;
  }

  if (!tb.connected()) {
      subscribed = false;

      Serial.print("Connecting to: ");                      // Connect to the Thingsboard server
      Serial.print(THINGSBOARD_SERVER);
      Serial.print(" with token ");
      Serial.println(TOKEN);
      tft.println("Connecting to");
      tft.println(THINGSBOARD_SERVER);
      tft.println("with token ");
      tft.println(TOKEN);
      if (!tb.connect(THINGSBOARD_SERVER, TOKEN)) {
          Serial.println("Failed to connect");
          tft.println();
          tft.println("Failed to connect");
          return;
      }
      delay(1000);
  }

  if (!subscribed) {
      Serial.println("Subscribing for RPC... ");

      for (size_t i = 0; i < COUNT_OF(callbacks); i++) {            // Subscribe for RPC callback functions
          if (!tb.RPC_Subscribe(callbacks[i])) {
              Serial.println("Failed to subscribe for RPC");
              return;
          }
      }

      Serial.println("Subscribe done");
      subscribed = true;
  }

  now = millis();                                                   
  if (now - lastMeasure > interval){                                // Waiting for millis() event trigger
      lastMeasure = now;

      q00 = !Tony.digitalRead(OPT0);                                // Reading optocoupler inputs
      q01 = !Tony.digitalRead(OPT1);
      q02 = !Tony.digitalRead(OPT2);
      q03 = !Tony.digitalRead(OPT3);
      q04 = !Tony.digitalRead(OPT4);
      q05 = !Tony.digitalRead(OPT5);
      q06 = !Tony.digitalRead(OPT6);
      q07 = !Tony.digitalRead(OPT7);

      tb.sendAttributeBool("Q00", q00);                             // Sending boolean attribute to Thingsboard
      tb.sendAttributeBool("Q01", q01);
      tb.sendAttributeBool("Q02", q02);
      tb.sendAttributeBool("Q03", q03);
      tb.sendAttributeBool("Q04", q04);
      tb.sendAttributeBool("Q05", q05);
      tb.sendAttributeBool("Q06", q06);
      tb.sendAttributeBool("Q07", q07);

      Serial.print("Q : ");                                         // Printing PLC outputs status Q0.0 - Q0.7
      Serial.print(q00);
      Serial.print(q01);
      Serial.print(q02);
      Serial.print(q03);
      Serial.print(q04);
      Serial.print(q05);
      Serial.print(q06);
      Serial.println(q07);

      tft.fillScreen(ST77XX_BLACK);                                 // Printing PLC outputs status Q0.0 - Q0.7 to TFT display
      tft.setCursor(0, 0);
      tft.setTextSize(2);
      tft.println();
      tft.setTextSize(4);
      tft.println("TONY SPACE");
      tft.setTextSize(3);
      tft.println("_____________");
      tft.println();
      tft.println();
      tft.println("MONITORED I/O");
      tft.setTextSize(1);
      tft.println();
      tft.setTextSize(3);
      tft.print("Q = ");
      tft.print(q00);
      tft.print(q01);
      tft.print(q02);
      tft.print(q03);
      tft.print(q04);
      tft.print(q05);
      tft.print(q06);
      tft.println(q07);
      tft.println();
      tft.println("_____________");
  }
  
  tb.loop();
}

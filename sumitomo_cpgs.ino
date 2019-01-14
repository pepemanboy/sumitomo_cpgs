#include "ciropkt.h"
#include "ciropkt_cmd.h"
#include <SoftwareSerial.h>

// COMMUNICATION
#define MASTER_ADDRESS 0

// SERIAL
#define SERIAL_TIMEOUT_MS 100

// HC12
#define HC12_CHANNEL 1
#define HC12_BAUDRATE 9600
#define HC12 HC12_Serial

// USB
#define USB Serial
#define USB_BAUDRATE 9600

// CONFIGURABLE DEFINES
// #define MASTER
// #define DEBUG

// MASTER DEFINES
#ifdef MASTER

#define LED_MASTER 8
#define LED_ERROR LED_MASTER

#define HC12_RX 2
#define HC12_TX 3
#define HC12_SET 7

#define RXBLINK_MS 200

#else // !MASTER

// SLAVE DEFINES
#define CPG_LED 2
#define CPG_BUZZER 3
#define LED_GREEN 7
#define LED_YELLOW 8
#define LED_RED 9
#define LED_ERROR LED_RED

#define HC12_TX 6
#define HC12_RX 5
#define HC12_SET 4

#define SW1 10
#define SW2 11
#define SW3 12
#define SW4 13
#define SW5 A0
#define SW6 A1
#define SW7 A2
#define SW8 A3
#define SW_NUMBER 5

const uint8_t SWITCHES[] = {SW1, SW2, SW3, SW5, SW6};

#define CYCLEGAP_MS 1000
#define SENDBLINK_MS 200

#define INIT_DELAY_MS 2000

#endif // !MASTER

// COMMON DEFINES

// COMMUNICATIONS
// RX
char rxBuf_[255]; // Min size pkt_MAXSPACE + 1
pkt_VAR(rxPacket_);
CPGInfo * rxCPGInfo_ = (CPGInfo *) &rxPacket_.data;
size_t rxCnt_ = 0;
unsigned long serialTimestamp_ = 0;

// TX
#ifndef MASTER

char txBuf_[pkt_MAXSPACE + 1];
size_t txBufLen_ = 0;
pkt_VAR(txPacket_);

#endif // !MASTER

// SENSORS
unsigned long cycleTimestamp_ = 0;
unsigned long ledTimestamp_ = 0;

// SOFTWARE SERIAL
SoftwareSerial HC12_Serial(HC12_RX, HC12_TX);

//// SHARED FUNCTIONS

/** Configure HC 12 parameter */
res_t HC12_configure_parameter(char * query)
{
  res_t r;
  HC12.println(query);
  size_t l = HC12.readBytes(rxBuf_, sizeof(rxBuf_));
  rxBuf_[l] = '\0';
  debug(rxBuf_);
  if (!strstr(rxBuf_, "OK")) 
    return EFormat;
  return Ok;
}

/** Setup HC12 module */
res_t HC12_setup()
{
  res_t r;
  pinMode(HC12_SET, OUTPUT);

  HC12.setTimeout(SERIAL_TIMEOUT_MS);
  HC12.begin(9600); // Default baudrate per datasheet
  
  debug((char *)"Setting set pin LOW");
  digitalWrite(HC12_SET, LOW); // Enter setup mode
  delay(250);

  char buf[20];
  // Test
  debug((char *)"printing AT");
  sprintf(buf, "AT");
  r = HC12_configure_parameter(buf);
  if (r != Ok)
    return r;

  // Set channel
  debug((char *)"Configuring channel");
  sprintf(buf, "AT+C%03d", HC12_CHANNEL); 
  r = HC12_configure_parameter(buf);
  if (r != Ok)
    return r;
    
  // Set baud rate
  debug((char *)"Configuring baudrate");
  sprintf(buf, "AT+B%d", HC12_BAUDRATE);
  r = HC12_configure_parameter(buf);
  if (r != Ok)
    return r;
    
  HC12.begin(HC12_BAUDRATE);  

  debug((char *)"Setting pin HIGH");
  digitalWrite(HC12_SET, HIGH); // Exit setup mode
  delay(250);
  
  return Ok;
}

/** Debug to PC Serial */
void debug(char * str)
{
  #ifdef DEBUG
  USB.println(str);
  #endif // DEBUG
}

typedef enum
{
  led_On = 0,
  led_Off,
}led_e;

/** Contro leds */
void ledControl(uint8_t led, uint8_t state)
{
  digitalWrite(led, state == led_On ? LOW : HIGH);
}

/** Blink led */
void ledBlink(uint8_t led, int time_ms)
{  
  ledControl(led, led_On);
  delay(time_ms);
  ledControl(led, led_Off);
}
/** Setup led module */
#ifdef MASTER
res_t ledSetup()
{
  pinMode(LED_MASTER, OUTPUT);
  ledControl(LED_MASTER, led_On);
  return Ok;
}
#else
res_t ledSetup(){
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  ledControl(LED_GREEN, led_Off);
  ledControl(LED_YELLOW, led_Off);
  ledControl(LED_RED, led_Off);
  return Ok;
}
#endif // !MASTER

/** Error function */
void error()
{
  while(1)
  {
    digitalWrite(LED_ERROR, HIGH);
    delay(500);
    digitalWrite(LED_ERROR, LOW);
    delay(500);
  }
}

//// MASTER FUNCTIONS
#ifdef MASTER

/** Master read loop */
void readLoop()
{
  // Turn off blink led
  if ((millis() - ledTimestamp_) > RXBLINK_MS)
    ledControl(LED_MASTER, led_Off);
    
  if (HC12.available()) 
  {
    serialTimestamp_ = millis();
    char inByte = HC12.read();
    if(inByte == 0)
    {
      packetRx();     
      rxCnt_ = 0;         
    }
    else
      rxBuf_[rxCnt_++] = inByte;
  }

  // Serial timeout
  if ((rxCnt_ != 0) && ((millis() - serialTimestamp_) > SERIAL_TIMEOUT_MS))
    rxCnt_ = 0;

  // Serial overflow
  if (rxCnt_ >= sizeof(rxBuf_))
    rxCnt_ = 0;
}

/** Toggle master led */
void ledToggleMaster()
{
  static bool b = true;
  b = !b;
  ledControl(LED_MASTER, b ? led_On : led_Off);
}

/** Process received packet in master */
res_t packetRx()
{
  packet_t * p = &rxPacket_;
  res_t r = pktDeserialize(p, (uint8_t *)rxBuf_, rxCnt_);
  if (!r) {
    return EParse; 
  }
  else
  {       
    if (pktCheck(p))
    {
      if (p->address != MASTER_ADDRESS) {
        return EAddress;
      }
      if (p->command != cmd_CPGInfo) {
        return ECommand;
      }
      USB.println(rxCPGInfo_->cpg_id);
      ledControl(LED_MASTER, led_On);
      ledTimestamp_ = millis();
    }
    else {
      return EFormat;
    }
  }   
}

#endif // MASTER

//// SLAVE FUNCTIONS
#ifndef MASTER

/** Setup switches */
res_t switchesSetup()
{
  for(uint8_t i = 0; i < SW_NUMBER; ++i)
  {
    pinMode(SWITCHES[i], INPUT);
  }
  return Ok;
}

/** Setup CPG inputs */
void cpgInputsSetup()
{
  pinMode(CPG_LED, INPUT);
  pinMode(CPG_BUZZER, INPUT);
}


/** Read switches in binary format */
uint8_t readSwitches()
{
  uint8_t val = 0;
  uint8_t reading = 0;
  for (uint8_t i = 0; i < SW_NUMBER; ++i)
  {
    reading = digitalRead(SWITCHES[i]) ? 0 : 1;
    val |= reading << i;
  }
  return val;
}

/** Read cpg led */
bool readCPGLed()
{
  return !digitalRead(CPG_LED);
}

/** Read cpg buzzer */
bool readCPGBuzzer()
{
  return !digitalRead(CPG_BUZZER);
}

/** Return cycle complete */
bool cycleComplete()
{
  return readCPGLed() && readCPGBuzzer();
}

/** Setup cpg packet */
void setupCPGpacket(packet_t *p)
{  
  p->address = MASTER_ADDRESS;
  p->command = cmd_CPGInfo;
  
  CPGInfo c = {.cpg_id = readSwitches()};
  pktUpdate(p, &c, sizeof(CPGInfo));
  
  pktRefresh(p);
}

/** Move packet to buffer */
void bufferPacket(packet_t *p)
{
  size_t len = sizeof(txBuf_);
  pktSerialize(p, (uint8_t *) txBuf_, &len);
  txBufLen_ = len;
}

/** Send buffered packet */
void sendPacket(packet_t *p)
{
  bufferPacket(p);
  sendBuffer();
}

/** Send txbuf */
void sendBuffer()
{
  HC12.write(txBuf_, txBufLen_);
  HC12.write((uint8_t)0);  
}

#endif // !MASTER




//// SETUP
void setup() 
{
  #ifndef MASTER  
  #ifdef DEBUG  
  USB.begin(USB_BAUDRATE);
  #endif // DEBUG
  #endif // !MASTER

  #ifdef MASTER
  USB.begin(USB_BAUDRATE);
  #endif // MASTER
  
  uint8_t res = Ok;
  res = ledSetup();
  if (res != Ok)
    error();

  res = HC12_setup();
  if (res != Ok)
    error();
    
  #ifndef MASTER  
  ledControl(LED_GREEN, led_On);
  switchesSetup();
  cpgInputsSetup();
  setupCPGpacket(&txPacket_);
  bufferPacket(&txPacket_);
  delay(INIT_DELAY_MS);
  #endif // !MASTER

  #ifdef MASTER
  ledSetup();
  HC12_setup();
  #endif // MASTER
}

//// LOOP
void loop() {
  #ifndef MASTER
  
  // Turn off blink led
  if ((millis() - ledTimestamp_) > SENDBLINK_MS)
    ledControl(LED_YELLOW, led_Off);
  
  if (cycleComplete())
  {
    if ((millis() - cycleTimestamp_) > CYCLEGAP_MS)
    {
      sendBuffer();
      // Turn on blink led
      ledControl(LED_YELLOW, led_On);
      ledTimestamp_ = millis();
    }
    cycleTimestamp_ = millis();
  }
  #endif // !MASTER

  #ifdef MASTER
  readLoop();
  #endif  // MASTER
}

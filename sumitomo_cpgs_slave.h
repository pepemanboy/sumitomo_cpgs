#include "sumitomo_cpgs_common.h"

class CPG_Slave:CPG
{
private:
  const uint8_t cpg_led_ = 2;
  const uint8_t cpg_buzzer_ = 3;
  const uint8_t led_green_ = 7;
  const uint8_t led_yellow_ = 8;
  const uint8_t led_red_ = 9;

  const uint8_t switch_1_ = 10;
  const uint8_t switch_2_ = 11;
  const uint8_t switch_3_ = 12;
  const uint8_t switch_4_ = 13;
  const uint8_t switch_5_ = A0;
  const uint8_t switch_6_ = A1;
  const uint8_t switch_7_ = A2;
  const uint8_t switch_8_ = A3;

  const uint8_t switches_[5] = {switch_1_, switch_2_, switch_3_, switch_5_, switch_6_};

  const uint16_t pulse_gap_min_ms_ = 300;
  const uint16_t send_blink_ms_ = 200;
  const uint16_t init_delay_ms = 2000;

  /// VARIABLES
  uint32_t pulse_timestamp_ = 0;
  uint32_t led_timestamp_ = 0;
  uint8_t pulse_count_ = 0;
  uint8_t sequence_ = 0;

public:
  /** Constructor */
  CPG_Slave():
  CPG(6,5,4)
  {
    led_error_ = led_red_;
  }

protected:
  /** Setup led module */
  void ledSetup(){
    pinMode(led_green_, OUTPUT);
    pinMode(led_yellow_, OUTPUT);
    pinMode(led_red_, OUTPUT);
  
    ledControl(led_green_, led_Off);
    ledControl(led_yellow_, led_Off);
    ledControl(led_red_, led_Off);
  }

  /** Setup switches */
  void switchesSetup()
  {
    for(uint8_t i = 0; sizeof(switches_); ++i)
      pinMode(switches_[i], INPUT);
  }

  /** Setup CPG inputs */
  void cpgInputsSetup()
  {
    pinMode(cpg_led_, INPUT);
    pinMode(cpg_buzzer_, INPUT);
  }

  
  /** Read switches in binary format */
  uint8_t readSwitches()
  {
    uint8_t val = 0;
    uint8_t reading = 0;
    for (uint8_t i = 0; i < sizeof(switches_); ++i)
    {
      reading = digitalRead(switches_[i]) ? 0 : 1;
      val |= reading << i;
    }
    return val;
  }

  /** Read cpg led */
  bool readCPGLed()
  {
    return !digitalRead(cpg_led_);
  }

  /** Read cpg buzzer */
  bool readCPGBuzzer()
  {
    return !digitalRead(cpg_buzzer_);
  }

  /** Return detection of pulse */
  bool pulseDetect()
  {
    return readCPGLed() && readCPGBuzzer();
  }

  /** Send txbuf */
  void sendBuffer()
  {
    HC12.write(tx_buffer_, tx_buffer_length_);
    HC12.write((uint8_t)0);  
  }

  /** Setup cpg packet */
  void setupCPGpacket(packet_t *p)
  {  
    p->address = master_address_;
    p->command = cmd_CPGInfoReply;
    
    CPGInfoReply c = {.cpg_id = readSwitches(), .cpg_count = pulse_count_, .cpg_sequence = sequence_};
    pktUpdate(p, &c, sizeof(CPGInfoReply));
    
    pktRefresh(p);
  }

  /** Move packet to buffer */
  void bufferPacket(packet_t *p)
  {
    size_t len = sizeof(tx_buffer_);
    pktSerialize(p, (uint8_t *) tx_buffer_, &len);
    tx_buffer_length_ = len;
  }

  /** Send buffered packet */
  void sendPacket(packet_t *p)
  {
    bufferPacket(p);
    sendBuffer();
  }

public:

  void setup()
  {
    #ifdef DEBUG  
    USB.begin(USB_BAUDRATE);
    #endif // DEBUG
    
    ledSetup();
  
    res_t res = HC12_setup();
    if (res != Ok)
      error();
      
    ledControl(led_green_, led_On);
    switchesSetup();
    cpgInputsSetup();
    setupCPGpacket(&tx_packet_);
    bufferPacket(&tx_packet_);
    delay(init_delay_ms);
  }
  
  
  void loop() {  
    // Turn off blink led
    if ((millis() - led_timestamp_) > send_blink_ms_)
      ledControl(led_yellow_, led_Off);
    
    if (pulseDetect())
    {
      if ((millis() - pulse_timestamp_) > pulse_gap_min_ms_)
      {
        sendBuffer();
        ledControl(led_yellow_, led_On);
        led_timestamp_ = millis();    
      }
      pulse_timestamp_ = millis();
    }
  }  
};




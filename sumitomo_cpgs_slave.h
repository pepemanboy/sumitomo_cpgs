#include "sumitomo_cpgs_common.h"

class CPG_Slave:CPG
{
  
private:
  /// CONFIGURATION
  const pin_t cpg_led_ = 2; ///< CPG Led pin
  const pin_t cpg_buzzer_ = 3; ///< CPG Buzzer pin
  
  const pin_t led_green_ = 7; ///< Green LED pin
  const pin_t led_yellow_ = 8; ///< Yellow LED pin
  const pin_t led_red_ = 9; ///< Red LED pin

  const pin_t switch_1_ = 10; ///< Switch 1 pin
  const pin_t switch_2_ = 11; ///< Switch 2 pin
  const pin_t switch_3_ = 12; ///< Switch 3 pin
  const pin_t switch_4_ = 13; ///< Switch 4 pin
  const pin_t switch_5_ = A0; ///< Switch 5 pin
  const pin_t switch_6_ = A1; ///< Switch 6 pin
  const pin_t switch_7_ = A2; ///< Switch 7 pin
  const pin_t switch_8_ = A3; ///< Switch 8 pin

  const pin_t switches_[5] = {switch_1_, switch_2_, switch_3_, switch_5_, switch_6_}; ///< Usable switches for selecting address

  const uint16_t pulse_gap_min_ms_ = 300; ///< Minimum gap between pulses [ms]
  const uint16_t send_blink_ms_ = 200; ///< Send blink LED duration [ms]
  const uint16_t init_delay_ms = 2000; ///< Initialization delay [ms]

private:
  /// VARIABLES
  uint32_t pulse_timestamp_ = 0; ///< Timestamp of last pulse
  uint32_t led_timestamp_ = 0; ///< Timestamp of last time LED turned on
  uint8_t pulse_count_ = 0; ///< Pulse count
  uint8_t pulse_backup_ = 0; ///< Pulse count backup
  uint8_t sequence_ = 0; ///< Communication sequence
  
  const uint8_t hc12_tx_ = 6; ///< HC12 Tx pin
  const uint8_t hc12_rx_ = 5; ///< HC12 Rx pin
  const uint8_t hc12_set_ = 4; ///< HC12 Set pin

public:
  /** Constructor */
  CPG_Slave():
  CPG(hc12_tx_, hc12_rx_, hc12_set_, led_red_)
  {}

/// PACKET QUERIES
private:
  /** Query CPG Info */
  void replyCPGInfo(uint8_t address, const CPGInfoReply * c)
  {
    queryPacket(address, cmd_CPGInfoReply, (uint8_t *)c, sizeof(c)); 
  }

private:
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

  /** Sample pulses */
  void samplePulse()
  {
    if (pulseDetect())
    {
      if ((millis() - pulse_timestamp_) > pulse_gap_min_ms_)
      {
        pulse_count_++;
        ledControl(led_yellow_, led_On);
        led_timestamp_ = millis();    
      }
      pulse_timestamp_ = millis();
    }
  }

public:
  /** Setup */
  void setup()
  {
    #ifdef DEBUG  
    USB.begin(usb_baudrate_);
    #endif // DEBUG
    
    ledSetup();
  
    res_t res = HC12_setup(home_channel_);
    if (res != Ok)
      error();      
    ledControl(led_green_, led_On);
    
    switchesSetup();    
    setAddress(readSwitches());
    
    cpgInputsSetup();
    
    delay(init_delay_ms);
  }
  
  /** Loop */
  void loop() {  

    res_t r = Ok;
    
    // Turn off blink led
    if ((millis() - led_timestamp_) > send_blink_ms_)
      ledControl(led_yellow_, led_Off);

    // Pulse detection
    samplePulse();   

    // Receive data
    if (HC12.available())
    {
      // Wait for reply
      size_t l = HC12.readBytesUntil(0, rx_buffer_, sizeof(rx_buffer_));

      // Process reply
      if (l)
      {        
        packet_t *p = &rx_packet_;
        r = packetRx(p, rx_buffer_, l-1);
        if (r == Ok) 
        {     
          // CPG Info Query           
          if (p->command == cmd_CPGInfoQuery && p->data_size == sizeof(CPGInfoQuery)) 
          {
            CPGInfoQuery *qry = (CPGInfoQuery*)&p->data;
            if (qry->cpg_sequence == sequence_)
            {
              pulse_backup_ = 0;
              ++sequence_;              
            }
            pulse_backup_ += pulse_count_;
            // Transmit info
            CPGInfoReply c = {.cpg_id = address(), .cpg_count = pulse_backup_, .cpg_sequence = sequence_};
            replyCPGInfo(master_address_, &c);            
          }
          // CPG Init query
          else if (p->command == cmd_CPGInitQuery && p->data_size == sizeof(CPGInitQuery))
          {
            CPGInitQuery *qry = (CPGInitQuery*)&p->data;
            if (qry->cpg_address & (1<<address()))
              HC12_setup(qry->cpg_channel);
          }
          else // Command mismatch
            r = ECommand;            
        }
      }
    }
  }  
};




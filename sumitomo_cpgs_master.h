#include "sumitomo_cpgs_common.h"

class CPG_Master:CPG
{
protected:

  /// CONFIGURATION
  const uint8_t led_master_ = 8;
  const uint16_t rx_blink_ms = 200;

  /// VARIABLES
  uint32_t led_timestamp_ = 0;
  uint32_t serial_timestamp_ = 0;

public:
  /** Constructor */
  CPG_Master():
  CPG(3,2,7)
  {
    led_error_ = led_master_;
  }


  protected:

  /** Setup led module */
  res_t ledSetup()
  {
    pinMode(led_master_, OUTPUT);
    ledControl(led_master_, led_On);
    return Ok;
  }

  /** Process received packet in master */
  res_t packetRx()
  {
    packet_t * p = &rx_packet_;
    res_t r = pktDeserialize(p, (uint8_t *)&rx_packet_, rx_count_);
    if (!r) {
      return EParse; 
    }
    else
    {       
      if (pktCheck(p))
      {
        if (p->address != master_address_) {
          return EAddress;
        }
        if (p->command != cmd_CPGInfoQuery) {
          return ECommand;
        }
        // print id
        ledControl(led_master_, led_On);
        led_timestamp_ = millis();
      }
      else {
        return EFormat;
      }
    }   
  }

public:
  
  void setup()
  {
    USB.begin(usb_baudrate_);
    
    uint8_t res = Ok;
    res = ledSetup();
    if (res != Ok)
      error();
  
    res = HC12_setup();
    if (res != Ok)
      error();
      
    // Mandar CPG Init packet
  }
  
  void loop() 
  {
    /*
     * for every slave
     *  send cpginfo query
     *  recieve cpginfo reply
     *    save sequence
     *    print id count times
     *  if timeout
     *    add 1 to slave[i].timeout_count
     *    
     * if any slave timeout > max_timeout_count
     *  switch to Channel 0
     *  send cpginit query to all slaves with timeouts != 0
     *  swtich to Master Channel
     *    
     *    
     * 
     * 
     */  
    // Turn off blink led
    if ((millis() - led_timestamp_) > rx_blink_ms)
      ledControl(led_master_, led_Off);
      
    if (HC12.available()) 
    {
      serial_timestamp_ = millis();
      char inByte = HC12.read();
      if(inByte == 0)
      {
        packetRx();     
        rx_count_ = 0;         
      }
      else
        rx_buffer_[rx_count_++] = inByte;
    }
  
    // Serial timeout
    if ((rx_count_ != 0) && ((millis() - serial_timestamp_) > serial_timeout_ms_))
      rx_count_ = 0;
  
    // Serial overflow
    if (rx_count_ >= sizeof(rx_buffer_))
      rx_count_ = 0;
  }
};



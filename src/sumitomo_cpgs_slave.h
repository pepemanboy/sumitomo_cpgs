/** @file
  CPG Slave implementation

  Defines the CPG_Slave class, wich inherits from CPG. 

  @date 2019-01-31
  @author pepemanboy

  Copyright 2019 Cirotec Automation

  Permission is hereby granted, free of charge, to any person obtaining a copy 
  of this software and associated documentation files (the "Software"), to deal 
  in the Software without restriction, including without limitation the rights 
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
  copies of the Software, and to permit persons to whom the Software is 
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in 
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
  SOFTWARE.
*/

#include "sumitomo_cpgs_common.h"

#ifndef SUMITOMO_CPGS_SLAVE_H
#define SUMITOMO_CPGS_SLAVE_H

class CPG_Slave:CPG
{
  
private:
  /// CONFIGURATION
  const static pin_t cpg_led_ = 2; ///< CPG Led pin
  const static pin_t cpg_buzzer_ = 3; ///< CPG Buzzer pin
  
  const static pin_t led_green_ = 7; ///< Green LED pin
  const static pin_t led_yellow_ = 8; ///< Yellow LED pin
  const static pin_t led_red_ = 9; ///< Red LED pin

  const static pin_t switch_1_ = 10; ///< Switch 1 pin
  const static pin_t switch_2_ = 11; ///< Switch 2 pin
  const static pin_t switch_3_ = 12; ///< Switch 3 pin
  const static pin_t switch_4_ = 13; ///< Switch 4 pin
  const static pin_t switch_5_ = A0; ///< Switch 5 pin
  const static pin_t switch_6_ = A1; ///< Switch 6 pin
  const static pin_t switch_7_ = A2; ///< Switch 7 pin
  const static pin_t switch_8_ = A3; ///< Switch 8 pin

  const pin_t switches_[5] = {
    switch_1_, 
    switch_2_, 
    switch_3_, 
    switch_5_, 
    switch_6_
  }; ///< Usable switches for selecting address 

  const static uint16_t pulse_gap_min_ms_ = 50; ///< Minimum gap between pulses [ms]
  const static uint16_t tx_blink_ms_ = 200; ///< Send blink LED duration [ms]
  const static uint16_t init_delay_ms = 2000; ///< Initialization delay [ms]
  const static uint16_t pulse_blink_ms_ = 200; ///< Pulse blink LED duration [ms]
  const static uint16_t serial_timeout_ms_ = 100; ///< Serial timeout [ms]

private:
  /// VARIABLES
  uint32_t pulse_timestamp_ = 0; ///< Timestamp of last pulse
  uint32_t led_yellow_timestamp_ = 0; ///< Timestamp of yellow LED
  uint32_t led_red_timestamp_ = 0; ///< Timestamp of red LED
  uint8_t pulse_count_ = 0; ///< Pulse count
  uint8_t pulse_backup_ = 0; ///< Pulse count backup
  uint8_t sequence_ = 0; ///< Communication sequence
  
  const static uint8_t hc12_tx_ = 6; ///< HC12 Tx pin
  const static uint8_t hc12_rx_ = 5; ///< HC12 Rx pin
  const static uint8_t hc12_set_ = 4; ///< HC12 Set pin

public:
  /** Constructor */
  CPG_Slave():
  CPG(hc12_tx_, hc12_rx_, hc12_set_, led_red_, serial_timeout_ms_)
  {}

/// PACKET QUERIES
private:
  /** Query CPG Info */
  void replyCPGInfo(uint8_t address, const CPGInfoReply * c)
  {
    queryPacket(address, cmd_CPGInfoReply, (uint8_t *)c, sizeof(CPGInfoReply)); 
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
    for(uint8_t i = 0; i < sizeof(switches_); ++i)
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
    char buf[10] = "";
    sprintf(buf, "sw %d", val);
    debug(buf);
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
        debug((char *)"Detected pulse");
        ledBlinkStart(led_yellow_);
      }
      pulse_timestamp_ = millis();
    }
  }

  /** Start blinking LED */
  void ledBlinkStart(pin_t led)
  {
    ledControl(led, led_On);
    switch(led)
    {
      case led_yellow_: led_yellow_timestamp_ = millis(); break;
      case led_red_: led_red_timestamp_ = millis(); break;
    }    
  }

  /** Reset blinking LED */
  void ledBlinkReset()
  {
    if ((millis() - led_yellow_timestamp_) > pulse_blink_ms_)
      ledControl(led_yellow_, led_Off);

    if ((millis() - led_red_timestamp_) > tx_blink_ms_)
      ledControl(led_red_, led_Off);
  }

public:
  /** Setup */
  void setup()
  {
    #ifdef DEBUG  
    USB.begin(usb_baudrate_);
    #endif // DEBUG
    
    ledSetup();
  
    HC12_setup_retry(home_channel_); 

    ledControl(led_green_, led_On);
    
    switchesSetup();
    setAddress(readSwitches());
    
    cpgInputsSetup();
    
    delay(init_delay_ms);
  }
  
  /** Loop */
  void loop() {  

    res_t r = Ok;
    
    // Turn off blink LEDs
    ledBlinkReset();

    // Pulse detection
    samplePulse();   

    // Receive data
    if (HC12.available())
    {
      debug((char *)"Received something");
      // Wait for reply
      size_t l = HC12.readBytesUntil(0, rx_buffer_, sizeof(rx_buffer_));

      // Process reply
      if (l)
      {        
        packet_t *p = &rx_packet_;
        debug((char *)"Processing");
        r = packetRx(p, rx_buffer_, l);
        if (r == Ok) 
        {     
          debug((char *)"packetrx ok");
          // CPG Info Query           
          if (p->command == cmd_CPGInfoQuery && 
            p->data_size == sizeof(CPGInfoQuery)) 
          {
            CPGInfoQuery *qry = (CPGInfoQuery*)p->data;
            if (qry->cpg_sequence == sequence_)
            {
              debug((char *)"Reset backup");
              ledBlinkStart(led_red_); 
              pulse_backup_ = 0;
              ++sequence_;              
            }
            pulse_backup_ += pulse_count_;
            pulse_count_ = 0;
            // Transmit info
            CPGInfoReply c = {
              .cpg_id = address(), 
              .cpg_count = pulse_backup_, 
              .cpg_sequence = sequence_
            };
            replyCPGInfo(master_address_, &c);    
            debug((char *)"Sent info reply");       
          }
          // CPG Init query
          else if (p->command == cmd_CPGInitQuery && 
            p->data_size == sizeof(CPGInitQuery))
          {
            CPGInitQuery *qry = (CPGInitQuery*)p->data;
            if (qry->cpg_address & addressMask())
            {
              HC12_setup_retry(qry->cpg_channel);
              ledBlinkStart(led_red_); 
            }
          }
          else // Command mismatch
          {
            debug((char *)"Command error");
            r = ECommand;            
          }
        }
        else
        {
          debug((char *)"Packet error");
        }
      }
      else
      {
        debug((char *)"Read error");
      }
    }
  }  
};

#endif // SUMITOMO_CPGS_SLAVE_H


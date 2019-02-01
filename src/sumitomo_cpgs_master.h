/** @file
  CPG Master implementation

  Defines the CPG_Master class, wich inherits from CPG. 

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
#include "sumitomo_cpgs_config.h"

#ifndef SUMITOMO_CPGS_MASTER_H
#define SUMITOMO_CPGS_MASTER_H

class CPG_Master:CPG
{

private:
  /// GET CONFIGURATION FROM FILE
  const static uint8_t slave_number_ = {
    SUMITOMO_CPGS_CONFIG_SLAVE_NUMBER
  }; ///< Number of slaves for this master
  const uint8_t slaves_[slave_number_] = {
     #include "../cfg/slaves_config.h"
  }; ///< Slaves addressable by this master
  const static uint8_t master_channel_ = {
    #include "../cfg/master_channel_config.h"
  }; ///< Master HC12 channel

  /// CONFIGURATION
  const static pin_t led_blue_ = 8; ///< Blue LED
  const static pin_t hc12_tx_ = 3; ///< HC12 Tx pin
  const static pin_t hc12_rx_ = 2; ///< HC12 Rx pin
  const static pin_t hc12_set_ = 7; ///< HC12 Set pin
  
  const static uint16_t rx_blink_ms_ = 200; ///< Blink time on RX
  const static uint16_t reply_timeout_ms = 100; ///< Reply timeout
  const static uint8_t fault_threshold_ = 10; ///< Allowed slave faults

  /// VARIABLES
  uint32_t led_timestamp_ = 0; ///< Timestamp of LED turn on
  uint32_t serial_timestamp_ = 0; ///< Timestamp of serial

  uint8_t sequences_[slave_number_] = {0}; ///< Sequences of slaves
  uint8_t faults_[slave_number_] = {0}; ///< Faults of slaves

public:
  /** Constructor */
  CPG_Master():
  CPG(hc12_tx_,hc12_rx_,hc12_set_, led_blue_)
  {
    setAddress(master_address_);
  }

/// PACKET QUERIES
private:
  /** Query CPG Info */
  void queryCPGInfo(uint8_t address, const CPGInfoQuery * c)
  {
    queryPacket(address, cmd_CPGInfoQuery, (uint8_t *)c, sizeof(c)); 
  }

  /** Query CPG Init */
  void queryCPGInit(const CPGInitQuery * c)
  {
    queryPacket(broadcast_address_, cmd_CPGInitQuery, (uint8_t *)c, sizeof(c)); 
  }
  
private:
  /** Setup led module */
  void ledSetup()
  {
    pinMode(led_blue_, OUTPUT);
    ledControl(led_blue_, led_On);
  }

public: 
  /** Setup */ 
  void setup()
  {
    USB.begin(usb_baudrate_);
    
    ledSetup();
    res_t res = HC12_setup(home_channel_);
    if (res != Ok)
      error();
      
    uint32_t init_slaves_ = 0;
    for (uint8_t i = 0; i < slave_number_; ++i)
      init_slaves_ |= 1<<slaves_[i];    
      
    CPGInitQuery c = {
      .cpg_channel = master_channel_, 
      .cpg_address = init_slaves_
    };
    queryCPGInit(&c);
    
    HC12_setup(master_channel_);
  }

  /** Loop */
  void loop() 
  {
    res_t r = Ok;
    uint32_t init_slaves_ = 0;
    for (uint8_t i; i < slave_number_; ++i)
    {
      // Clean rx buffer
      while(HC12.available()) 
        HC12.read();

      // Query Info
      CPGInfoQuery c = {.cpg_sequence = sequences_[i]};
      queryCPGInfo(slaves_[i], &c);

      // Wait for reply
      size_t l = HC12.readBytesUntil(0, rx_buffer_, sizeof(rx_buffer_));

      // Process reply
      if (l)
      {        
        packet_t *p = &rx_packet_;
        r = packetRx(p, rx_buffer_, l-1);
        if (r == Ok) 
        {                
          if (p->command == cmd_CPGInfoReply && 
            p->data_size == sizeof(CPGInfoReply)) 
          {
            CPGInfoReply *rpy = (CPGInfoReply*)&p->data;
            if (rpy->cpg_id == slaves_[i])
            {
              sequences_[i] = rpy->cpg_sequence;
              for (uint8_t j = 0; j < rpy->cpg_count; ++j)
                USB.println(rpy->cpg_id);
              faults_[i] = 0;
            }
            else // Id mismatch
              r = EId;
          }
          else // Command mismatch
            r = ECommand;            
        }
      }
      else // No incoming data
      {
        ++faults_[i];
        if (faults_[i] > fault_threshold_)
          init_slaves_ |= 1<<slaves_[i];
      }
    }

    if (init_slaves_)
    {
      HC12_setup(home_channel_);
      CPGInitQuery c = {
        .cpg_channel = master_channel_, 
        .cpg_address = init_slaves_
      };
      queryCPGInit(&c);
      HC12_setup(master_channel_);
    }
  }
};

#endif // SUMITOMO_CPGS_MASTER_H


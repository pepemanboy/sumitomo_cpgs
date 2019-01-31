#include "ciropkt.h"
#include "ciropkt_cmd.h"
#include "Arduino.h" 
#include <SoftwareSerial.h>

/// MACROS
#define USB Serial

#ifdef DEBUG
#define debug(s) USB.println(s);
#else
#define debug(s) (void)0
#endif // DEBUG
/// END MACROS

class CPG
{  

/// ENUMS
protected:
  typedef enum
  {
    led_On = 0,
    led_Off,
  }led_e;
  
/// CONFIGURATION
protected:
  const uint8_t master_address_ = 0; ///< Address of the master
  const uint16_t serial_timeout_ms_ = 0; ///< Serial timeout [ms]
  const uint32_t hc12_baudrate_ = 9600; ///< HC12 baudrate [bps]
  const uint32_t usb_baudrate_ = 9600; ///< USB baudrate [bps]

/// VARIABLES
protected:
  char rx_buffer_[255]; ///< Rx buffer
  pkt_VAR(rx_packet_); ///< Rx packet
  size_t rx_count_ = 0; ///< Rx read bytes counter

  char tx_buffer_[pkt_MAXSPACE +1]; ///< Tx buffer
  size_t tx_buffer_length_ = 0; ///< Tx buffer length
  pkt_VAR(tx_packet_); ///< Tx packet
  
  SoftwareSerial HC12; ///< HC12 communication  

  uint8_t hc12_channel_ = 1; ///< HC12 channel
  uint8_t hc12_tx_; ///< HC12 Tx pin
  uint8_t hc12_rx_; ///< HC12 Rx pin
  uint8_t hc12_set_; ///< HC12 Set pin
  uint8_t led_error_; ///< LED error indicator  

/// FUNCTIONS
protected:
  
  /** Constructor */
  CPG(const uint8_t hc12_tx, const uint8_t hc12_rx, const uint8_t hc12_set):
  hc12_tx_(hc12_tx),
  hc12_rx_(hc12_rx),
  hc12_set_(hc12_set),
  HC12(hc12_rx, hc12_tx)
  {}

  /** Error function */
  void error()
  {
    while(1)
    {
      digitalWrite(led_error_, HIGH);
      delay(500);
      digitalWrite(led_error_, LOW);
      delay(500);
    }
  }

  /** Configure HC 12 parameter */
  res_t HC12_configure_parameter(char * query)
  {
    res_t r;
    HC12.println(query);
    debug(query);
    size_t l = HC12.readBytes(rx_buffer_, sizeof(rx_buffer_));
    rx_buffer_[l] = '\0';
    debug(rx_buffer_);
    if (!strstr(rx_buffer_, "OK")) 
      return EFormat;
    return Ok;
  }
  
  /** Setup HC12 module */
  res_t HC12_setup()
  {    
    res_t r;
    pinMode(hc12_set_, OUTPUT);
  
    HC12.setTimeout(serial_timeout_ms_);
    HC12.begin(9600); // Default baudrate per datasheet
    
    debug((char *)"Setting set pin LOW");
    digitalWrite(hc12_set_, LOW); // Enter setup mode
    delay(250); // As per datasheet
  
    char buf[20];
    
    // Set channel
    debug((char *)"Configuring channel");
    sprintf(buf, "AT+C%03hu", hc12_channel_); 
    r = HC12_configure_parameter(buf);
    if (r != Ok)
      return r;
      
    // Set baud rate
    debug((char *)"Configuring baudrate");
    sprintf(buf, "AT+B%lu", hc12_baudrate_);
    r = HC12_configure_parameter(buf);
    if (r != Ok)
      return r;
      
    HC12.begin(hc12_baudrate_);  
  
    debug((char *)"Setting pin HIGH");
    digitalWrite(hc12_set_, HIGH); // Exit setup mode
    delay(250); // As per datasheet
    
    return Ok;
  }

  /** Set HC12 Channel */
  res_t setHC12Channel(uint8_t channel) 
  {    
    res_t r = HC12_setup();
    if (r == Ok)
      hc12_channel_ = channel;
    return r;
  };
  
  /** Control leds */
  void ledControl(uint8_t led, uint8_t state)
  {
    digitalWrite(led, state == led_On ? LOW : HIGH);
  }

public:
  void setup();
  void loop();
  
};












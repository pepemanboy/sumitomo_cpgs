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
/// TYPEDEFS
protected:
  typedef uint8_t pin_t;
  
/// CONFIGURATION
protected:
  const uint8_t master_address_ = 0; ///< Address of the master
  const uint8_t broadcast_address_ = 255; ///< Address of broadcast
  const uint16_t serial_timeout_ms_ = 10; ///< Serial timeout [ms]
  const uint32_t hc12_baudrate_ = 9600; ///< HC12 baudrate [bps]
  const uint32_t usb_baudrate_ = 9600; ///< USB baudrate [bps]
  const uint8_t home_channel_ = 1; ///< Home HC12 channel 

private:
  pin_t led_error_; ///< LED error indicator  
  pin_t hc12_tx_; ///< HC12 Tx pin
  pin_t hc12_rx_; ///< HC12 Rx pin
  pin_t hc12_set_; ///< HC12 Set pin  
  uint8_t hc12_channel_ = home_channel_; ///< HC12 channel  

/// VARIABLES
protected:
  char rx_buffer_[255]; ///< Rx buffer  
  size_t rx_buffer_length_ = 0; ///< Rx read bytes counter
  pkt_VAR(rx_packet_); ///< Rx packet

  char tx_buffer_[pkt_MAXSPACE +1]; ///< Tx buffer
  size_t tx_buffer_length_ = 0; ///< Tx buffer length
  pkt_VAR(tx_packet_); ///< Tx packet    
  
  SoftwareSerial HC12; ///< HC12 communication  

  uint8_t address_ = 0;

/// FUNCTIONS
protected:  
  /** Constructor */
  CPG(pin_t hc12_tx, pin_t hc12_rx, pin_t hc12_set, pin_t led_error):
  hc12_tx_(hc12_tx),
  hc12_rx_(hc12_rx),
  hc12_set_(hc12_set),
  HC12(hc12_rx, hc12_tx),
  led_error_(led_error)
  {}

  /** Set and get address */
  void setAddress(uint8_t address) { address_ = address;}
  uint8_t address() { return address_; }

  /** Get HC12 channel */
  uint8_t HC12Channel() { return hc12_channel_; }

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

  /** Query packet */
  void queryPacket (uint8_t address, uint8_t command, uint8_t * data, size_t data_length)
  {
    packet_t *p = &tx_packet_;
    p->address = address;
    p->command = command;      
    pktUpdate(p, &data, data_length);      
    pktRefresh(p);
    size_t len = sizeof(tx_buffer_);
    pktSerialize(p, (uint8_t *) tx_buffer_, &len);
    HC12.write(tx_buffer_, len);
    HC12.write((uint8_t)0); 
    HC12.flush();
  }

  /** Process received packet in buffer and store it in a packet*/
  res_t packetRx(packet_t *p, const char *buf, size_t buf_len)
  {
    res_t r = pktDeserialize(p, (uint8_t *)&buf, buf_len);
    if (!r) {
      return EParse; 
    }
    else
    {       
      if (pktCheck(p))
      {
        if (p->address != address() && p->address != broadcast_address_) {
          return EAddress;
        }        
      }
      else {
        return EFormat;
      }
    }   
  }

private:
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

protected:
  /** Setup HC12 module */
  res_t HC12_setup(uint8_t channel)
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
    sprintf(buf, "AT+C%03hu", channel); 
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
    hc12_channel_ = channel; // Acknowledge channel set
    
    return Ok;
  }

  /** Led control */
  typedef enum
  {
    led_On = 0,
    led_Off,
  }led_e;
  
  /** Control leds */
  void ledControl(pin_t led, led_e state)
  {
    digitalWrite(led, state == led_On ? LOW : HIGH);
  }

  /** Wait for HC12 reply */
  res_t HC12WaitReply(uint16_t timeout_ms)
  {
    int16_t i = 0;
    while(!HC12.available())
    {
      delay(1);
      ++i;
      if(i > timeout_ms)
        return Error;
    }
    return Ok;
  }

public:
  void setup();
  void loop();
  
};












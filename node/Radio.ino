//includes
#include <nRF905.h>
#include <SPI.h>
#include <stdint.h>
#include <EEPROM.h>

//#include <avr/io.h>
//#include <avr/interrupt.h>



//radio property variables
byte DEFAULT_RADIO_ADDR[] = {0x00, 0x00, 0x00, 0x01}; // Address of this device (4 bytes)
#define HUB_ADDR {0x00, 0x00, 0x00, 0x00} // Address of device to send to (4 bytes)
bool isConfigured;


//Radio Setup
void radio_setup() {
  Serial.println("Radio Setup");
  
  // Start up
  nRF905_init();

  //Converter
  uint32_t rx_addr; 
//  rx_addr = byte2int(DEFAULT_RADIO_ADDR);

  //Set my address
//  Serial.print("Default Radio Address: ");
//  Serial.println(rx_addr);
//  nRF905_setRXAddress(DEFAULT_RADIO_ADDR);
//  protocol_set_my_address(rx_addr);
  
  // Set address of device to send to
  byte tx_addr[] = HUB_ADDR;
  nRF905_setTXAddress(tx_addr);

  if(!isConfiguredFunc()){
    Serial.print("Default Radio Address: ");
    rx_addr = byte2int(DEFAULT_RADIO_ADDR);
    Serial.println(rx_addr);
    nRF905_setRXAddress(DEFAULT_RADIO_ADDR);
    protocol_set_my_address(rx_addr);
    configure_address();
  }
  else {
    //set RX addr to stored addr in EEPROM
    byte rx_addr_byteArray[4];
    for(int i = 0; i < 4; i++) {
      rx_addr_byteArray[i] = EEPROM.read(i); 
    }

    Serial.print("Radio Address: ");
    rx_addr = byte2int(rx_addr_byteArray);
    Serial.println(rx_addr);
    nRF905_setRXAddress(rx_addr_byteArray);
    protocol_set_my_address(rx_addr);

    isConfigured = true;
  }
}

//functions here
void configure_address() {
  //send message init config message
  sendPacket_config();
  //wait for response from hub
  while(!getPacket()) {
    //resend init config packet if the config button is pressed
    //TODO: if(getConfigButtonPressed) sendPacket_config;
  }
  
  
}

int initVal = 255;
bool isConfiguredFunc(){
  for(int i = 0; i < 4; i++) {
    if(initVal != EEPROM.read(EEPROM_ADDR_START_BYTE+i)) {
      Serial.println("The radio address is already configured");
      return true; //if at least one is different from the default address, we are configured
    }
  }
  Serial.println("The radio address is NOT configured yet");
  return false;
}

bool getIsConfigured() {
  return isConfigured;
}

void updateEEPROMAddress(byte *new_address) {
  Serial.println("Updating EEPROM with new address");
  for(int i = 0; i < 4; i++) {
    EEPROM.update(EEPROM_ADDR_START_BYTE+i, new_address[i]);
  }
}

void radio_loop() {
  if(getPacket()) {
    //Serial.println("Got a message.");
  }
  else {
    //Serial.println("No message received.");
  }


  if(isReadyToSend()) {
    //send status packet
    sendPacket_status();
    doneSending();
  }
}

//Send packet ack
void sendPacket_ack() {
  Serial.println("Send ack packet.");
  byte tmpBuff[NRF905_MAX_PAYLOAD];
  create_msg_ack(tmpBuff);

  // Set payload data
  nRF905_setData(tmpBuff, NRF905_MAX_PAYLOAD);

  // Send payload (send fails if other transmissions are going on, keep trying until success)
  while(!nRF905_send());

  nRF905_receive();
}

// Send a init config packet
static void sendPacket_config()
{
  Serial.println("Send init config packet.");

  // Convert packet data to plain byte array
  byte tmpBuff[NRF905_MAX_PAYLOAD];
  create_msg_init_config(tmpBuff);
  
  // Set payload data
  nRF905_setData(tmpBuff, NRF905_MAX_PAYLOAD);

  // Send payload (send fails if other transmissions are going on, keep trying until success)
  while(!nRF905_send());

  nRF905_receive();
}

void sendPacket_status() {
  //Serial.println("Send status packet.");

  // Convert packet data to plain byte array
  byte tmpBuff[NRF905_MAX_PAYLOAD];
  float temp[SENSOR_RDG_PER_PACKET] = {10, 50, 42.3, 98.6, 0};
  create_msg_status(tmpBuff, getRelayState(), getSwitchState(), temp);
  
  // Set payload data
  nRF905_setData(tmpBuff, NRF905_MAX_PAYLOAD);

  // Send payload (send fails if other transmissions are going on, keep trying until success)
  while(!nRF905_send());

  //Let current sensor know it can start collecting data again
  doneSending();
  
  //Serial.println("Done sending status.");

  nRF905_receive();
}

// Get a packet
static bool getPacket()
{
  nRF905_receive();

  byte buffer[NRF905_MAX_PAYLOAD];

  // See if any data available
  if(!nRF905_getData(buffer, sizeof(buffer)))
    return false;

  //Print message
  //Serial.print("Data incoming, type number: ");
  //Serial.println(buffer[0]);
  
  // Convert byte array to info
  MessageType type = get_msg_type(buffer);
  uint32_t addr = get_msg_address(buffer);

  if(type != MessageType::ACK) {
    sendPacket_ack();
  }

  //Do different things for different messages  
  switch(type) {
    case MessageType::CONFIG:
      Serial.println("Message Type: Config");
      uint32_t newAddrInt;
      decode_msg_config(buffer, newAddrInt); //Note: with new protocol, switched from &newAddr to newAddr
      protocol_set_my_address(newAddrInt);

      Serial.print("Address int: ");
      Serial.println(newAddrInt);

      //Convert to byte[]
      byte new_address[4];
      int2byte(newAddrInt, new_address);

      Serial.println(new_address[0], HEX);
      Serial.println(new_address[1], HEX);
      Serial.println(new_address[2], HEX);
      Serial.println(new_address[3], HEX);
      
      
      nRF905_setRXAddress(new_address);
      Serial.print("Address Converted from byte array: ");
      newAddrInt = byte2int(new_address);
      Serial.println(newAddrInt);
      updateEEPROMAddress(new_address);
      isConfigured = true;
      sendPacket_status();
      break;
    case MessageType::ON:
      Serial.println("Message Type: On");
      set_relay_state_on();
      break;
    case MessageType::OFF:
      Serial.println("Message Type: Off");
      set_relay_state_off();
      break;
    case MessageType::ACK:
      Serial.println("Message Type: Ack");
      break;
    case MessageType::INIT_CONFIG:
      Serial.println("Message Type: Init_config - SHOULDN'T GET THIS MESSAGE");
      break;
    case MessageType::STATUS:
       Serial.println("Message Type: Status");
      break;
  }
  
  return true;
}


//Conversion functions
//This not working
uint32_t byte2int(uint8_t* input) {
  uint32_t val;
  val = (uint32_t) input[0] << 24;
  val |=  (uint32_t) input[1] << 16;
  val |= (uint32_t) input[2] << 8;
  val |= (uint32_t) input[3];
  return val;
}

void int2byte(uint32_t value, byte* result) {
  result[3] = (byte)(value & 0x000000ff);
  result[2] = (byte)((value & 0x0000ff00) >> 8);
  result[1] = (byte)((value & 0x00ff0000) >> 16);
  result[0] = (byte)((value & 0xff000000) >> 24);
}


/**************************************************************************/
/*!
    @file     DAC80501.cpp
    @author   Masa

        I2C Driver for DAC80501/TI

        @section  HISTORY

*/
/**************************************************************************/

#include "DAC80501.h"

/**************************************************************************/
/*!
    @brief  Instantiates a new DAC80501 class
*/
/**************************************************************************/
DAC80501::DAC80501() {}

/**************************************************************************/
/*!
    @brief  Setups the hardware and checks the DAC was found
    @param i2c_address The I2C address of the DAC, defaults to 0x90
    @param wire The I2C TwoWire object to use, defaults to &Wire
    @returns True if DAC was found on the I2C address.
*/
/**************************************************************************/
bool DAC80501::begin(uint8_t i2c_address, TwoWire *wire) {
  if (i2c_dev) {
    delete i2c_dev;
  }

  i2c_dev = new Adafruit_I2CDevice(i2c_address, wire);

  if (!i2c_dev->begin()) {
    return false;
  }

  return true;
}

/**************************************************************************/
/*!
    @brief  Initialize DAC80501
            VFS=2.5V, Async output update, enable output and internal VREF

    @returns True if no alarm on the device, False otherwise.
*/
/**************************************************************************/
bool DAC80501::init(void) {

  uint8_t packet[3];

  packet[0] = DAC80501::CMD::CMD_TRIGGER;
  packet[1] = 0x00;
  packet[2] = SOFT_RES ; //RESET command
  
  if (!i2c_dev->write(packet, 3)) {
    return false;
  }

  delay(10); // wait for restarting

  packet[0] = DAC80501::CMD::CMD_SYNC;
  packet[1] = 0x00;
  packet[2] = DAC80501::DAC_SYNC_EN::UPDATE_ASYNC; //the output is update immedietely
  if (!i2c_dev->write(packet, 3)) {
    return false;
  }
  
  packet[0] = DAC80501::CMD::CMD_CONFIG;
  packet[1] = DAC80501::REF_PWDWN::REFPWDWN_DISABLE; //use internal VREF 2.5V
  packet[2] = DAC80501::DAC_PWDWN::DACPWDN_DISABLE; //activate DAC

  if (!i2c_dev->write(packet, 3)) {
    return false;
  }

  // In case of that the Vcc = 3.3V, VREF setting must be as follows
  packet[0] = DAC80501::CMD::CMD_GAIN;
  packet[1] = DAC80501::REF_DIV::REFDIV_2;     // VREF divider = 1/2 
  packet[2] = DAC80501::BUFF_GAIN::BUFGAIN_2;  // DAC Buffer gain =2 ,thus VFS=2.5V

  if (!i2c_dev->write(packet, 3)) {
    return false;
  }

  DAC80501::DAC_VOLT2LSB = 65535 / 2.5;

  // check the status
  packet[0] = DAC80501::CMD::CMD_STATUS;
  if (!i2c_dev->write(packet, 1)) {
    return false;
  }

  //  Read 2byte of spacified resigter.
  if (!i2c_dev->read(packet,2,true)) {
    return false;
  }

  // return ture STATUS::REF-ALARM==0 
  return (packet[1] & 0x01)==0 ;

  // Reading protocol:
  //  Send a command byte for the register to be read.
  // packet[0] = DAC80501_CMD::CMD_DEVID;

  // if (!i2c_dev->write(packet, 1)) {
  //   return false;
  // }

  // //  Read 2byte of spacified resigter.
  // if (!i2c_dev->read(packet,2,true)) {
  //   return false;
  // }

}

/**************************************************************************/
/*!
    @brief  Sets the output voltage to a fraction of source vref.  (Value
            can be 0..65535)

    @param[in]  output
                The 16-bit value representing the relationship between
                the DAC's input voltage and its output voltage.

    @param i2c_frequency What we should set the I2C clock to when writing
    to the DAC, defaults to 400 KHz
    @returns True if able to write the value over I2C
*/
/**************************************************************************/
bool DAC80501::setVoltage(const uint16_t output, 
                          const uint32_t i2c_frequency) {
  i2c_dev->setSpeed(i2c_frequency); // Set I2C frequency to desired speed

  uint8_t packet[3];


  packet[0] = DAC80501::CMD::CMD_DAC_BUF;
  packet[1] = output / 256;        // Upper data bits (D15.....D8)
  packet[2] = (output % 256);      // Lower data bits (D7......D0)

  if (!i2c_dev->write(packet, 3)) {
    return false;
  }

  i2c_dev->setSpeed(100000); // reset to arduino default
  return true;
}



/**************************************************************************/
/*!
    @brief  Sets the output voltage to a fraction of source vref.  (0 -- 2.5V)

    @param[in]  output
                absolute voltage [V] to be output. Assuming VFS=2.5V

    @param i2c_frequency What we should set the I2C clock to when writing
    to the DAC, defaults to 400 KHz
    @returns True if able to write the value over I2C
*/
/**************************************************************************/
bool DAC80501::setVoltage(const float output, 
                          const uint32_t i2c_frequency) {
  i2c_dev->setSpeed(i2c_frequency); // Set I2C frequency to desired speed

  uint8_t packet[3];

  if (  output < 0.0 || output > 2.5 ){
    return false;
  };

  uint16_t output_bin = output * DAC80501::DAC_VOLT2LSB;

  packet[0] = DAC80501::CMD::CMD_DAC_BUF;
  packet[1] = output_bin / 256;        // Upper data bits (D15.....D8)
  packet[2] = (output_bin % 256);      // Lower data bits (D7......D0)

  if (!i2c_dev->write(packet, 3)) {
    return false;
  }

  i2c_dev->setSpeed(100000); // reset to arduino default
  return true;
}
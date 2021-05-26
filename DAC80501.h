/**************************************************************************/
/*!
    @file     DAC80501.h
*/
/**************************************************************************/

#ifndef _DAC80501_H_
#define _DAC80501_H_

#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Wire.h>


constexpr uint8_t DAC80501_I2CADDR_DEFAULT=0x48; ///< Default i2c address
// A0 pin = GND (0x48 = Default)
// A0 pin = VDD (0x49)
// A0 pin = SDA (0x4A)
// A0 pin = SCL (0x4B)                                     

/**************************************************************************/
/*!
    @brief  Class for communicating with an DAC80501 DAC
*/
/**************************************************************************/
class DAC80501 {
public:
  //  command byte table:
  enum CMD{
  CMD_NOOP,     //0[W]
  CMD_DEVID,    //1[R] device identification : read resolution & Reset Setting
  CMD_SYNC,     //2[RW] synchronization : DAC output sync setting
  CMD_CONFIG,   //3[RW] configuration :
  CMD_GAIN,     //4[RW] Gain
  CMD_TRIGGER,  //5[W] Trigger
  CMD_NA6,      //6 :not appicable.
  CMD_STATUS,   //7[R] Status
  CMD_DAC_BUF       //8[RW] DAC
  };

  // for DEVID command
  enum RESOLUTION{
    RESOLUTION_16BIT,  //0x0
    RESOLUTION_14BIT,  //0x1
    RESOLUTION_12BIT   //0x2
  };

  enum RSTSEL{
    RESET_TO_ZERO,   // 0 : set to Zero at RESET
    RESET_TO_MID     // 1 : set to midscale at RESET
  };

  // for SYNC command
  enum DAC_SYNC_EN{
    UPDATE_ASYNC,  // 0 : the output is updated immediately (DEFAULT)
    UPDATE_SYNC    // 1 : the output is updated in respond to LDAC signal
  };

  // for CONFIG command
  enum REF_PWDWN{
    REFPWDWN_DISABLE,  // 0 : Activate internal VREF (2.5V)
    REFPWDWN_ENABLE    // 1 : PowerDown(de-activate) internal VREF (2.5V)
  };

  enum DAC_PWDWN{
    DACPWDN_DISABLE,  // 0 : Activate DAC 
    DACPWDN_ENABLE    // 1 : PowerDown(de-activate) the DAC 
  };

  // for GAIN command
  enum REF_DIV{
    REFDIV_1,     // 0 : VREF Divider set to x1 (THROUGH)
    REFDIV_2      // 1 : VREF Divider set to 1/2 (Half of Vref is applied to the DAC)
  };
  enum BUFF_GAIN{
    BUFGAIN_1,     // 0 : the gain of DAC BufferAmp is set to x1
    BUFGAIN_2      // 1 : the gain of DAC BufferAmp is set to x2
  };

  // for TRIGGER command
  enum LDAC{
    LDAC_NA,   // 0 : !NOT APPLICABLE! 
    LDAC_SET   // 1 : Activate LDAC
  };

public:
  DAC80501();
  bool begin(uint8_t i2c_address = DAC80501_I2CADDR_DEFAULT,
             TwoWire *wire = &Wire);
  
  bool init(void);

  bool setVoltage(const uint16_t output,
                  const uint32_t dac_frequency = 400000);

  bool setVoltage(const float output,
                  const uint32_t dac_frequency = 400000);

private:
  Adafruit_I2CDevice *i2c_dev = NULL;
  float DAC_VOLT2LSB = 0.0;
  const uint8_t SOFT_RES=0xA; // Soft Reset code


};

#endif

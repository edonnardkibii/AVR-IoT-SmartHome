/*
 * File:   handleSensors.c
 * Author: M67732
 *
 * Created on October 28, 2022, 4:45 PM
 */


#include <xc.h>
#include "handleSensors.h"
#include "mcc_generated_files/mcc.h"



#define MCP9809_ADDR				0x18 
#define MCP9808_REG_TA				0x05
#define LIGHT_SENSOR_ADC_CHANNEL	5


uint16_t getLightSensorValue(void) {
    
  return ADC0_GetConversion(LIGHT_SENSOR_ADC_CHANNEL);

}

int16_t getTempSensorValue (void) {

    int32_t temperature;
    
    temperature = i2c_read2ByteRegister(MCP9809_ADDR, MCP9808_REG_TA);
    
    temperature = temperature << 19;
    temperature = temperature >> 19;
    
    temperature *= 100;
    temperature /= 16;
    
    return temperature;


}


/*
float getTempSensorValue (void) {

    //int32_t temperature;
    int16_t temperature;
    float ambient_temperature;
    unsigned int temperature_upper_byte;
    unsigned int temperature_lower_byte;
    
    temperature = i2c_read2ByteRegister(MCP9809_ADDR, MCP9808_REG_TA);
    
    temperature_upper_byte = (temperature >> 8) & (0xFF00);
    temperature_lower_byte = temperature & (0xFF);
    
    //Convert the temperature data
    if((temperature_upper_byte & 0x80) == 0x80)
    {
        //TA > T_crit
    }
    if((temperature_upper_byte & 0x40) == 0x40)
    {
        //TA > T_upper
    }
    if((temperature_upper_byte & 0x20) == 0x20)
    {
        //TA > T_lower
    }
    
    
    temperature_upper_byte = temperature_upper_byte & 0x1F; //Clear flag bits
    
    if((temperature_upper_byte & 0x10) == 0x10)
    {
        //TA < 0°C
        temperature_upper_byte = temperature_upper_byte & 0x0F; //Clear Sign
        
        ambient_temperature = 256 - (temperature_upper_byte * 16 + temperature_lower_byte/16);
    }
    else
    {
        ambient_temperature = (temperature_upper_byte * 16 + temperature_lower_byte/16);
    }
        
    
    //temperature = temperature << 19;
    //temperature = temperature >> 19;
    
    //temperature *= 100;
    //temperature /= 16;
    //printf("%d", ambient_temperature);
    return ambient_temperature;


}
*/
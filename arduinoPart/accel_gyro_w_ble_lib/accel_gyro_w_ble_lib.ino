/*****************************************************************************/
/*INCLUDES                                                                   */
/*****************************************************************************/
#include "Arduino.h"
/* For the bluetooth funcionality */
#include <ArduinoBLE.h>
/* For the use of the IMU sensor */
#include "Nano33BLEAccelerometer.h"
#include "Nano33BLEGyroscope.h"

/*****************************************************************************/
/*MACROS                                                                     */
/*****************************************************************************/
/* 
 * We use strings to transmit the data via BLE, and this defines the buffer
 * size used to transmit these strings. Only 20 bytes of data can be 
 * transmitted in one packet with BLE, so a size of 20 is chosen the the data 
 * can be displayed nicely in whatever application we are using to monitor the
 * data.
 */
#define BLE_BUFFER_SIZES             20
/* Device name which can be scene in BLE scanning software. */
#define BLE_DEVICE_NAME                "Arduino Nano 33 BLE Sense"
/* Local name which should pop up when scanning for BLE devices. */
#define BLE_LOCAL_NAME                "Accelerometer And Gyro BLE"

/*****************************************************************************/
/*GLOBAL Data                                                                */
/*****************************************************************************/
/* 
 * Nano33BLEAccelerometerData object which we will store data in each time we read
 * the accelerometer data. 
 */ 
Nano33BLEAccelerometerData accelerometerData;
Nano33BLEGyroscopeData gyroscopeData;

/* 
 * Declares the BLEService and characteristics we will need for the BLE 
 * transfer. The UUID was randomly generated using one of the many online 
 * tools that exist. It was chosen to use BLECharacteristic instead of 
 * BLEFloatCharacteristic was it is hard to view float data in most BLE 
 * scanning software. Strings can be viewed easiler enough. In an actual
 * application you might want to transfer floats directly.
 */
 
BLEService BLEAccelerometerAndGyro("590d65c7-3a0a-4023-a05a-6aaf2f22441c");
BLECharacteristic accelerometerXBLE("0004", BLERead | BLENotify | BLEBroadcast, BLE_BUFFER_SIZES);
BLECharacteristic accelerometerYBLE("0005", BLERead | BLENotify | BLEBroadcast, BLE_BUFFER_SIZES);
BLECharacteristic accelerometerZBLE("0006", BLERead | BLENotify | BLEBroadcast, BLE_BUFFER_SIZES);
BLECharacteristic gyroscopeXBLE("0014", BLERead | BLENotify | BLEBroadcast, BLE_BUFFER_SIZES);
BLECharacteristic gyroscopeYBLE("0015", BLERead | BLENotify | BLEBroadcast, BLE_BUFFER_SIZES);
BLECharacteristic gyroscopeZBLE("0016", BLERead | BLENotify | BLEBroadcast, BLE_BUFFER_SIZES);

/* Common global buffer will be used to write to the BLE characteristics. */
char bleBuffer[BLE_BUFFER_SIZES];

/*****************************************************************************/
/*SETUP (Initialisation)                                                     */
/*****************************************************************************/
void setup()
{
    /* 
     * Serial setup. This will be used to transmit data for viewing on serial 
     * plotter 
     */
    Serial.begin(115200);
    while(!Serial);


    /* BLE Setup. For information, search for the many ArduinoBLE examples.*/
    if (!BLE.begin()) 
    {
        while (1);    
    }
    else
    {
        BLE.setDeviceName(BLE_DEVICE_NAME);
        BLE.setLocalName(BLE_LOCAL_NAME);
        BLE.setAdvertisedService(BLEAccelerometerAndGyro);
        /* A seperate characteristic is used for each X, Y, and Z axis. */
        BLEAccelerometerAndGyro.addCharacteristic(accelerometerXBLE);
        BLEAccelerometerAndGyro.addCharacteristic(accelerometerYBLE);
        BLEAccelerometerAndGyro.addCharacteristic(accelerometerZBLE);
        /* A seperate characteristic is used for each X, Y, and Z axis of GyroScope*/
        BLEAccelerometerAndGyro.addCharacteristic(gyroscopeXBLE);
        BLEAccelerometerAndGyro.addCharacteristic(gyroscopeYBLE);
        BLEAccelerometerAndGyro.addCharacteristic(gyroscopeZBLE);
        
        BLE.addService(BLEAccelerometerAndGyro);
        BLE.advertise();
        /* 
         * Initialises the IMU sensor, and starts the periodic reading of the 
         * sensor using a Mbed OS thread. The data is placed in a circular 
         * buffer and can be read whenever.
         */
        Accelerometer.begin();
        Gyroscope.begin();

        /* Plots the legend on Serial Plotter */
        Serial.println("X, Y, Z");
    }
}

/*****************************************************************************/
/*LOOP (runtime super loop)                                                  */
/*****************************************************************************/
void loop()
{
    BLEDevice central = BLE.central();
    if(central)
    {
        int writeLength;
        /* 
         * If a BLE device is connected, accelerometer data will start being read, 
         * and the data will be written to each BLE characteristic. The same 
         * data will also be output through serial so it can be plotted using 
         * Serial Plotter. 
         */
        while(central.connected())
        {            
            if(Accelerometer.pop(accelerometerData))
            {
                writeLength = sprintf(bleBuffer, "%f", accelerometerData.x);
                accelerometerXBLE.writeValue(bleBuffer, writeLength); 
                writeLength = sprintf(bleBuffer, "%f", accelerometerData.y);
                accelerometerYBLE.writeValue(bleBuffer, writeLength);      
                writeLength = sprintf(bleBuffer, "%f", accelerometerData.z);
                accelerometerZBLE.writeValue(bleBuffer, writeLength);      

                Serial.printf("%f,%f,%f\r\n", accelerometerData.x, accelerometerData.y, accelerometerData.z);
            }
            
            if(Gyroscope.pop(gyroscopeData))
            {
                writeLength = sprintf(bleBuffer, "%f", gyroscopeData.x);
                gyroscopeXBLE.writeValue(bleBuffer, writeLength); 
                writeLength = sprintf(bleBuffer, "%f", gyroscopeData.y);
                gyroscopeYBLE.writeValue(bleBuffer, writeLength);      
                writeLength = sprintf(bleBuffer, "%f", gyroscopeData.z);
                gyroscopeZBLE.writeValue(bleBuffer, writeLength);      

                Serial.printf("%f,%f,%f\r\n", gyroscopeData.x, gyroscopeData.y, gyroscopeData.z);
            }
        }
    }
}

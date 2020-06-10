struct CayenneLPP; // forward declared for encapsulation

struct CayenneLPP* CayenneLPP__create(unsigned char size);
void CayenneLPP__destroy(struct CayenneLPP* self);
void CayenneLPP__reset(struct CayenneLPP* self);
unsigned char CayenneLPP__getSize(struct CayenneLPP* self);
unsigned char *CayenneLPP__getBuffer(struct CayenneLPP* self);
unsigned char CayenneLPP__copy(struct CayenneLPP* self, unsigned char *dst);
unsigned char CayenneLPP__addDigitalInput(struct CayenneLPP* self, unsigned char value);
unsigned char CayenneLPP__addDigitalOutput(struct CayenneLPP* self, unsigned char value);
unsigned char CayenneLPP__addAnalogInput(struct CayenneLPP* self, float value);
unsigned char CayenneLPP__addAnalogOutput(struct CayenneLPP* self, float value);
unsigned char CayenneLPP__addLuminosity(struct CayenneLPP* self, unsigned short int lux);
unsigned char CayenneLPP__addPresence(struct CayenneLPP* self, unsigned char value);
unsigned char CayenneLPP__addTemperature(struct CayenneLPP* self, float celsius);
unsigned char CayenneLPP__addRelativeHumidity(struct CayenneLPP* self, float rh);
unsigned char CayenneLPP__addAccelerometer(struct CayenneLPP* self, float x, float y, float z);
unsigned char CayenneLPP__addBarometricPressure(struct CayenneLPP* self, float hpa);
unsigned char CayenneLPP__addGyrometer(struct CayenneLPP* self, float x, float y, float z);
unsigned char CayenneLPP__addGPS(struct CayenneLPP* self, float latitude, float longitude, float meters);

//ADD KAMMANIT 29/04/63
unsigned char CayenneLPP__addError_code(struct CayenneLPP* self, unsigned char value);
unsigned char CayenneLPP__addGen(struct CayenneLPP* self, unsigned char value);
unsigned char CayenneLPP__addModel(struct CayenneLPP* self, unsigned char value);
unsigned char CayenneLPP__addPowerINDEX(struct CayenneLPP* self, unsigned char value);
unsigned char CayenneLPP__addNodeID(struct CayenneLPP* self, char* val);
unsigned char CayenneLPP__addParentID(struct CayenneLPP* self, char* val);
unsigned char CayenneLPP__addColor(struct CayenneLPP* self, unsigned short int val);
unsigned char CayenneLPP__addLightControl(struct CayenneLPP* self, unsigned char value);
unsigned char CayenneLPP__addVoltage(struct CayenneLPP* self, float value);
unsigned char CayenneLPP__addPower(struct CayenneLPP* self, unsigned short int val);
unsigned char CayenneLPP__addActuation(struct CayenneLPP* self, uint64_t val);
unsigned char CayenneLPP__addMESHID(struct CayenneLPP* self,uint32_t val);
unsigned char CayenneLPP__addMESHCH(struct CayenneLPP* self,unsigned short int val);

#define LPP_DIGITAL_INPUT 0         // 1 byte
#define LPP_DIGITAL_OUTPUT 1        // 1 byte
#define LPP_ANALOG_INPUT 2          // 2 bytes, 0.01 signed
#define LPP_ANALOG_OUTPUT 3         // 2 bytes, 0.01 signed
#define LPP_LUMINOSITY 101          // 2 bytes, 1 lux unsigned
#define LPP_PRESENCE 102            // 1 byte, 1
#define LPP_TEMPERATURE 103         // 2 bytes, 0.1°C signed
#define LPP_RELATIVE_HUMIDITY 104   // 1 byte, 0.5% unsigned
#define LPP_ACCELEROMETER 113       // 2 bytes per axis, 0.001G
#define LPP_BAROMETRIC_PRESSURE 115 // 2 bytes 0.1 hPa Unsigned
#define LPP_GYROMETER 134           // 2 bytes per axis, 0.01 °/s
#define LPP_GPS 136                 // 3 byte lon/lat 0.0001 °, 3 bytes alt 0.01 meter


//ADD KAMMANIT 29/04/63

#define CM_LPP_ACTUATION                106 /**< 1 byte, 0.5% unsigned */
#define CM_LPP_LIGHT_CONTROL            111 /**< 1 byte, 0-100% unsigned */
#define CM_LPP_VOLTAGE                  116 /**< 2 bytes 1V Unsigned */
#define CM_LPP_CURRENT                  117 /**< 2 bytes 0.01A Unsigned */
#define CM_LPP_POWER                    128 /**< 2 bytes 1A Unsigned */
#define CM_LPP_COLOR                    135 /**< 2 bytes */
#define CM_LPP_ERR_CODE                 146 /**< 1 bytes */
#define CM_LPP_GEN                      147 /**< 1 bytes */
#define CM_LPP_MODEL                    148 /**< 1 bytes */
#define CM_LPP_POWER_INDEX              149 /**< 1 bytes */
#define CM_LPP_NODE_ID                  150 /**<6 bytes */
#define CM_LPP_PARENT_ID                151 /**< 6 bytes */
#define CM_LPP_READ_GPS                 152
#define CM_LPP_MESH_ID                  153
#define CM_LPP_MESH_CH                  154
#define CM_LPP_TYPE_CONTROL             155

// Data ID + Data Type + Data Size
#define LPP_DIGITAL_INPUT_SIZE 3       // 1 byte
#define LPP_DIGITAL_OUTPUT_SIZE 3      // 1 byte
#define LPP_ANALOG_INPUT_SIZE 4        // 2 bytes, 0.01 signed
#define LPP_ANALOG_OUTPUT_SIZE 4       // 2 bytes, 0.01 signed
#define LPP_LUMINOSITY_SIZE 4          // 2 bytes, 1 lux unsigned
#define LPP_PRESENCE_SIZE 3            // 1 byte, 1
#define LPP_TEMPERATURE_SIZE 4         // 2 bytes, 0.1°C signed
#define LPP_RELATIVE_HUMIDITY_SIZE 3   // 1 byte, 0.5% unsigned
#define LPP_ACCELEROMETER_SIZE 8       // 2 bytes per axis, 0.001G
#define LPP_BAROMETRIC_PRESSURE_SIZE 4 // 2 bytes 0.1 hPa Unsigned
#define LPP_GYROMETER_SIZE 8           // 2 bytes per axis, 0.01 °/s
#define LPP_GPS_SIZE 11                // 3 byte lon/lat 0.0001 °, 3 bytes alt 0.01 meter

//ADD KAMMANIT 29/04/63
#define CM_LPP_ACTUATION_SIZE           6   /**< 1 byte, 0.5% unsigned */
#define CM_LPP_LIGHT_CONTROL_SIZE       2   /**< 1 byte, 0-100% unsigned */
#define CM_LPP_VOLTAGE_SIZE             3   /**< 2 bytes 1V Unsigned */
#define CM_LPP_CURRENT_SIZE             3   /**< 2 bytes 0.01 Unsigned */
#define CM_LPP_POWER_SIZE               3   /**< 2 bytes 1A Unsigned */
#define CM_LPP_COLOR_SIZE               3   /**< 2 bytes per axis, 0.01 °/s */
#define CM_LPP_ERR_CODE_SIZE            2   /**< 2 bytes */
#define CM_LPP_GEN_SIZE                 2   /**< 2 bytes */
#define CM_LPP_MODEL_SIZE               2   /**< 2 bytes */
#define CM_LPP_POWER_INDEX_SIZE         2   /**< 2 bytes */
#define CM_LPP_NODE_ID_SIZE             7   /**< 12 bytes */
#define CM_LPP_PARENT_ID_SIZE           7   /**< 7 bytes */
#define CM_LPP_READ_GPS_SIZE            2   /**< 2 bytes */

#define CM_LPP_MESH_ID_SIZE             4   /**< 2 bytes */
#define CM_LPP_MESH_CH_SIZE             2   /**< 2 bytes */
#define CM_LPP_TYPE_CONTROL_SIZE        2   /**< 2 bytes */

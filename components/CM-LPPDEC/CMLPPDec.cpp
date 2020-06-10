
#include "CMLPPDec.h"
#include <CMLPP.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <mlink_utils.h>
#include "cJSON.h"

bool ParseCMLPP (const uint8_t *pBuffer, uint8_t Len, cJSON **root)
{
        *root = cJSON_CreateObject();
        // printf("Len : %d\n", Len);
        while (Len >= 2)
        {
                uint8_t lpp_type = pBuffer[0];

                if (lpp_type == LPP_GPS) {
                        if (Len < LPP_GPS_SIZE)
                                return false;

                        // JsonObject data = root.createNestedObject ();
                        // data["channel"] = channel;
                        // data["type"] = "gps";

                        int32_t tvalue = (int32_t)(pBuffer[1] << 16) | (pBuffer[2] << 8) | pBuffer[3];
                        if ((pBuffer[1] & 0xF0) == 0xF0)
                                tvalue |= 0xFF000000;
                        float value = float (tvalue) / 10000.0f;
                        // data["lat"] = value;
                        tvalue = (pBuffer[4] << 16) | (pBuffer[5] << 8) | pBuffer[6];
                        if ((pBuffer[4] & 0xF0) == 0xF0)
                                tvalue |= 0xFF000000;
                        value = float (tvalue) / 10000.0f;
                        // data["lon"] = value;
                        tvalue = (int32_t)((pBuffer[7] << 16) | (pBuffer[8] << 8) | pBuffer[9]);
                        if ((pBuffer[7] & 0xF0) == 0xF0)
                                tvalue |= 0xFF000000;
                        value = float (tvalue) / 100.0f;
                        // data["alt"] = value;

                        pBuffer += LPP_GPS_SIZE;
                        Len -= LPP_GPS_SIZE;

                } else if (lpp_type == CM_LPP_COLOR) {
                        if (Len < CM_LPP_COLOR_SIZE)
                                return false;


                        uint16_t value = (pBuffer[1] << 8) | pBuffer[2];
                        cJSON_AddNumberToObject(*root, "color", value);



                        pBuffer += CM_LPP_COLOR_SIZE;
                        Len -= CM_LPP_COLOR_SIZE;

                } else if (lpp_type == CM_LPP_LIGHT_CONTROL) {
                        if (Len < CM_LPP_LIGHT_CONTROL_SIZE)
                                return false;


                        // uint8_t value = pBuffer[1];

                        // printf("Brighhtnerr: %d\n", pBuffer[1]);
                        cJSON_AddNumberToObject(*root, "brightness", pBuffer[1]);



                        pBuffer += CM_LPP_LIGHT_CONTROL_SIZE;
                        Len -= CM_LPP_LIGHT_CONTROL_SIZE;

                }  else if (lpp_type == CM_LPP_NODE_ID) {
                        if (Len < CM_LPP_NODE_ID_SIZE)
                                return false;

                        uint8_t nodeid[6] = {0};

                        nodeid[0] = pBuffer[1];
                        nodeid[1] = pBuffer[2];
                        nodeid[2] = pBuffer[3];
                        nodeid[3] = pBuffer[4];
                        nodeid[4] = pBuffer[5];
                        nodeid[5] = pBuffer[6];
                        char mac_str[13] = {0};
                        mlink_mac_hex2str(nodeid, mac_str);

                        cJSON_AddStringToObject(*root, "nodeid", mac_str);


                        pBuffer += CM_LPP_NODE_ID_SIZE;
                        Len -= CM_LPP_NODE_ID_SIZE;

                }else if (lpp_type == CM_LPP_READ_GPS) {
                        if (Len < CM_LPP_READ_GPS_SIZE)
                                return false;

                        cJSON_AddNumberToObject(*root, "read_gps", pBuffer[1]);
                        // JsonObject data = root.createNestedObject ();
                        // // data["channel"] = channel;
                        // data["type"] = "digital_input";
                        // data["value"] = pBuffer[1];

                        pBuffer += CM_LPP_READ_GPS_SIZE;
                        Len -= CM_LPP_READ_GPS_SIZE;

                }
                else if (lpp_type == CM_LPP_TYPE_CONTROL) {
                        if (Len < CM_LPP_TYPE_CONTROL_SIZE)
                                return false;

                        cJSON_AddNumberToObject(*root, "type_control", pBuffer[1]);

                        pBuffer += CM_LPP_TYPE_CONTROL_SIZE;
                        Len -= CM_LPP_TYPE_CONTROL_SIZE;

                }

                 else {
                        return false;
                }
        }
        return (Len == 0);
}

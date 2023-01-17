#include "../mcc.h"
#include "../winc/m2m/m2m_wifi.h"
#include "../winc/socket/socket.h"
#include "../winc/common/winc_defines.h"
#include "../winc/driver/winc_adapter.h"
#include "../mqtt/mqtt_core/mqtt_core.h"
#include "../winc/m2m/m2m_types.h"
#include "../config/mqtt_config.h"
#include "mqtt_example.h"
#include "clock_config.h"
#include <util/delay.h>
#include "../../handleSensors.h"
#include "../../aes.h"
#include "../../pkcs7_padding.h"
#include "../../base64.h"
#include <string.h>
#include <stdio.h>

#define CONN_SSID CFG_MAIN_WLAN_SSID
#define CONN_AUTH CFG_MAIN_WLAN_AUTH
#define CONN_PASSWORD CFG_MAIN_WLAN_PSK

#define CBC 1
#define CTR 1
#define ECB 1

typedef enum {
    APP_STATE_INIT,
    APP_STATE_STA_CONNECTING,
    APP_STATE_STA_CONNECTED,
    APP_STATE_WAIT_FOR_DNS,
    APP_STATE_TLS_START,
    APP_STATE_TLS_CONNECTING,
    APP_STATE_TLS_CONNECTED,
    APP_STATE_ERROR,
    APP_STATE_STOPPED
} appStates_e;

static const char mqttPublishTopic[PUBLISH_TOPIC_SIZE] = CFG_PUBTOPIC;
const char mqttPublishMsg[] = "mchp payload";
static const char mqttSubscribeTopicsList[NUM_TOPICS_SUBSCRIBE][SUBSCRIBE_TOPIC_SIZE] = {
    {CFG_SUBTOPIC}};
static appStates_e appState = APP_STATE_INIT;
static uint32_t serverIPAddress = 0;
static uint8_t recvBuffer[256];
static bool timeReceived = false;
static bool appMQTTPublishTimeoutOccured = false;

char messageString[64];
static char json[PAYLOAD_SIZE]; //-added

#if defined(AES256)
    uint8_t key[] = { 0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe, 0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
                      0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7, 0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4 };
    uint8_t out[] = { 0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba, 0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6,
                      0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb, 0x80, 0x8d, 0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d,
                      0x39, 0xf2, 0x33, 0x69, 0xa9, 0xd9, 0xba, 0xcf, 0xa5, 0x30, 0xe2, 0x63, 0x04, 0x23, 0x14, 0x61,
                      0xb2, 0xeb, 0x05, 0xe2, 0xc3, 0x9b, 0xe9, 0xfc, 0xda, 0x6c, 0x19, 0x07, 0x8c, 0x6a, 0x9d, 0x1b };
#elif defined(AES192)
    uint8_t key[] = { 0x8e, 0x73, 0xb0, 0xf7, 0xda, 0x0e, 0x64, 0x52, 0xc8, 0x10, 0xf3, 0x2b, 0x80, 0x90, 0x79, 0xe5, 0x62, 0xf8, 0xea, 0xd2, 0x52, 0x2c, 0x6b, 0x7b };
    uint8_t out[] = { 0x4f, 0x02, 0x1d, 0xb2, 0x43, 0xbc, 0x63, 0x3d, 0x71, 0x78, 0x18, 0x3a, 0x9f, 0xa0, 0x71, 0xe8,
                      0xb4, 0xd9, 0xad, 0xa9, 0xad, 0x7d, 0xed, 0xf4, 0xe5, 0xe7, 0x38, 0x76, 0x3f, 0x69, 0x14, 0x5a,
                      0x57, 0x1b, 0x24, 0x20, 0x12, 0xfb, 0x7a, 0xe0, 0x7f, 0xa9, 0xba, 0xac, 0x3d, 0xf1, 0x02, 0xe0,
                      0x08, 0xb0, 0xe2, 0x79, 0x88, 0x59, 0x88, 0x81, 0xd9, 0x20, 0xa9, 0xe6, 0x4f, 0x56, 0x15, 0xcd };
#elif defined(AES128)
    uint8_t key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
    uint8_t out[] = { 0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46, 0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
                      0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee, 0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
                      0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b, 0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16,
                      0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09, 0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7 };
#endif
    uint8_t iv[]  = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
    uint8_t in[]  = { 0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96, 0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
                      0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c, 0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
                      0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11, 0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
                      0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17, 0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10 };
struct AES_ctx ctx;

static void app_buildMQTTConnectPacket(void);
static void app_buildPublishPacket(void);
static uint32_t appCheckMQTTPublishTimeout();
timerStruct_t appMQTTPublishTimer = {appCheckMQTTPublishTimeout, NULL};

static uint32_t appCheckMQTTPublishTimeout() {
    appMQTTPublishTimeoutOccured = true; // Mark that timer has executed
    return ((CFG_MQTT_CONN_TIMEOUT - 1) * SECONDS);
}

static void app_buildMQTTConnectPacket(void) {
    mqttConnectPacket appConnectPacket;

    memset(&appConnectPacket, 0, sizeof (mqttConnectPacket));

    appConnectPacket.connectVariableHeader.connectFlagsByte.All = 0x02;
    // Packets need to be sent to the server every 10s.
    appConnectPacket.connectVariableHeader.keepAliveTimer = CFG_MQTT_CONN_TIMEOUT;
    appConnectPacket.clientID = "jamesIoT";

    MQTT_CreateConnectPacket(&appConnectPacket);
}

static void app_buildPublishPacket(void) {
     
    //    appPublishPacket.payload = mqttPublishMsg;
    //message = sprintf(json,"{\"light\":%d,\"temp\":%.2f}", rawLight,rawTemp);
//    int     rawLightValue = 1023;
//    int     rawTemperatureValue = 15.21;
    int     message = 0;

    int rawLightValue = getLightSensorValue();
    int rawTemperatureValue = getTempSensorValue();

    mqttPublishPacket appPublishPacket;
    
    memset(&appPublishPacket, 0, sizeof (mqttPublishPacket));

    appPublishPacket.topic = mqttPublishTopic;

    message = sprintf(json, "%d,%d.%02d", rawLightValue, rawTemperatureValue/100
                     , abs(rawTemperatureValue)%100);
    
    printf("%d,%d.%02d\r\n", rawLightValue, rawTemperatureValue/100
                     , abs(rawTemperatureValue)%100);
    int msglen = strlen(json);
    
    //Add Padding
    int msglenu = msglen;
    
    if(msglenu % 16){
        msglenu += 16 - (msglen %16);
    }
 
    uint8_t msgarray[msglenu];
    
    memset(msgarray, 0, msglenu);
    
    for (int i=0; i<msglen; i++){
        msgarray[i] = (uint8_t)json[i];
    }
    
    int messagePad = pkcs7_padding_pad_buffer(msgarray, msglen, sizeof(msgarray), 16);
    
//    for (int i = strlen(msgarray) ; i < msglenu; i++){
//        msgarray[i] = (uint8_t)messagePad;
//    }
//    
//    for( int i = 0; i < msglenu; i++ ){
//        printf("%c", msgarray[i]);
//    }
//    printf("\r\n");
//    printf("%d \r\n", messagePad);

//    int valid = pkcs7_padding_valid( msgarray, msglen, sizeof(msgarray), 16 );
//    printf("Is it valid %d \n", valid);
//    printf("\n");
    
    /*
     * Encryption part starts here. 
     * Set the cipher text, secret key & initialization vector 
     * Output: encrypted msgarray
     */
    
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, msgarray, msglenu);
    
//    for( int i = 0; i < msglenu; i++ )
//    {
//        printf("%c", msgarray[i]);
//    }
//    printf("\r\n");
    
    /*
     * To check if after decryption you get the same original clear text values
     */
    
//    AES_ctx_set_iv(&ctx, iv);
//    AES_CBC_decrypt_buffer(&ctx, msgarray, msglenu);
    
//    for( int i = 0; i < msglenu; i++ )
//    {
//        printf("%c", msgarray[i]);
//    }
//    printf("\r\n");

//    printf("%s\r\n", base64_encode(messageString));
    
    /*
     * Finally use base64 encoding to avoid data loss via transportation medium
     * (especially special characters)
     */

//    printf("%s \n", base64_encode((char*)msgarray));
    
    
    appPublishPacket.payload = base64_encode((char*)msgarray);

    // Fixed header
    appPublishPacket.publishHeaderFlags.duplicate = 0;
    appPublishPacket.publishHeaderFlags.qos = CFG_QOS;
    appPublishPacket.publishHeaderFlags.retain = 0;

    // Variable header
    appPublishPacket.packetIdentifierLSB = 10;
    appPublishPacket.packetIdentifierMSB = 0;

    appPublishPacket.payloadLength = strlen(appPublishPacket.payload);

    MQTT_CreatePublishPacket(&appPublishPacket);
}



static void changeState(appStates_e newState) {
    if (newState != appState) {
        appState = newState;
    }
}

static void wifiHandler(uint8_t u8MsgType, void *pvMsg) {
    tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *) pvMsg;

    switch (u8MsgType) {
        case M2M_WIFI_RESP_CON_STATE_CHANGED:
            if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
                // Add your custom indication for successful AP connection
                printf("Wi-Fi connected\r\n");
            }
            break;

        case M2M_WIFI_REQ_DHCP_CONF:
            if (appState == APP_STATE_STA_CONNECTING) {
                changeState(APP_STATE_STA_CONNECTED);
                uint8_t *pu8IPAddress = (uint8_t *) pvMsg;
                printf("Wi-Fi IP is %u.%u.%u.%u\r\n",
                        pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
                break;
            }
            break;

        case M2M_WIFI_RESP_GET_SYS_TIME:
            timeReceived = true;
            break;

        default:
            break;
    }
}

static void icmpReplyHandler(uint32_t u32IPAddr, uint32_t u32RTT, uint8_t u8ErrorCode) {
    // Add your custom ICMP data handler
}

static void dnsHandler(uint8_t *pu8DomainName, uint32_t u32ServerIP) {
    if (u32ServerIP) {
        if (appState == APP_STATE_WAIT_FOR_DNS) {
            changeState(APP_STATE_TLS_START);
            m2m_ping_req(u32ServerIP, 0, icmpReplyHandler);
        }
    }

    serverIPAddress = u32ServerIP;
}

static void socketHandler(SOCKET sock, uint8_t u8Msg, void *pvMsg) {
    switch (u8Msg) {
        case SOCKET_MSG_CONNECT:
        {
            mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
            tstrSocketConnectMsg *pstrConnect = (tstrSocketConnectMsg *) pvMsg;

            if (pstrConnect && pstrConnect->s8Error >= 0) {
                changeState(APP_STATE_TLS_CONNECTED);

                recv(sock, recvBuffer, sizeof (recvBuffer), 0);
            } else {
                changeState(APP_STATE_ERROR);

                *mqttConnnectionInfo->tcpClientSocket = -1;
                close(sock);
            }

            break;
        }

        case SOCKET_MSG_RECV:
        {
            mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();
            tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *) pvMsg;

            if (pstrRecv && pstrRecv->s16BufferSize > 0) {
                MQTT_GetReceivedData(pstrRecv->pu8Buffer, pstrRecv->s16BufferSize);
            } else {
                changeState(APP_STATE_ERROR);

                *mqttConnnectionInfo->tcpClientSocket = -1;
                close(sock);
            }
            break;
        }

        case SOCKET_MSG_SEND:
        {
            break;
        }
    }
}

void app_mqttInit(void) {
    tstrWifiInitParam param;

    winc_adapter_init();

    param.pfAppWifiCb = wifiHandler;
    if (M2M_SUCCESS != m2m_wifi_init(&param)) {
        // Add your custom error handler
    }

    MQTT_ClientInitialise();
    app_buildMQTTConnectPacket();
    timeout_create(&appMQTTPublishTimer, ((CFG_MQTT_CONN_TIMEOUT - 1) * SECONDS));

    m2m_wifi_set_sleep_mode(M2M_PS_DEEP_AUTOMATIC, 1);
}

void app_mqttScheduler(void) {
    mqttContext* mqttConnnectionInfo = MQTT_GetClientConnectionInfo();

    timeout_callNextCallback();

    switch (appState) {
        case APP_STATE_INIT:
        {
            tstrNetworkId strNetworkId;
            tstrAuthPsk strAuthPsk;

            strNetworkId.pu8Bssid = NULL;
            strNetworkId.pu8Ssid = (uint8_t*) CONN_SSID;
            strNetworkId.u8SsidLen = strlen(CONN_SSID);
            strNetworkId.enuChannel = M2M_WIFI_CH_ALL;

            strAuthPsk.pu8Psk = NULL;
            strAuthPsk.pu8Passphrase = (uint8_t*) CONN_PASSWORD;
            strAuthPsk.u8PassphraseLen = strlen((const char*) CONN_PASSWORD);


            if (M2M_SUCCESS != m2m_wifi_connect((char *) CONN_SSID, sizeof (CONN_SSID), CONN_AUTH, (void *) CONN_PASSWORD, M2M_WIFI_CH_ALL)) {
                changeState(APP_STATE_ERROR);
                break;
            }

            changeState(APP_STATE_STA_CONNECTING);
            break;
        }

        case APP_STATE_STA_CONNECTING:
        {
            break;
        }

        case APP_STATE_STA_CONNECTED:
        {
            socketInit();
            registerSocketCallback(socketHandler, dnsHandler);
            changeState(APP_STATE_TLS_START);

            break;
        }

        case APP_STATE_WAIT_FOR_DNS:
        {
            break;
        }

        case APP_STATE_TLS_START:
        {
            struct sockaddr_in addr;

            if (*mqttConnnectionInfo->tcpClientSocket != -1) {
                close(*mqttConnnectionInfo->tcpClientSocket);
            }

            if (*mqttConnnectionInfo->tcpClientSocket = socket(AF_INET, SOCK_STREAM, 0)) {
                changeState(APP_STATE_ERROR);
                break;
            }
            addr.sin_family = AF_INET;
            addr.sin_port = _htons(CFG_MQTT_PORT);
            addr.sin_addr.s_addr = _htonl(CFG_MQTT_SERVERIPv4_HEX);

            if (connect(*mqttConnnectionInfo->tcpClientSocket, (struct sockaddr *) &addr, sizeof (struct sockaddr_in)) < 0) {
                close(*mqttConnnectionInfo->tcpClientSocket);
                *mqttConnnectionInfo->tcpClientSocket = -1;
                changeState(APP_STATE_ERROR);

                break;
            }
            changeState(APP_STATE_TLS_CONNECTING);

            break;
        }

        case APP_STATE_TLS_CONNECTING:
        {
            break;
        }

        case APP_STATE_TLS_CONNECTED:
        {
            if (appMQTTPublishTimeoutOccured == true) {
                appMQTTPublishTimeoutOccured = false;
                app_buildPublishPacket();
//                DELAY_milliseconds(2000);
                
            }

            MQTT_ReceptionHandler(mqttConnnectionInfo);
            MQTT_TransmissionHandler(mqttConnnectionInfo);
        }
            break;

        case APP_STATE_ERROR:
        {
            m2m_wifi_deinit(NULL);
            timeout_delete(&appMQTTPublishTimer);
            changeState(APP_STATE_STOPPED);

            break;
        }

        case APP_STATE_STOPPED:
        {
            changeState(APP_STATE_INIT);
            break;
        }
    }

    m2m_wifi_handle_events(NULL);
}

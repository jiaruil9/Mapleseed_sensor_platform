/*
 * Copyright (c) 2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/***** Includes *****/
/* Standard C Libraries */
#include <stdlib.h>

/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/UART.h>
/* Driverlib Header files */
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)

/* Board Header files */
#include "Board.h"

/* Application Header files */
#include "RFQueue.h"
#include "smartrf_settings/smartrf_settings.h"

/***** Defines *****/

/* Packet RX Configuration */
#define DATA_ENTRY_HEADER_SIZE 8  /* Constant header size of a Generic Data Entry */
#define MAX_LENGTH             102 /* Max length byte the radio will accept */
#define NUM_DATA_ENTRIES       2  /* NOTE: Only two data entries supported at the moment */
#define NUM_APPENDED_BYTES     2  /* The Data Entries data field will contain:
                                   * 1 Header byte (RF_cmdPropRx.rxConf.bIncludeHdr = 0x1)
                                   * Max 30 payload bytes
                                   * 1 status byte (RF_cmdPropRx.rxConf.bAppendStatus = 0x1) */



/***** Prototypes *****/
static void callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);

/***** Variable declarations *****/
static RF_Object rfObject;
static RF_Handle rfHandle;

/* Pin driver handle */
static PIN_Handle ledPinHandle;
static PIN_State ledPinState;

UART_Handle uart;
UART_Params uartParams;
const char  newline[] = "\r\n";

/* Buffer which contains all Data Entries for receiving data.
 * Pragmas are needed to make sure this buffer is 4 byte aligned (requirement from the RF Core) */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN (rxDataEntryBuffer, 4);
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  MAX_LENGTH,
                                                  NUM_APPENDED_BYTES)];
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment = 4
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  MAX_LENGTH,
                                                  NUM_APPENDED_BYTES)];
#elif defined(__GNUC__)
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  MAX_LENGTH,
                                                  NUM_APPENDED_BYTES)]
                                                  __attribute__((aligned(4)));
#else
#error This compiler is not supported.
#endif

/* Receive dataQueue for RF Core to fill in data */
static dataQueue_t dataQueue;
static rfc_dataEntryGeneral_t* currentDataEntry;
static uint8_t packetLength;
static uint8_t* packetDataPointer;


static uint8_t packet[MAX_LENGTH + NUM_APPENDED_BYTES - 1]; /* The length byte is stored in a separate variable */

/*
 * Application LED pin configuration table:
 *   - All LEDs board LEDs are off.
 */
PIN_Config pinTable[] =
{
    Board_PIN_LED2 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
	PIN_TERMINATE
};

struct gps_msg_full_t
{
    //35 bytes
    uint8_t time[9];
    uint8_t latitude[8];
    uint8_t N_S[1];
    uint8_t longitude[9];
    uint8_t E_W[1];
    uint8_t quality_indicator;
    uint8_t sat_num[2];
    uint8_t horizontal_resolution[2];
    uint8_t altitude[4];
};              //GPS message struct


struct gps_msg_full_t gps_full_msgs[2];
static void GPS_GGA_parse_full(uint8_t * msg, char idx)
{
    // Fill in this struct
    /*
    struct gps_msg_full_t
    {
        //27 bytes
        char time[9];
        char latitude[8];
        char N_S[1];
        char longitude[9];
        char E_W[1];
        char horizontal_resolution[2];
        char altitude[2];
    };
    */
    gps_full_msgs[idx].time[0] = msg[7];
    gps_full_msgs[idx].time[1] = msg[8];
    gps_full_msgs[idx].time[2] = msg[9];
    gps_full_msgs[idx].time[3] = msg[10];
    gps_full_msgs[idx].time[4] = msg[11];
    gps_full_msgs[idx].time[5] = msg[12];
    //skip period
    gps_full_msgs[idx].time[6] = msg[14];
    gps_full_msgs[idx].time[7] = msg[15];
    gps_full_msgs[idx].time[8] = msg[16];
    //skip comma
    gps_full_msgs[idx].latitude[0] = msg[18];
    gps_full_msgs[idx].latitude[1] = msg[19];
    gps_full_msgs[idx].latitude[2] = msg[20];
    gps_full_msgs[idx].latitude[3] = msg[21];
    //skip period
    gps_full_msgs[idx].latitude[4] = msg[23];
    gps_full_msgs[idx].latitude[5] = msg[24];
    gps_full_msgs[idx].latitude[6] = msg[25];
    gps_full_msgs[idx].latitude[7] = msg[26];
    //skip comma is 27
    gps_full_msgs[idx].N_S[0] = msg[28];

    //skip comma is 29
    gps_full_msgs[idx].longitude[0] = msg[30];
    gps_full_msgs[idx].longitude[1] = msg[31];
    gps_full_msgs[idx].longitude[2] = msg[32];
    gps_full_msgs[idx].longitude[3] = msg[33];
    gps_full_msgs[idx].longitude[4] = msg[34];
    // skip period is 35
    gps_full_msgs[idx].longitude[5] = msg[36];
    gps_full_msgs[idx].longitude[6] = msg[37];
    gps_full_msgs[idx].longitude[7] = msg[38];
    gps_full_msgs[idx].longitude[8] = msg[39];
    //skip comma is 40
    gps_full_msgs[idx].E_W[0] = msg[41];
    //skip comma
    gps_full_msgs[idx].quality_indicator = msg[43];
    //skip comma
    gps_full_msgs[idx].sat_num[0] = msg[45];
    gps_full_msgs[idx].sat_num[1] = msg[46];
    //skip comma
    gps_full_msgs[idx].horizontal_resolution[0] = msg[48];
    //skip period
    gps_full_msgs[idx].horizontal_resolution[1] = msg[50];
    //skip comma
    gps_full_msgs[idx].altitude[0] = msg[52];
    gps_full_msgs[idx].altitude[1] = msg[53];
    gps_full_msgs[idx].altitude[2] = msg[54];
    //skip comma
    gps_full_msgs[idx].altitude[3] = msg[56];

}
/***** Function definitions *****/

void *mainThread(void *arg0)
{
    RF_Params rfParams;
    RF_Params_init(&rfParams);

    /* UART */



    UART_init();
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_BINARY;
    uartParams.readDataMode = UART_DATA_BINARY;
    uartParams.readReturnMode = UART_RETURN_FULL;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.baudRate = 4800;

    uart = UART_open(Board_UART0, &uartParams);

    if (uart == NULL) {
        /* UART_open() failed */
        while (1);
    }

    /* Open LED pins */
    ledPinHandle = PIN_open(&ledPinState, pinTable);
    if (ledPinHandle == NULL)
    {
        while(1);
    }

    if( RFQueue_defineQueue(&dataQueue,
                            rxDataEntryBuffer,
                            sizeof(rxDataEntryBuffer),
                            NUM_DATA_ENTRIES,
                            MAX_LENGTH + NUM_APPENDED_BYTES))
    {
        /* Failed to allocate space for all data entries */
        while(1);
    }

    /* Modify CMD_PROP_RX command for application needs */
    /* Set the Data Entity queue for received data */
    RF_cmdPropRx.pQueue = &dataQueue;
    /* Discard ignored packets from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushIgnored = 1;
    /* Discard packets with CRC error from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushCrcErr = 1;
    /* Implement packet length filtering to avoid PROP_ERROR_RXBUF */
    RF_cmdPropRx.maxPktLen = MAX_LENGTH;
    RF_cmdPropRx.pktConf.bRepeatOk = 1;
    RF_cmdPropRx.pktConf.bRepeatNok = 1;

    /* Request access to the radio */
#if defined(DeviceFamily_CC26X0R2)
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioSetup, &rfParams);
#else
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &rfParams);
#endif// DeviceFamily_CC26X0R2

    /* Set the frequency */
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);

    /* Enter RX mode and stay forever in RX */
    RF_EventMask terminationReason = RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropRx,
                                               RF_PriorityNormal, &callback,
                                               RF_EventRxEntryDone);

    switch(terminationReason)
    {
        case RF_EventLastCmdDone:
            // A stand-alone radio operation command or the last radio
            // operation command in a chain finished.
            break;
        case RF_EventCmdCancelled:
            // Command cancelled before it was started; it can be caused
            // by RF_cancelCmd() or RF_flushCmd().
            break;
        case RF_EventCmdAborted:
            // Abrupt command termination caused by RF_cancelCmd() or
            // RF_flushCmd().
            break;
        case RF_EventCmdStopped:
            // Graceful command termination caused by RF_cancelCmd() or
            // RF_flushCmd().
            break;
        default:
            // Uncaught error event
            while(1);
    }

    uint32_t cmdStatus = ((volatile RF_Op*)&RF_cmdPropRx)->status;
    switch(cmdStatus)
    {
        case PROP_DONE_OK:
            // Packet received with CRC OK
            break;
        case PROP_DONE_RXERR:
            // Packet received with CRC error
            break;
        case PROP_DONE_RXTIMEOUT:
            // Observed end trigger while in sync search
            break;
        case PROP_DONE_BREAK:
            // Observed end trigger while receiving packet when the command is
            // configured with endType set to 1
            break;
        case PROP_DONE_ENDED:
            // Received packet after having observed the end trigger; if the
            // command is configured with endType set to 0, the end trigger
            // will not terminate an ongoing reception
            break;
        case PROP_DONE_STOPPED:
            // received CMD_STOP after command started and, if sync found,
            // packet is received
            break;
        case PROP_DONE_ABORT:
            // Received CMD_ABORT after command started
            break;
        case PROP_ERROR_RXBUF:
            // No RX buffer large enough for the received data available at
            // the start of a packet
            break;
        case PROP_ERROR_RXFULL:
            // Out of RX buffer space during reception in a partial read
            break;
        case PROP_ERROR_PAR:
            // Observed illegal parameter
            break;
        case PROP_ERROR_NO_SETUP:
            // Command sent without setting up the radio in a supported
            // mode using CMD_PROP_RADIO_SETUP or CMD_RADIO_SETUP
            break;
        case PROP_ERROR_NO_FS:
            // Command sent without the synthesizer being programmed
            break;
        case PROP_ERROR_RXOVF:
            // RX overflow observed during operation
            break;
        default:
            // Uncaught error event - these could come from the
            // pool of states defined in rf_mailbox.h
            while(1);
    }

    while(1);
}

void callback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    const char  time[] = "time:\r\n";
    const char  latitude[] = "latitude:\r\n";
    const char  longitude[] = "longitude:\r\n";
    const char  newline[] = "\r\n";
    const char  period[] = ".";
    const char  degree[] = "deg ";
    const char  min[] = "'";
    if (e & RF_EventRxEntryDone)
    {
        /* Toggle pin to indicate RX */
        PIN_setOutputValue(ledPinHandle, Board_PIN_LED2,
                           !PIN_getOutputValue(Board_PIN_LED2));

        /* Get current unhandled data entry */
        currentDataEntry = RFQueue_getDataEntry();

        /* Handle the packet data, located at &currentDataEntry->data:
         * - Length is the first byte with the current configuration
         * - Data starts from the second byte */
        packetLength      = *(uint8_t*)(&currentDataEntry->data);
        packetDataPointer = (uint8_t*)(&currentDataEntry->data + 1);

        /* Copy the payload + the status byte to the packet variable */
        memcpy(packet, packetDataPointer, (packetLength + 1));
//        GPS_GGA_parse_full(packet+3, 1);
//        //print out the time, latitude, longitude
//        UART_write(uart, time,sizeof(time));
//        UART_write(uart, gps_full_msgs[1].time,6);
//        UART_write(uart, period, 1);
//        UART_write(uart, gps_full_msgs[1].time+5,3);
//        UART_write(uart, newline,sizeof(newline));
//        UART_write(uart, latitude,sizeof(latitude));
//        UART_write(uart, gps_full_msgs[1].latitude,2);
//        UART_write(uart, degree, sizeof(degree));
//        UART_write(uart, gps_full_msgs[1].latitude+2, 2);
//        UART_write(uart, period, 1);
//        UART_write(uart, gps_full_msgs[1].latitude+4, 4);
//        UART_write(uart, min,1);
//        UART_write(uart, gps_full_msgs[1].N_S, 1);
//        UART_write(uart, newline,sizeof(newline));
//        UART_write(uart, longitude,sizeof(longitude));
//        if (gps_full_msgs[1].longitude[0] == '0'){
//            UART_write(uart, gps_full_msgs[1].longitude+1,2);
//        }else{
//            UART_write(uart, gps_full_msgs[1].longitude,3);
//        }
//        UART_write(uart, degree, sizeof(degree));
//        UART_write(uart, gps_full_msgs[1].longitude+3,2);
//        UART_write(uart, period, 1);
//        UART_write(uart, gps_full_msgs[1].longitude+5,3);
//        UART_write(uart, min,1);
//        UART_write(uart, gps_full_msgs[1].E_W, 1);
//        UART_write(uart, newline,sizeof(newline));
        UART_write(uart, packet+3,sizeof(packet)-3);
        UART_write(uart, newline,sizeof(newline));
        GPS_GGA_parse_full(packet+3, 1);
        UART_write(uart, gps_full_msgs[1].time,6);
        UART_write(uart, newline,sizeof(newline));
        RFQueue_nextEntry();
    }
}

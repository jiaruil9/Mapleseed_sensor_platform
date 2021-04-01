/*
 * Copyright (c) 2015-2019, Texas Instruments Incorporated
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

/*
 *  ======== uartecho.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/UART.h>

/* Example/Board Header files */
#include "Board.h"

struct gps_msg_full_t
{
    //35 bytes
    char time[9];
    char latitude[8];
    char N_S[1];
    char longitude[9];
    char E_W[1];
    char quality_indicator;
    char sat_num[2];
    char horizontal_resolution[2];
    char altitude[4];
};              //GPS message struct


struct gps_msg_full_t gps_full_msgs[3];
static void GPS_GGA_parse_full(char * msg, char idx)
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

/********************** GPS Sensor Connection **********************/
/********* GND - GND, 3V3 - 3V3, TX - DIO2, PWR - DIO12  *********/

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    char        input;
    const char  echoPrompt[] = "Echoing characters:\r\n";
    const char  time[] = "time:\r\n";
    const char  latitude[] = "latitude:\r\n";
    const char  longitude[] = "longitude:\r\n";
    const char  newline[] = "\r\n";
    const char  period[] = ".";
    const char  degree[] = "deg ";
    const char  min[] = "'";
    UART_Handle uart;
    UART_Params uartParams;
    char message[100];
    char count = 0;
    /* Call driver init functions */
    GPIO_init();
    UART_init();

    /* Configure the LED pin */
    GPIO_setConfig(Board_GPIO_LED0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    IOCPinTypeGpioOutput( IOID_12 );   //initialize IOID_12 as PWR input for GPS Nano
    /* Turn on user LED */
    GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);
    /* Create a UART with data processing off. */
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

    GPIO_setDio (IOID_12);
    cc1310_usleep(100,0);
    GPIO_clearDio(IOID_12);   // a pulse to wake up the GPS Nano
    /* Loop forever echoing */

    while (1) {


        UART_read(uart, &input, 1);
        message[count] = input;
        ++count;
        if (input == '\n')
        {
             // Once we finish a line, check if msg is a GPGGA
            if (message[3] == 'G' && message[4] == 'G' && message[5] == 'A')
            {
                 //we have the message we need
                 GPS_GGA_parse_full(message, 1);
                 //print out the time, latitude, longitude
                 UART_write(uart, time,sizeof(time));
                 UART_write(uart, gps_full_msgs[1].time,6);
                 UART_write(uart, period, 1);
                 UART_write(uart, gps_full_msgs[1].time+5,3);
                 UART_write(uart, newline,sizeof(newline));
                 UART_write(uart, latitude,sizeof(latitude));
                 UART_write(uart, gps_full_msgs[1].latitude,2);
                 UART_write(uart, degree, sizeof(degree));
                 UART_write(uart, gps_full_msgs[1].latitude+2, 2);
                 UART_write(uart, period, 1);
                 UART_write(uart, gps_full_msgs[1].latitude+4, 4);
                 UART_write(uart, min,1);
                 UART_write(uart, gps_full_msgs[1].N_S, 1);
                 UART_write(uart, newline,sizeof(newline));
                 UART_write(uart, longitude,sizeof(longitude));
                 if (gps_full_msgs[1].longitude[0] == '0'){
                     UART_write(uart, gps_full_msgs[1].longitude+1,2);
                 }else{
                     UART_write(uart, gps_full_msgs[1].longitude,3);
                 }
                 UART_write(uart, degree, sizeof(degree));
                 UART_write(uart, gps_full_msgs[1].longitude+3,2);
                 UART_write(uart, period, 1);
                 UART_write(uart, gps_full_msgs[1].longitude+5,3);
                 UART_write(uart, min,1);
                 UART_write(uart, gps_full_msgs[1].E_W, 1);
                 UART_write(uart, newline,sizeof(newline));
            }
        count = 0;
       } // if end of message
//        UART_write(uart, &input, 1);

    }
}

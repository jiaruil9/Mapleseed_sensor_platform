/*
 * Copyright (c) 2016-2019, Texas Instruments Incorporated
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
 *  ======== accel_task.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* POSIX Header files */
#include <pthread.h>

/* Driver Header files */
#include <ti/drivers/ADC.h>
#include <ti/display/Display.h>

/* Example/Board Header files */
#include "Board.h"

/* ADC sample count */
#define ADC_SAMPLE_COUNT  (10)

#define THREADSTACKSIZE   (768)

/* ADC conversion result variables */

struct accel_msg_t{
  float accelX;
  float accelY;
  float accelZ;
};

float scale = 200.0; // 3 (±3g) for ADXL337, 200 (±200g) for ADXL377
uint16_t accelX;
uint16_t accelY;
uint16_t accelZ;

// Store single data point
struct accel_msg_t accel_data;
// If we want to store msg history
//struct accel_msg_t accel_msgs[3];

// Same functionality as Arduino's standard map function, except using floats
float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*
 *  ======== accelDataThread ========
 *  Open a ADC handles to XYZ channels and create an array of 
 *  accelerometer readings after calling several conversions.
 */
void *accelDataThread(void *arg0)
{
    uint16_t     i;
    ADC_Handle   adcAccelX;
    ADC_Handle   adcAccelY;
    ADC_Handle   adcAccelZ;
    ADC_Params   params;
    int_fast16_t rawX, rawY, rawZ;

    ADC_Params_init(&params);
    adcAccelX = ADC_open(Board_ADC0, &params);
    adcAccelY = ADC_open(Board_ADC1, &params);
    //adcAccelZ = ADC_open(Board_ADC2, &params);
    

    if (adcAccelX == NULL || adcAccelY == NULL) {
        printf("Error initializing ADC Channels\n");
        while (1);
    }
    
    while(1){
        rawX = ADC_convert(adcAccelX, &accelX);
        rawY = ADC_convert(adcAccelY, &accelY);
        //rawZ = ADC_convert(adcAccelZ, &adcValueZ[i]);
        
        if(rawX == ADC_STATUS_SUCCESS){
            //Convert raw to scaled
            // Store in global struct to send to receiver
            accel_data.accelX = mapf(rawX, 0.0, 1023.0,( -1 * scale), scale);
            accel_data.accelY = mapf(rawY, 0.0, 1023.0, (-1 * scale), scale);
            //accel_data.accelZ =  mapf(float(rawZ), 0.0, 1023.0, -1 * scale, scale);
        }
        else{
            printf("ERROR READING RAW ANALOG VALUES");
        }
        
        // Sleep for specified time, don't need continuous data
    }
    ADC_close(adcAccelX);
    ADC_close(adcAccelY);
    //ADC_close(adcAccelZ);
    return (NULL);
}

/*
 *  ======== accelThread ========
 */
void *accelThread(void *arg0)
{
    pthread_t           thread0;
    pthread_attr_t      attrs;
    struct sched_param  priParam;
    int                 retc;
    int                 detachState;

    /* Call driver init functions */
    ADC_init();
    /* Create application threads */
    pthread_attr_init(&attrs);

    detachState = PTHREAD_CREATE_DETACHED;
    /* Set priority and stack size attributes */
    retc = pthread_attr_setdetachstate(&attrs, detachState);
    if (retc != 0) {
        /* pthread_attr_setdetachstate() failed */
        while (1);
    }

    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
    if (retc != 0) {
        /* pthread_attr_setstacksize() failed */
        while (1);
    }

    /* Create threadFxn0 thread */
    priParam.sched_priority = 1;
    pthread_attr_setschedparam(&attrs, &priParam);

    /* Create threadFxn1 thread */
    retc = pthread_create(&thread0, &attrs, accelDataThread, (void* )0);
    if (retc != 0) {
        /* pthread_create() failed */
        while (1);
    }

    return (NULL);
}

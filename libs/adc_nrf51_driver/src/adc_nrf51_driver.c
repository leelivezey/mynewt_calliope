/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <os/os.h>
#include "syscfg/syscfg.h"
#include "adc_nrf51_driver/adc_nrf51_driver.h"

/* Mynewt ADC driver */
#include <adc/adc.h>

/* Mynewt Nordic driver */
#include <adc_nrf51/adc_nrf51.h>

/* Nordic driver */
#include "nrf.h"
#include "app_util_platform.h"
#include "nrf_drv_adc.h"
#include "nrf_adc.h"

// could maybe use NRF_ADC_CHANNEL_COUNT   (1) but only defined in the adc_nrf51.c
#define ADC_NUMBER_CHANNELS (1)

#define ADC_NUMBER_SAMPLES MYNEWT_VAL(ADC_0_SAMPLES)

static uint8_t *sample_buffer1;

//I don't like that his adc holds a reference to adc and passes it out
//If you're passing it out, then pass it back into adc_read
//also you can look it up with os_dev_lookup
static struct adc_dev *adc_nrf51_driver_adc;

static struct adc_dev os_bsp_adc0;

static nrf_drv_adc_config_t os_bsp_adc0_config = {
    .interrupt_priority = MYNEWT_VAL(ADC_0_INTERRUPT_PRIORITY),
};
//dont understand why there are two configurations, one for create and one for open
//I guess maybe because often be in the bsp except the licensing issues
// nrf_drv_adc_config_t adc_config = NRF_DRV_ADC_DEFAULT_CONFIG;

// nrf_drv_adc_channel_t cc = NRF_DRV_ADC_DEFAULT_CHANNEL(MYNEWT_VAL(ADC_0_INPUT));
static nrf_drv_adc_channel_t cc = {{{
    .resolution           = MYNEWT_VAL(ADC_0_RESOLUTION),
    .input                = MYNEWT_VAL(ADC_0_SCALING),
    .reference            = MYNEWT_VAL(ADC_0_REFERENCE),
    .ain                  = MYNEWT_VAL(ADC_0_INPUT),
    .external_reference   = MYNEWT_VAL(ADC_0_EXTERNAL_REFERENCE),
}}, NULL};


void*
adc_nrf51_driver_get(void)
{
    return adc_nrf51_driver_adc;
}

void
adc_nrf51_driver_init(void)
{
    int rc;

    //this would probably be in the bsp except the licensing issues?
    rc = os_dev_create((struct os_dev *) &os_bsp_adc0, "adc0",
            OS_DEV_INIT_KERNEL, OS_DEV_INIT_PRIO_DEFAULT,
            nrf51_adc_dev_init, &os_bsp_adc0_config);
    assert(rc == 0);

    adc_nrf51_driver_adc = (struct adc_dev *) os_dev_open("adc0", 0, &os_bsp_adc0_config);
    assert(adc_nrf51_driver_adc != NULL);

    rc = adc_chan_config(adc_nrf51_driver_adc, 0, &cc);
    assert(rc == 0);

    //why not use calloc instead?
    sample_buffer1 = malloc(adc_buf_size(adc_nrf51_driver_adc, ADC_NUMBER_CHANNELS, ADC_NUMBER_SAMPLES));
    memset(sample_buffer1, 0, adc_buf_size(adc_nrf51_driver_adc, ADC_NUMBER_CHANNELS, ADC_NUMBER_SAMPLES));
    //nrf51 currently only supports one buffer
    adc_buf_set(adc_nrf51_driver_adc, sample_buffer1, NULL,
        adc_buf_size(adc_nrf51_driver_adc, ADC_NUMBER_CHANNELS, ADC_NUMBER_SAMPLES));
}


/*
 * calculate the average of ADC_NUMBER_SAMPLES in the received buffer
 */
int
adc_nrf51_driver_read(void *buffer, int buffer_len)
{
    int i;
    int adc_result;
    int sum_adc_result = 0;
    int my_result_mv = 0;
    int rc;
    for (i = 0; i < ADC_NUMBER_SAMPLES; i++) {
        rc = adc_buf_read(adc_nrf51_driver_adc, buffer, buffer_len, i, &adc_result);
        if (rc != 0) {
            goto err;
        }
        sum_adc_result += adc_result;
    }
    my_result_mv = adc_result_mv(adc_nrf51_driver_adc, 0, sum_adc_result/ADC_NUMBER_SAMPLES);
    adc_buf_release(adc_nrf51_driver_adc, buffer, buffer_len);
    return my_result_mv;
err:
    return (rc);
}

/*
 * PackageLicenseDeclared: Apache-2.0
 * Copyright (c) 2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include "mbed-drivers/mbed_assert.h"
#include "fsl_clock_manager.h"
#include "fsl_dspi_hal.h"

#include "ws2xxx-rgb-led/ws2xxx.h"
#include "color-correction.h"

#define RGB_DIMMING 2

WS2xxx::WS2xxx(PinName pin) :
    SPI(pin, NC, NC),
    m_alternating(false),
    m_width(1),
    m_height(1)
{
    init();
};

WS2xxx::WS2xxx(PinName pin, int width) :
    SPI(pin, NC, NC),
    m_alternating(false),
    m_width(width),
    m_height(1)
{
    init();
};

WS2xxx::WS2xxx(PinName pin, int width, int height) :
    SPI(pin, NC, NC),
    m_alternating(false),
    m_width(width),
    m_height(height)
{
    init();
};

WS2xxx::WS2xxx(PinName pin, int width, int height, bool alternating) :
    SPI(pin, NC, NC),
    m_alternating(alternating),
    m_width(width),
    m_height(height)
{
    init();
};

void WS2xxx::clear(void)
{
    /* clear output buffer */
    memset(m_buffer, 0, m_size*sizeof(m_buffer[0]));
}

void WS2xxx::init(void)
{
    MBED_ASSERT(COLOR_CORRECTION_RANGE == 0x100);

    if(m_width<1)
        m_width = 1;
    if(m_height<1)
        m_height = 1;

    /* allocate output buffer */
    m_size = m_width*m_height;
    m_buffer = new int [m_size];
    clear();

    /* update SPI cmd */
    memset(&m_cmd, 0, sizeof(m_cmd));
    m_cmd.isEndOfQueue = false;
    m_cmd.isChipSelectContinuous = true;
    /* used data CTAR by default */
    m_cmd.whichCtar = kDspiCtar0;

    /* configure SPI settings */
    format(16, 0, SPI_MSB);
    /* data  CTAR */
    frequency(12600000, kDspiCtar0);
    /* reset CTAR */
    frequency(300000, kDspiCtar1);
}

void WS2xxx::tx_raw(uint16_t value)
{
    /* wait for TX buffer */
    while(!DSPI_HAL_GetStatusFlag(_spi.spi.address, kDspiTxFifoFillRequest));
    DSPI_HAL_WriteDataMastermode(_spi.spi.address, &m_cmd, value);
    DSPI_HAL_ClearStatusFlag(_spi.spi.address, kDspiTxFifoFillRequest);
}

void WS2xxx::frequency(int hz, dspi_ctar_selection_t ctar)
{
    uint32_t busClock;

    /* update HZ only for CTAR0 */
    if(ctar == kDspiCtar0)
        _hz = hz;

    CLOCK_SYS_GetFreq(kBusClock, &busClock);
    DSPI_HAL_SetBaudRate(_spi.spi.address, ctar, (uint32_t)hz, busClock);
}

void WS2xxx::tx(uint32_t value)
{
    int i;

    /* transmit RGB sequence for one WD2812B pixel */
    for(i=0; i<24; i++)
    {
        tx_raw( (value & 0x800000) ? 0xFF80 : 0xF800 );
        value <<= 1;
    }
}

void WS2xxx::tx_reset(void)
{
    /* send reset CTAR */
    m_cmd.whichCtar = kDspiCtar1;
    tx_raw(0x00);
    /* switch back to data CTAR */
    m_cmd.whichCtar = kDspiCtar0;
}

void WS2xxx::send(void)
{
    int length, *pixel;

    /* disable interrupts during update */
    __disable_irq();

    /* force RGB array reset */
    tx_reset();

    /* transmit pixel buffer */
    pixel = m_buffer;
    length = m_size;
    while(length--)
        tx(*pixel++);

    /* force RGB array update */
    tx_reset();

    /* re-enable interrupts */
    __enable_irq();
}

void WS2xxx::set(int x, int rgb)
{
    int y;
    int rgb_corrected;

    /* check range */
    if((x<0) || (x>m_size))
        return;

    rgb_corrected =
        ((uint32_t)g_cie_correction[((rgb>>16) & 0xFF)/RGB_DIMMING]) <<  8 |
        ((uint32_t)g_cie_correction[((rgb>> 8) & 0xFF)/RGB_DIMMING]) << 16 |
        ((uint32_t)g_cie_correction[((rgb>> 0) & 0xFF)/RGB_DIMMING]) <<  0;

    /* handle RGB arrays with alternating directions on every line */
    if(!m_alternating)
        m_buffer[x] = rgb_corrected;
    else
    {
        /* calculate x/y position */
        y = x / m_width;
        x = x % m_width;

        if(y&1)
            x = m_width - 1 - x;

        m_buffer[y*m_width + x] = rgb_corrected;
    }
}

void WS2xxx::set(int x, int y, int rgb)
{
    /* check x/y range */
    if( (y<0) || (y>m_height) || (x<0) || (x>m_width) )
        return;

    /* perform linear write */
    set(y * m_width + x, rgb);
}

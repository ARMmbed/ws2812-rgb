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
 
#ifndef __WS2xxx__
#define __WS2xxx__

#include "mbed-drivers/SPI.h"
#include "fsl_clock_manager.h"
#include "fsl_dspi_hal.h"

class WS2xxx : protected mbed::SPI {
    protected:
        bool m_alternating;
        int m_width, m_height, m_size;
        int *m_buffer;
        dspi_command_config_t m_cmd;

        void init(void);
        void tx(uint32_t value);
        void tx_raw(uint16_t value);
        void tx_reset(void);
        void frequency(int hz, dspi_ctar_selection_t ctar);

    public:
        WS2xxx(PinName pin);
        WS2xxx(PinName pin, int width);
        WS2xxx(PinName pin, int width, int height);
        WS2xxx(PinName pin, int width, int height, bool alternating);

        void send(void);
        void clear(void);
        void set(int x, int rgb);
        void set(int x, int y, int rgb);
};

#endif/*__WS2xxx__*/

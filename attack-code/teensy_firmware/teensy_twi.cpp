// Copyright (C) 2021 Niklas Jacob
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/*
  [1]:  i.MX RT1060 Processor ReferenceManual
        https://www.pjrc.com/teensy/IMXRT1060RM_rev2.pdf
*/

#include "teensy_twi.hpp"

namespace Teensy {
namespace Twi {

bool Hardware::base_clock_initialized = false;

// Pin configurations working on the teensy
// (see [1] pages 427-782 and 829-835)
// (see https://www.pjrc.com/store/teensy40.html#tech)
const Hardware Hardware::Pins_19_18 = {
    .regs = &IMXRT_LPI2C1, 
    .scl = { Pad19, &IOMUXC_LPI2C1_SCL_SELECT_INPUT },
    .sda = { Pad18, &IOMUXC_LPI2C1_SDA_SELECT_INPUT },
    .pin_cfg = ModuleInput::Config().Input().Mux(3).Daisy(1),
};

const Hardware Hardware::Pins_37_36 = {
    .regs = &IMXRT_LPI2C3,
    .scl = { Pad37, &IOMUXC_LPI2C3_SCL_SELECT_INPUT },
    .sda = { Pad36, &IOMUXC_LPI2C3_SDA_SELECT_INPUT },
    .pin_cfg = ModuleInput::Config().Input().Mux(2).Daisy(1),
};

const Hardware Hardware::Pins_16_17 = {
    .regs = &IMXRT_LPI2C3,
    .scl = { Pad16, &IOMUXC_LPI2C3_SCL_SELECT_INPUT },
    .sda = { Pad17, &IOMUXC_LPI2C3_SDA_SELECT_INPUT },
    .pin_cfg = ModuleInput::Config().Input().Mux(1).Daisy(2),
};

const Hardware Hardware::Pins_24_25 = {
    .regs = &IMXRT_LPI2C4,
    .scl = { Pad24, &IOMUXC_LPI2C4_SCL_SELECT_INPUT },
    .sda = { Pad25, &IOMUXC_LPI2C4_SDA_SELECT_INPUT },
    .pin_cfg = ModuleInput::Config().Input().Mux(0).Daisy(1),
};

// Returns zero on success, negative error codes on error
int Master::send_u16(uint8_t address, uint16_t message, uint32_t timeout) {

    // sanity check address
    if ((address >> 7) != 0) return -2;

    // wait while the master is busy
    while (regs().MSR & LPI2C_MSR_MBF)
        if (timeout-- == 0) return -1;

    // are there any errors?
    if (regs().MSR & ~(LPI2C_MSR_SDF | LPI2C_MSR_EPF | LPI2C_MSR_TDF))
        return (1<<31) | regs().MSR;

    // transmit buffer should be empty
    // (see [1] pages 2777- 2778)
    if ((regs().MFSR & 0x7) != 0)
        return (1<<31) | (1<<30) | (regs().MFSR & 0xf);

    // Add commands for the LPI2C to the FIFO buffer

    // Generate a start condition and transmit address
    // Note: the data are the address together with the LSB = 0,
    //       which indicates that we want to transmit
    regs().MTDR = LPI2C_MTDR_CMD_START | LPI2C_MTDR_DATA(address << 1);
    // send regular data
    regs().MTDR = LPI2C_MTDR_CMD_TRANSMIT | LPI2C_MTDR_DATA(message);
    regs().MTDR = LPI2C_MTDR_CMD_TRANSMIT | LPI2C_MTDR_DATA(message >> 8);
    // send stop condition
    regs().MTDR = LPI2C_MTDR_CMD_STOP;

    while (
            // fifos not empty
            (regs().MFSR & 0x7) != 0
        ||
            // master is busy
            (regs().MSR & LPI2C_MSR_MBF) != 0
    ) {
        // return the culprit on timeout
        if (timeout-- == 0) {
            if ((regs().MFSR & 0x7) != 0)
                return (1<<31) | (1<<29) | regs().MFSR;
            if ((regs().MSR & LPI2C_MSR_MBF) != 0)
                return (1<<31) | (3<<29) | regs().MSR;
            return -1;
        }
    }

    // clear the NACK detection flag
    if (regs().MSR & LPI2C_MSR_NDF)
        regs().MSR |= LPI2C_MSR_NDF; // TODO: replace with equal

    // Any error flags?
    if (regs().MSR & ~(LPI2C_MSR_SDF | LPI2C_MSR_EPF | LPI2C_MSR_TDF))
        return (1<<31) | (1<<28) | regs().MSR;

    // is always positive
    return regs().MSR;
}

int Slave::recv(uint8_t &address, uint8_t *message, unsigned n, uint32_t timeout) {

    clear_errors_and_fifos();

    uint32_t ssr, sasr, srdr, count = 0;

    // Wait for address byte to have been received
    timeout++;
    do {
        // TODO: return culprit of the timeout
        if (timeout-- == 0) return -1;
        ssr = regs().SSR;
        // error flags
        if (ssr & (LPI2C_SSR_FEF | LPI2C_SSR_BEF)) return (1<<31) | ssr;
    } while ((ssr & LPI2C_SSR_AVF) == 0);

    // unexpected flags
    if (ssr & ~(LPI2C_SSR_AVF
                | LPI2C_SSR_AM1F
                | LPI2C_SSR_RSF
                | LPI2C_SSR_BBF
                | LPI2C_SSR_SBF
    )) return (1<<31) | (1<<30) | ssr;

    // wrong address?
    if ((ssr & LPI2C_SSR_AM1F) == 0) return -2;

    // read address
    sasr = regs().SASR;
    address = 0xff & sasr;
    // is address not valid?
    if (sasr & LPI2C_SASR_ANV) return -3;
    // is transmit request?
    if (sasr & 1) return -4;

    // receive bytes
    while (true) {

        // wait for byte
        do {
            // TODO: return culprit of the timeout
            if (timeout-- == 0) return -1;
            ssr = regs().SSR;
            // error flags
            if (ssr & (LPI2C_SSR_FEF | LPI2C_SSR_BEF))
                return (1<<31) | (1<<29) | ssr;
            // stop flag?
            if (ssr & LPI2C_SSR_SDF) {
                // clear stop flag
                regs().SSR = LPI2C_SSR_SDF;
                return count;
            }
        } while ((ssr & LPI2C_SSR_RDF) == 0);

        // unexpected flags
        if (ssr & ~(
            LPI2C_SSR_RDF
            | LPI2C_SSR_RSF
            | LPI2C_SSR_AM0F
            | LPI2C_SSR_AM1F
            | LPI2C_SSR_AVF
            | LPI2C_SSR_RSF
            | LPI2C_SSR_BBF
            | LPI2C_SSR_SBF
        )) {
            return (1<<31) | (3<<29) | ssr;
        }

        // read received data
        srdr = regs().SRDR;

        // nothing was there?
        if (srdr & LPI2C_SRDR_RXEMPTY) {
            return -5;
        }

        // start of frame, but buffer not empty?
        if ((srdr & LPI2C_SRDR_SOF) != 0 && count != 0) {
            return -6;
        }

        // buffer full?
        if (count >= n) {
            return -7;
        }

        // put data in buffer
        message[count] = (uint8_t) srdr;
        count++;
    }
}

} /* namespace Twi */
} /* namespace Teensy */

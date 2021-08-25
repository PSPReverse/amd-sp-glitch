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


ifeq (,$(ARDUINO_PATH))
$(error You need to set the environment variable ARDUINO_PATH (see README.md))
endif

I2C_BASE_CLK = 60000000
I2C_BAUDRATE = 4670000
#I2C_BAUDRATE = 3300000
#I2C_BAUDRATE = 1000000

TEENSY_TOOL = ../teensy_tool/teensy_loader_cli

NO_ARDUINO=True
COMPILERPATH=$(ARDUINO_PATH)/hardware/tools/arm/bin
TEENSY4_PATH=$(ARDUINO_PATH)hardware/teensy/avr/cores/teensy4/

default: main.elf main.hex

vpath %.c $(TEENSY4_PATH) $(pwd)
vpath %.cpp $(TEENSY4_PATH) $(pwd)
vpath %.ld $(TEENSY4_PATH) $(pwd)

include $(TEENSY4_PATH)Makefile

MCU_LD := $(TEENSY4_PATH)$(MCU_LD)

CPPFLAGS += -I$(TEENSY4_PATH) -DI2C_BASE_CLK=$(I2C_BASE_CLK) -DI2C_BAUDRATE=$(I2C_BAUDRATE)
TEENSY_C_FILES += $(wildcard $(TEENSY4_PATH)*.c)
TEENSY_CPP_FILES += $(wildcard $(TEENSY4_PATH)*.cpp)
OBJS += $(filter-out %main.o,$(TEENSY_C_FILES:.c=.o) $(TEENSY_CPP_FILES:.cpp=.o))

program: $(TARGET).hex
	$(TEENSY_TOOL) --mcu=TEENSY40 -s -w $^

$(TARGET).elf: $(OBJS) $(MCU_LD)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(TEENSY4_PATH)*.o $(TEENSY4_PATH)*.d *.o *.d main.elf main.hex

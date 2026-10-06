# DAC-ADC

DAC → ADC loopback demo for the **NUCLEO-F429ZI**. The DAC steps through a
set of output codes, the ADC reads each voltage back, and the board prints a
comparison table over the ST-LINK USB serial port.

Bare-metal register-level C: no HAL or CMSIS download needed, just
`arm-none-eabi-gcc` and `make`.

## Wiring

One jumper wire:

| From | To |
|---|---|
| **PA4** (DAC_OUT1), CN7 pin 17 (Zio "D24") | **PA3** (ADC1_IN3), CN9 pin 1 (Arduino "A0") |

With the ST-LINK USB connector at the top and looking at the top side of the
board, the odd pins of each Zio header are in the **left** column:

- **PA3** is the top-left pin of CN9, the lower header on the left side.
- **PA4** is the 9th pin down in the left column of CN7, the upper header on
  the right side.

Source: UM1974, Table 16 and Figure 9.

## Build and flash

```sh
make          # builds build/dac_adc.elf / .bin
make flash    # programs over ST-LINK using STM32_Programmer_CLI
```

You can also drag `build/dac_adc.bin` onto the `NOD_F429ZI` USB drive that
the Nucleo shows up as.

## View the output

The ST-LINK virtual COM port runs at 115200 8N1 (USART3, PD8/PD9):

```sh
picocom -b 115200 /dev/ttyACM0
# or
python3 -m serial.tools.miniterm /dev/ttyACM0 115200
```

Press the black **RESET** button to see the banner again. The output looks like this:

```
NUCLEO-F429ZI DAC -> ADC loopback (PA4 -> PA3)
DAC output buffer: ON

  DAC code   DAC mV   ADC code   ADC mV   error mV
  --------   ------   --------   ------   --------
         0        0        ...      ...        ...
       ...
```

## What to point out

- **Middle of the range:** the DAC and ADC agree to within a few mV, because
  each step averages 16 ADC samples.
- **Ends of the range:** with the DAC output buffer on (`DAC_BUFFER 1` in
  `src/main.c`), the output can't swing closer than about 0.2 V to the rails.
  Codes near 0 and 4095 therefore show large errors.
- **Buffer off:** set `DAC_BUFFER 0` and rebuild. The output then reaches the
  rails, but its output impedance rises to about 15 kΩ. This is fine for the
  ADC's long 480-cycle sample time, but not for driving a real load.
- **Unplug the jumper:** the readings drift and float, which shows that the
  numbers really come from the wire.

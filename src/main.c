/*
 * DAC -> ADC loopback demo for NUCLEO-F429ZI.
 *
 * Wiring: one jumper from PA4 (DAC_OUT1) to PA3 (ADC1_IN3, Arduino A0).
 *
 * The DAC is stepped through a range of codes; after each step the ADC
 * reads the voltage back and a table is printed over the ST-LINK virtual
 * COM port (USART3 on PD8/PD9, 115200 8N1). The sweep repeats every 3 s.
 *
 * Runs on the 16 MHz HSI clock at reset, so no clock setup is needed.
 */
#include <stdint.h>

#define REG(addr) (*(volatile uint32_t *)(addr))

/* RCC */
#define RCC_AHB1ENR  REG(0x40023830)
#define RCC_APB1ENR  REG(0x40023840)
#define RCC_APB2ENR  REG(0x40023844)

/* GPIO */
#define GPIOA_MODER  REG(0x40020000)
#define GPIOD_MODER  REG(0x40020C00)
#define GPIOD_AFRH   REG(0x40020C24)

/* USART3 */
#define USART3_SR    REG(0x40004800)
#define USART3_DR    REG(0x40004804)
#define USART3_BRR   REG(0x40004808)
#define USART3_CR1   REG(0x4000480C)

/* DAC */
#define DAC_CR       REG(0x40007400)
#define DAC_DHR12R1  REG(0x40007408)

/* ADC1 */
#define ADC1_SR      REG(0x40012000)
#define ADC1_CR2     REG(0x40012008)
#define ADC1_SMPR2   REG(0x40012010)
#define ADC1_SQR1    REG(0x4001202C)
#define ADC1_SQR3    REG(0x40012034)
#define ADC1_DR      REG(0x4001204C)

/* SysTick */
#define SYST_CSR     REG(0xE000E010)
#define SYST_RVR     REG(0xE000E014)
#define SYST_CVR     REG(0xE000E018)

#define CPU_HZ       16000000u
#define VREF_MV      3300u   /* VDDA on the Nucleo */
#define ADC_AVG      16u     /* readings averaged per step */

/*
 * 1 = DAC output buffer on: low output impedance, but the output can't
 *     swing closer than ~0.2 V to either rail, so the extremes read wrong.
 * 0 = buffer off: rail-to-rail, but high output impedance (~15 kOhm).
 */
#define DAC_BUFFER   1

static void delay_ms(uint32_t ms)
{
    SYST_RVR = CPU_HZ / 1000 - 1;
    SYST_CVR = 0;
    SYST_CSR = 1u << 2 | 1u << 0;               /* CPU clock, enable */
    while (ms--)
        while (!(SYST_CSR & 1u << 16)) {}       /* COUNTFLAG */
    SYST_CSR = 0;
}

static void uart_init(void)
{
    RCC_AHB1ENR |= 1u << 3;                     /* GPIOD */
    RCC_APB1ENR |= 1u << 18;                    /* USART3 */

    /* PD8 = TX, PD9 = RX, alternate function 7 */
    GPIOD_MODER = (GPIOD_MODER & ~(0xFu << 16)) | (0xAu << 16);
    GPIOD_AFRH  = (GPIOD_AFRH & ~0xFFu) | 0x77u;

    USART3_BRR = 0x8B;                          /* 16 MHz / 115200 = 8.6875 */
    USART3_CR1 = 1u << 13 | 1u << 3;            /* UE, TE */
}

static void uart_putc(char c)
{
    while (!(USART3_SR & 1u << 7)) {}           /* TXE */
    USART3_DR = (uint8_t)c;
}

static void uart_puts(const char *s)
{
    while (*s) {
        if (*s == '\n')
            uart_putc('\r');
        uart_putc(*s++);
    }
}

/* Print a signed integer right-aligned in a field of the given width. */
static void uart_putint(int32_t v, int width)
{
    char buf[12];
    int n = 0;
    uint32_t u = v < 0 ? -(uint32_t)v : (uint32_t)v;

    do {
        buf[n++] = '0' + u % 10;
        u /= 10;
    } while (u);
    if (v < 0)
        buf[n++] = '-';
    while (width-- > n)
        uart_putc(' ');
    while (n)
        uart_putc(buf[--n]);
}

static void dac_init(void)
{
    RCC_AHB1ENR |= 1u << 0;                     /* GPIOA */
    RCC_APB1ENR |= 1u << 29;                    /* DAC */

    GPIOA_MODER |= 3u << (4 * 2);               /* PA4 analog */
    DAC_CR = (DAC_BUFFER ? 0 : 1u << 1) | 1u << 0;  /* BOFF1, EN1 */
}

static void dac_write(uint16_t code)
{
    DAC_DHR12R1 = code;
}

static void adc_init(void)
{
    RCC_APB2ENR |= 1u << 8;                     /* ADC1 */

    GPIOA_MODER |= 3u << (3 * 2);               /* PA3 analog */
    ADC1_SQR1  = 0;                             /* 1 conversion */
    ADC1_SQR3  = 3;                             /* channel 3 */
    ADC1_SMPR2 = 7u << (3 * 3);                 /* 480 cycles on ch3 */
    ADC1_CR2   = 1u << 0;                       /* ADON */
    delay_ms(1);
}

static uint16_t adc_read(void)
{
    ADC1_CR2 |= 1u << 30;                       /* SWSTART */
    while (!(ADC1_SR & 1u << 1)) {}             /* EOC */
    return (uint16_t)ADC1_DR;
}

static uint16_t adc_read_avg(void)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < ADC_AVG; i++)
        sum += adc_read();
    return (uint16_t)((sum + ADC_AVG / 2) / ADC_AVG);
}

static uint32_t code_to_mv(uint32_t code)
{
    return (code * VREF_MV + 2047) / 4095;
}

static void run_sweep(void)
{
    static const uint16_t codes[] = {
        0, 128, 256, 512, 1024, 1536, 2048, 2560, 3072, 3584, 3968, 4095,
    };

    uart_puts("\n  DAC code   DAC mV   ADC code   ADC mV   error mV\n");
    uart_puts("  --------   ------   --------   ------   --------\n");

    for (unsigned i = 0; i < sizeof codes / sizeof codes[0]; i++) {
        dac_write(codes[i]);
        delay_ms(5);                            /* let the output settle */
        uint16_t raw = adc_read_avg();

        int32_t set_mv = code_to_mv(codes[i]);
        int32_t got_mv = code_to_mv(raw);

        uart_putint(codes[i], 10);
        uart_putint(set_mv, 9);
        uart_putint(raw, 11);
        uart_putint(got_mv, 9);
        uart_putint(got_mv - set_mv, 11);
        uart_puts("\n");
    }
}

int main(void)
{
    uart_init();
    dac_init();
    adc_init();

    uart_puts("\nNUCLEO-F429ZI DAC -> ADC loopback (PA4 -> PA3)\n");
    uart_puts(DAC_BUFFER ? "DAC output buffer: ON\n" : "DAC output buffer: OFF\n");

    for (;;) {
        run_sweep();
        delay_ms(3000);
    }
}

/* Minimal startup: vector table, .data/.bss init, jump to main. */
#include <stdint.h>

extern uint32_t _estack, _etext, _sdata, _edata, _sbss, _ebss;
int main(void);

void Reset_Handler(void)
{
    uint32_t *src = &_etext;
    for (uint32_t *dst = &_sdata; dst < &_edata;)
        *dst++ = *src++;
    for (uint32_t *dst = &_sbss; dst < &_ebss;)
        *dst++ = 0;
    main();
    for (;;) {}
}

void Default_Handler(void)
{
    for (;;) {}
}

/* Only the core exceptions are populated; no peripheral interrupts are used. */
__attribute__((section(".isr_vector"), used))
const void *vector_table[16] = {
    &_estack,
    Reset_Handler,
    Default_Handler, /* NMI */
    Default_Handler, /* HardFault */
    Default_Handler, /* MemManage */
    Default_Handler, /* BusFault */
    Default_Handler, /* UsageFault */
    0, 0, 0, 0,
    Default_Handler, /* SVCall */
    Default_Handler, /* DebugMon */
    0,
    Default_Handler, /* PendSV */
    Default_Handler, /* SysTick */
};

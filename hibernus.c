#include "hibernus.h"
#include <msp430.h>


// data ptrs
extern uint8_t __bss_low, __bss_high;
extern uint8_t __data_low, __data_high;
extern uint8_t __stack_low, __STACK_END;
extern uint8_t __cp_stack_high;

// use FRAM
#pragma SET_DATA_SECTION(".TI.noinit")

// core processor registers
uint32_t core_registers[15];
uint32_t return_address;

// Snapshots of volatile memory
uint8_t bss_snapshot[BSS_SIZE];
uint8_t data_snapshot[DATA_SIZE];
uint8_t stack_snapshot[STACK_SIZE];
uint8_t peripheral_registers[3]; // p1out + p1dir + p1ren
// no heap for now, you should not use it anyway :3

#pragma SET_DATA_SECTION() 
#pragma PERSISTENT(snapshot_valid)
uint16_t snapshot_valid = 0;
#pragma PERSISTENT(from_hiber)
uint16_t from_hiber = 0;

/**
 * @brief _system_pre_init tells the systen whether or not the C environment should
 * initialise the variables at startup. Ideally you would want to avoid this 
 * when restoring a snapshot since it would just overwrite those initialisations.
 * For more info about this function check the TI datasheets and chenge behaviour 
 * according to your needs
 */
int _system_pre_init(void){
    return !snapshot_valid;
}

void send_EOT_and_shutdown(uint8_t gpio){
    hibernate();
    if (from_hiber){
        P4OUT |= gpio; // !!! ask PSU to cut the power
        __bis_SR_register(LPM4_bits); // sleep while I'm being killed
    }
}

/*
the comparator is not used but it might in the future so a possible solution is here
inline void initCOMPE(void){
    // Setup Comparator_E
    CECTL0 = CEIPEN | CEIPSEL_12;             // Enable V+, input channel CE12
    CECTL1 = CEPWRMD_1|                       // normal power mode
             CEF      |                       // filter enabled
             CEFDLY_3 ;                       // filter delay
    CECTL2 = CEREFL_1 |                       // VREF 1.2V is selected
             CERS_2   |                       // VREF applied to R-ladder
             CERSEL   |                       // to -terminal (CEEX is 0)
             CEREF1_17|                       // (n + 1) / 32 * 2 * 2
             CEREF0_20;                       // (n + 1) / 32 * 2 * 2
    CECTL3 = CEPD12   ;                       // Input Buffer Disable @P9.4/CE12
    CEINT  = CEIE     |                       // Interrupt enabled
             CEIIE    ;                       // Inverted interrupt enabled

    while(!(REFCTL0 & REFBGRDY));            // Wait for reference generator to settle

    CECTL1 |= CEON;                           // Turn On Comparator_E

    // __delay_cycles(75);                    // delay for the reference to settle
}
*/

void hibernus_init(void){
    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    // Indicate powerup
    if (snapshot_valid)
       restore();

    // ! reachable iff first boot or invalid snapshot (debug op)
    __no_operation();
}

void hibernate (void){
    snapshot_valid = 0;

    asm(" MOVA R1,&core_registers");      // R1: Stack Pointer
    asm(" MOVA R2,&core_registers+4");    // R2: Status Register
    asm(" MOVA R3,&core_registers+8");
    asm(" MOVA R4,&core_registers+12");
    asm(" MOVA R5,&core_registers+16");
    asm(" MOVA R6,&core_registers+20");
    asm(" MOVA R7,&core_registers+24");
    asm(" MOVA R8,&core_registers+28");
    asm(" MOVA R9,&core_registers+32");
    asm(" MOVA R10,&core_registers+36");
    asm(" MOVA R11,&core_registers+40");
    asm(" MOVA R12,&core_registers+44");
    asm(" MOVA R13,&core_registers+48");
    asm(" MOVA R14,&core_registers+52");
    asm(" MOVA R15,&core_registers+56");


    // Return to the next line of Hibernate();
    return_address = *(uint16_t *)(__get_SP_register());

    // Save a snapshot of volatile memory
    // allocate to .npbss to prevent being overwritten when copying .bss
    static volatile uint8_t * src __attribute__((section(".npbss")));
    static volatile uint8_t * dst __attribute__((section(".npbss")));
    static volatile size_t len __attribute__((section(".npbss")));

    // bss
    src = &__bss_low;
    dst = (uint8_t *)bss_snapshot;
    len = &__bss_high - src;
    memcpy((void *)dst, (void *)src, len);

    // data
    src = &__data_low;
    dst = (uint8_t *)data_snapshot;
    len = &__data_high - src;
    memcpy((void *)dst, (void *)src, len);

    // stack
#ifdef TRACK_STACK
    src = (uint8_t *)core_registers[0]; // Saved SP
#else
    src = &__stack_low;
#endif

    /* stack_low-----[SP-------stack_high] */
    len = &__STACK_END - src;
    uint16_t offset = (uint16_t)(src - &__stack_low) >> 1; // word offset
    dst = (uint8_t *)&stack_snapshot[offset];
    memcpy((void *)dst, (void *)src, len);

    // peripheral register
    peripheral_registers[0] = P1OUT;
    peripheral_registers[1] = P1DIR;
    peripheral_registers[2] = P1REN;

    snapshot_valid = 1;
    from_hiber = 1;
}

void restore (void){
    // move to different spaces to avoid overwriting
    static volatile uint32_t src __attribute__((section(".npbss")));
    static volatile uint32_t dst __attribute__((section(".npbss")));
    static volatile size_t len __attribute__((section(".npbss")));

    from_hiber = 0;

    // data
    dst = (uint32_t) &__data_low;
    src = (uint32_t)data_snapshot;
    len = (uint32_t)&__data_high - dst;
    memcpy((void *)dst, (void *)src, len);

    // bss
    dst = (uint32_t) &__bss_low;
    src = (uint32_t)bss_snapshot;
    len = (uint32_t)&__bss_high - dst;
    memcpy((void *)dst, (void *)src, len);

    // stack
#ifdef TRACK_STACK
        dst = core_registers[0]; // Saved stack pointer
#else
        dst = (uint32_t)&__stack_low; // Save full stack
#endif

    len = (uint8_t *)&__STACK_END - (uint8_t *)dst;
    uint16_t offset = (uint16_t)((uint16_t *)dst - (uint16_t *)&__stack_low); // word offset
    src = (uint32_t)&stack_snapshot[offset];

#pragma diag_suppress=770

    /* Move to separate stack space before restoring original stack. */
    __set_SP_register((uint16_t)&__cp_stack_high); // Move to separate stack

#pragma diag_warning=770
    memcpy((uint8_t *)dst, (uint8_t *)src, len);  // Restore default stack


    // peripheral registers
    P1OUT = peripheral_registers[0] & ~(BIT0|BIT1);
    P1DIR = peripheral_registers[1];
    P1REN = peripheral_registers[2];

    asm(" MOVA &core_registers,R1");
    // !!! DO NOT REMOVE THOSE NOPS AS THEY ARE REQUIRED BY HW
    __no_operation();
    asm(" MOVA &core_registers+4,R2");
    __no_operation();
    asm(" MOVA &core_registers+8,R3");
    asm(" MOVA &core_registers+12,R4");
    asm(" MOVA &core_registers+16,R5");
    asm(" MOVA &core_registers+20,R6");
    asm(" MOVA &core_registers+24,R7");
    asm(" MOVA &core_registers+28,R8");
    asm(" MOVA &core_registers+32,R9");
    asm(" MOVA &core_registers+36,R10");
    asm(" MOVA &core_registers+40,R11");
    asm(" MOVA &core_registers+44,R12");
    asm(" MOVA &core_registers+48,R13");
    asm(" MOVA &core_registers+52,R14");
    asm(" MOVA &core_registers+56,R15");


    // Restore return address
    *(uint16_t *)(__get_SP_register()) = return_address;
}

#if 0
// possible comparator solution
#pragma vector = COMP_E_VECTOR
__interrupt void COMPE_ISR(void)
{
    switch(__even_in_range(CEIV, CEIV_CERDYIFG)) {
        case CEIV_NONE: 
            break;
        case CEIV_CEIFG:
            P1OUT ^= BIT1;
            __delay_cycles(10000000);
            CEINT &= ~CEIFG;
            if (snapshot_valid == 1){
                if (need_restore) {
                    // boot from power failure, Restore needed
                    need_restore = 0; // clear this flag
                #pragma FORCEINLINE
                    restore();
                    // ** Does not return **
                    /* !** Restore returns to line after Hibernate() **! */
                } else {
                     // from sleep, Restore not needed
                    __bic_SR_register_on_exit(LPM4_bits); // Wake up on return
                }
            } else {
                // first boot, start execution from beginning
                need_restore = 0; // clear this flag
                __bic_SR_register_on_exit(LPM4_bits); // Wake up on return
            }
            break;
        case CEIV_CEIIFG:
            P1OUT ^= BIT0;
            __delay_cycles(10000000);
            CEINT &= ~CEIIFG;
            // Power is failing
        #pragma FORCEINLINE
            hibernate();
            // ** Returning from Hibernate() or from Restore() **
            // Returning from Hibernate(), go to sleep
            // Enter LPM4 with interrupts enabled (wait for restore)
            if (from_hiber) {
                // from Hibernate, sleep on return
                __bis_SR_register_on_exit(LPM4_bits + GIE);
            }
            else {
                // from Restore, wake up on return
                __bic_SR_register_on_exit(LPM4_bits);
            }
            break;
        case CEIV_CERDYIFG:
            CEINT &= ~CERDYIFG;
            break;
        default: 
            break;
    }
}
#endif

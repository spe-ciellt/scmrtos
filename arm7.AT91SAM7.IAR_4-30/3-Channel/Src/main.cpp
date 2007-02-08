//********************************************************************************
//*
//*     FULLNAME:  Single-Chip Microcontroller Real-Time Operating System
//*
//*     NICKNAME:  scmRTOS
//*
//*     PROCESSOR: AT91SAM7xxx (Atmel)
//*
//*     TOOLKIT:   EWARM (IAR Systems)
//*
//*     PURPOSE:   Port Test File
//*
//*     Version:   3.00-beta
//*
//*     $Revision$
//*     $Date$
//*
//*     Copyright (c) 2003-2006, Harry E. Zhurov
//*
//*     =================================================================
//*     scmRTOS is free software; you can redistribute it and/or
//*     modify it under the terms of the GNU General Public License
//*     as published by the Free Software Foundation; either version 2
//*     of the License, or (at your option) any later version.
//*
//*     This program is distributed in the hope that it will be useful,
//*     but WITHOUT ANY WARRANTY; without even the implied warranty of
//*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//*     GNU General Public License for more details.
//*
//*     You should have received a copy of the GNU General Public License
//*     along with this program; if not, write to the Free Software
//*     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//*     MA  02110-1301, USA.
//*     =================================================================
//*
//*     =================================================================
//*     See http://scmrtos.sourceforge.net for documentation, latest
//*     information, license and contact details.
//*     =================================================================
//*
//********************************************************************************
//*     ARM port by Sergey A. Borshch, Copyright (c) 2006


//---------------------------------------------------------------------------
#include <scmRTOS.h>

//---------------------------------------------------------------------------
//
//      Process types
//
typedef OS::process<OS::pr0, 200> TProc1;
typedef OS::process<OS::pr1, 200> TProc2;
typedef OS::process<OS::pr2, 200> TProc3;
//---------------------------------------------------------------------------
//
//      Process objects
//
TProc1 Proc1;
TProc2 Proc2;
TProc3 Proc3;
dword T;                         // global variable for OS::GetTickCount testing
                                 //

//------------------------------------------------------------------------------
//
//   Message "body"
//
//
class TSlon
{
public:
    TSlon() { }
    virtual void eat() = 0;      // feed the slon. For non-russians: slon == elephant ;)
};
//------------------------------------------------------------------------------
class TAfricanSlon : public TSlon
{
public:
    virtual void eat()
    {
        TCritSect cs;

        AT91C_BASE_PIOA->PIO_SODR = (1 << 0);
        AT91C_BASE_PIOA->PIO_CODR = (1 << 0);
    }
};
//------------------------------------------------------------------------------
class TIndianSlon : public TSlon
{
public:
    virtual void eat()
    {
        TCritSect cs;

        AT91C_BASE_PIOA->PIO_SODR = (1 << 0);
        AT91C_BASE_PIOA->PIO_CODR = (1 << 0);
        AT91C_BASE_PIOA->PIO_SODR = (1 << 0);
        AT91C_BASE_PIOA->PIO_CODR = (1 << 0);
    }
};
//------------------------------------------------------------------------------

TAfricanSlon African;
TIndianSlon  Indian;

OS::channel<TSlon*, 8> SlonQueue; // OS::channel object for 8 'TSlon' items

OS::TEventFlag Timer_Ovf;
//---------------------------------------------------------------------------
int main()
{
    OS::Run();
}
//---------------------------------------------------------------------------
OS_PROCESS void TProc1::Exec()
{
    for(;;)
    {
        Timer_Ovf.Wait();
        SlonQueue.push(&African);
    }
}
//---------------------------------------------------------------------------
OS_PROCESS void TProc2::Exec()
{
    for(;;)
    {
        T += OS::GetTickCount();

        Sleep(1);
        SlonQueue.push(&Indian);
    }
}
//---------------------------------------------------------------------------
OS_PROCESS void TProc3::Exec()
{
    for(;;)
    {
        //--------------------------------------------------
        //
        //            Channel test
        //
        //
        //     Get data through channel
        //
        TSlon *p;
        SlonQueue.pop(p);     // get pointer from queue
        p->eat();             // feed the appropriate Slon
    }
}
//---------------------------------------------------------------------------
void OS::SystemTimerUserHook() { }
//---------------------------------------------------------------------------
void OS::IdleProcessUserHook() { }
//---------------------------------------------------------------------------
OS_INTERRUPT void Timer_ISR()
{
    OS::TISRW ISRW;
    volatile dword Tmp = AT91C_BASE_TCB->TCB_TC0.TC_SR;   // read to clear int flag

    Timer_Ovf.SignalISR();

    AT91C_BASE_AIC->AIC_EOICR = 0;
}
//-----------------------------------------------------------------------------

_C_LIB_DECL
#pragma language=extended
#pragma location="ICODE"

#define	RTOS_TICK_RATE  1000		// Hz
#define	SLCK            32768L			// something about :-(
#define	MAINCLK         1843200LL
#define	PLLMUL          26
#define	PLLDIV          5
#define	PLLCLK          ((MAINCLK * PLLMUL) / PLLDIV)
#define	MCK             (PLLCLK / 2)
#define	PCK             SLCK
#define	TEST_TIMER_RATE 3500		// Hz

#ifndef	AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE     // ioAT91SAM7Sxx.h patch
#define	AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE	AT91C_AIC_SRCTYPE_INT_EDGE_TRIGGERED
#endif

#if scmRTOS_CONTEXT_SWITCH_SCHEME == 0
extern "C" void ContextSwitcher_ISR();
#endif
int __low_level_init(void)
{

    AT91C_BASE_PIOA->PIO_MDDR = ~0;		            // push-pull
    AT91C_BASE_PIOA->PIO_PPUDR = ~0;	        	// pull-up disable
    AT91C_BASE_PIOA->PIO_OER = (1 << 0);		    // pin 0 = output

	// Main oscillator
    AT91C_BASE_CKGR->CKGR_MOR = (1 * AT91C_CKGR_MOSCEN) | (0 * AT91C_CKGR_OSCBYPASS) | (0xFF * AT91C_CKGR_OSCOUNT);
    while( ! AT91C_BASE_PMC->PMC_SR & (1<<AT91C_PMC_MOSCS));

    // PLL
    AT91C_BASE_CKGR->CKGR_PLLR = (PLLDIV * AT91C_CKGR_DIV / 0xFF) | (0x1FLL * AT91C_CKGR_PLLCOUNT / 0x3F) | (0 * AT91C_CKGR_OUT / 0x03) \
                                | (PLLMUL * 1LL* AT91C_CKGR_MUL / 0x7FF) | (0 * AT91C_CKGR_USBDIV / 0x03);
    while( ! AT91C_BASE_PMC->PMC_SR & (1<<AT91C_PMC_LOCK));

    // Main clock
    AT91C_BASE_PMC->PMC_MCKR = AT91C_PMC_CSS_PLL_CLK | AT91C_PMC_PRES_CLK_2;
    // System (core) clock
    AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_PCK;       // Proc clock enabled
    // Peripherial clock
    AT91C_BASE_PMC->PMC_PCER = (1<<AT91C_ID_TC0);   // Timer clock enable

    // RTOS System Timer
    AT91C_BASE_PITC->PITC_PIMR = (((MCK / 16) / RTOS_TICK_RATE - 1) & AT91C_PITC_PIV) \
                                | (1 * AT91C_PITC_PITEN) | (1 * AT91C_PITC_PITIEN);
    // Periodic timer
    AT91C_BASE_TCB->TCB_BMR = AT91C_TCB_TC0XC0S_NONE | AT91C_TCB_TC1XC1S_NONE | AT91C_TCB_TC2XC2S_NONE;

    AT91C_BASE_TCB->TCB_TC0.TC_CMR =
                         AT91C_TC_BSWTRG_NONE | AT91C_TC_BEEVT_NONE | AT91C_TC_BCPC_NONE | AT91C_TC_BCPB_NONE   // TIOB: nothing
                        | AT91C_TC_ASWTRG_NONE | AT91C_TC_AEEVT_NONE | AT91C_TC_ACPC_NONE | AT91C_TC_ACPA_NONE  // TIOA: nothing
                        | AT91C_TC_WAVE | AT91C_TC_WAVESEL_UP_AUTO                      // waveform mode, up to RC
                        | AT91C_TC_ENETRG | AT91C_TC_EEVT_XC2 | AT91C_TC_EEVTEDG_NONE   // no ext. trigger
                        | (0 * AT91C_TC_CPCDIS) | (0 * AT91C_TC_CPCSTOP) | AT91C_TC_BURST_NONE
                        | (0 * AT91C_TC_CLKI) | AT91C_TC_CLKS_TIMER_DIV1_CLOCK;         // clock mck / 2, inc on rising edge
    AT91C_BASE_TCB->TCB_TC0.TC_RC = MCK / 2 / TEST_TIMER_RATE;          // Timeout
    AT91C_BASE_TCB->TCB_TC0.TC_IER = AT91C_TC_CPCS;                     // enable ints on RC compare
    AT91C_BASE_TCB->TCB_TC0.TC_CCR = AT91C_TC_SWTRG | AT91C_TC_CLKEN;   // reset and enable TC0 clock

    // Set RTOS timer handler
    AT91C_BASE_AIC->AIC_SMR[AT91C_ID_SYS] = AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE | AT91C_AIC_PRIOR_LOWEST;
    AT91C_BASE_AIC->AIC_SVR[AT91C_ID_SYS] = (dword)OS::SystemTimer_ISR;

    // Set timer handler
    AT91C_BASE_AIC->AIC_SMR[AT91C_ID_TC0] = AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE | AT91C_AIC_PRIOR_LOWEST;
    AT91C_BASE_AIC->AIC_SVR[AT91C_ID_TC0] = (dword)Timer_ISR;



    #if scmRTOS_CONTEXT_SWITCH_SCHEME == 1
        AT91C_BASE_AIC->AIC_SMR[CONTEXT_SWITCH_INT] = AT91C_AIC_SRCTYPE_INT_POSITIVE_EDGE | AT91C_AIC_PRIOR_HIGHEST;
        AT91C_BASE_AIC->AIC_SVR[CONTEXT_SWITCH_INT] = (dword)ContextSwitcher_ISR;

        AT91C_BASE_AIC->AIC_IECR = (1<<AT91C_ID_SYS)|(1<<CONTEXT_SWITCH_INT)|(1<<AT91C_ID_TC0);
    #else
        AT91C_BASE_AIC->AIC_IECR = (1<<AT91C_ID_SYS)|(1<<AT91C_ID_TC0);
    #endif
    AT91C_BASE_AIC->AIC_EOICR = 0;                                      // Reset AIC logic

    return 1;
}
#pragma language=default
_END_C_LIB_DECL

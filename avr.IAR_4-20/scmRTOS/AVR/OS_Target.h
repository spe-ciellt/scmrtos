//********************************************************************************
//*
//*     FULLNAME:  Single-Chip Microcontroller Real-Time Operating System
//*
//*     NICKNAME:  scmRTOS
//*
//*     PROCESSOR: AVR (Atmel)
//*
//*     TOOLKIT:   EWAVR (IAR Systems)
//*               
//*     PURPOSE:   Target Dependent Stuff Header. Declarations And Definitions
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

#ifndef scmRTOS_AVR_H
#define scmRTOS_AVR_H

#include <commdefs.h>
#include <scmRTOS_defs.h>
#include <inavr.h>

//------------------------------------------------------------------------------
//
//    Compiler and Target checks
//
//
#ifndef __IAR_SYSTEMS_ICC__
#error "This file should only be compiled with IAR C++ Compiler"
#endif // __IAR_SYSTEMS_ICC__

#if (((__TID__ >> 8) & 0x7F) != 90)
#error "This file must be compiled for AVR processor only."
#endif

#if __VER__ < 412
#error "This file must be compiled by IAR C/C++ Compiler v4.12 or higher."
#endif

#define INLINE _Pragma("inline=forced")

//------------------------------------------------------------------------------
//
//    Target specific types
//
//
typedef byte TStackItem;
typedef byte TStatusReg;

//-----------------------------------------------------------------------------
//
//      Target specific macros
//
//
#define OS_PROCESS    __task
#define OS_INTERRUPT  __interrupt
#define DUMMY_INSTR() __no_operation()

//--------------------------------------------------
//
//   Uncomment macro value below for SystemTimer() run in critical section
// 
//   This is useful (and necessary) when target processor has hardware 
//   enabled nested interrups. AVR does not have such interrupts.
// 
#define SYS_TIMER_CRIT_SECT() // TCritSect cs

#define SEPARATE_RETURN_STACK  1


#include "scmRTOS_TARGET_CFG.h"

//-----------------------------------------------------------------------------
//
//     The Critital Section Wrapper
//
//
class TCritSect
{
public:
    TCritSect () : StatusReg(__save_interrupt()) { __disable_interrupt(); }
    ~TCritSect() { __restore_interrupt(StatusReg); }

private:
    TStatusReg StatusReg;

};
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//     Priority stuff
//
//
namespace OS
{
struct TPrioMaskTable
{
    TPrioMaskTable()
    {
        TProcessMap pm = 0x01;
        for(byte i = 0; i < sizeof(Table); i++)
        {
            Table[i] = pm;
            pm <<= 1;
        }
    }

    TProcessMap Table[scmRTOS_PROCESS_COUNT+1];
};
extern TPrioMaskTable PrioMaskTable;

INLINE inline OS::TProcessMap GetPrioTag(const byte pr) { return PrioMaskTable.Table[pr]; }

#if scmRTOS_PRIORITY_ORDER == 0
    INLINE inline byte GetHighPriority(TProcessMap pm)
    {
        byte pr = 0;

        while( !(pm & 0x0001) )
        {
            pr++;
            pm >>= 1;
        }
        return (TPriority)pr;
    }
#else
    INLINE inline TPriority GetHighPriority(TProcessMap pm)
    {
        byte pr = scmRTOS_PROCESS_COUNT;

        while( !(pm & (1 << scmRTOS_PROCESS_COUNT) ) )
        {
            pr--;
            pm <<= 1;
        }
        return (TPriority)pr;
    }
#endif // scmRTOS_PRIORITY_ORDER
}


//-----------------------------------------------------------------------------
//
//     Interrupt and Interrupt Service Routines support
//
INLINE inline TStatusReg GetInterruptState( )             { return __save_interrupt(); }
INLINE inline void       SetInterruptState(TStatusReg sr) { __set_interrupt_state(sr); }

INLINE inline void EnableInterrupts() { __enable_interrupt();  }
INLINE inline void DisableInterrupts() { __disable_interrupt(); }

#pragma segment="CSTACK"
#pragma segment="RSTACK"

#define  ABS_WORD(x)  (*((volatile word*)x))

INLINE inline TStackItem* GetDataSP()   { return reinterpret_cast<TStackItem*>(ABS_WORD(28)); }
INLINE inline TStackItem* GetReturnSP() { return reinterpret_cast<TStackItem*>(SP); }

INLINE inline void SetISRStackPointers() 
{ 
    ABS_WORD(28) = reinterpret_cast<word>(__segment_end("CSTACK"));
    SP = reinterpret_cast<word>(__segment_end("RSTACK")) - 1;
}

INLINE inline void SetDataSP(TStackItem* sp)   { ABS_WORD(28) = reinterpret_cast<word>(sp); }
INLINE inline void SetReturnSP(TStackItem* sp) { SP = reinterpret_cast<word>(sp); }

namespace OS
{
    INLINE inline void EnableContextSwitch()  { EnableInterrupts();  }
    INLINE inline void DisableContextSwitch() { DisableInterrupts(); }
}
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
#include <OS_Kernel.h>
///////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
namespace OS
{
    struct TSavedSP
    {
        TStackItem *DataSP;
        TStackItem *ReturnSP;
    };

    extern TSavedSP SavedSP;
    //--------------------------------------------------------------------------
    //
    //      NAME       :   OS ISR support 
    //
    //      PURPOSE    :   Implements common actions on interrupt enter and exit 
    //                     under the OS
    //
    //      DESCRIPTION:
    //
    //
    class TISRW
    {
    public:
        INLINE  TISRW()  { ISR_Enter(); }
        INLINE  ~TISRW() { ISR_Exit();  }

    private:
        //-----------------------------------------------------
        INLINE void ISR_Enter() // volatile
        {
            Kernel.ISR_NestCount++;
        }
        //-----------------------------------------------------
        INLINE void ISR_Exit()
        {
            DisableInterrupts();
             if(--Kernel.ISR_NestCount) return;
            Kernel.SchedISR();
        }
        //-----------------------------------------------------
    };

    class TISRW_SS
    {
    public:
        INLINE  TISRW_SS()  { ISR_Enter(); }
        INLINE  ~TISRW_SS() { ISR_Exit();  }

    private:
        //-----------------------------------------------------
        INLINE void ISR_Enter() // volatile
        {
            if(Kernel.ISR_NestCount++ == 0)
            {
                 SavedSP.DataSP   = GetDataSP();
                 SavedSP.ReturnSP = GetReturnSP();
                 SetISRStackPointers();
            }
        }
        //-----------------------------------------------------
        INLINE void ISR_Exit()
        {
            DisableInterrupts();
             if(--Kernel.ISR_NestCount) return;
             SetReturnSP(SavedSP.ReturnSP);
             SetDataSP  (SavedSP.DataSP);
            Kernel.SchedISR();
        }
        //-----------------------------------------------------
    };
}
//-----------------------------------------------------------------------------

#endif // scmRTOS_AVR_H

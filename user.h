/******************************************************************************
;  *   	@型号				  : MC30P6250
;  *   	@创建日期             : 2021.08.04
;  *   	@公司/作者			  : SINOMCU-FAE
;  *   	@晟矽微技术支持       : 2048615934
;  *   	@晟矽微官网           : http://www.sinomcu.com/
;  *   	@版权                 : 2021 SINOMCU公司版权所有.
;  *---------------------- 建议 ---------------------------------
;  *   				变量定义时使用全局变量	               	
******************************************************************************/

#ifndef USER
#define USER

#ifdef SIMULATION
/*

   	__asm 
   	movai 0x40 
   	movra FSR
   	movai 48 
   	movra 0x07 
   	decr FSR
   	clrr INDF
   	djzr 0x07 
   	goto $ -3;
   	clrr 0x07 
   	__endasm;
*/
#define __at(x)
#define __asm       0x0
#define movai       +
#define movra       +
#define decr        +
#define clrr        +
#define goto        +
#define $           +
#define djzr        +
#define swapar      +
#define swapr       +
#define __endasm
#define __interrupt
#define nop
#define stop
#define clrwdt
#define _abuf       0x0
#define _STATUS     0x0
#define _statusbuf  0x0

#define __sfr       char
#endif

#include "mc30-common.h"
#include "mc30p6250.h"

#define u8 unsigned char
#define u16 unsigned int
#define u32 unsigned long int
#define uint8_t  unsigned char
#define uint16_t unsigned int
#define uint32_t unsigned long int

u8 abuf;
u8 statusbuf;

//=============Global Variable ===========

void CLR_RAM(void);
void IO_Init(void);
void TIMER0_INT_Init(void);
void TIMER1_PWM_Init(void);
void Sys_Init(void);

//============Define  Flag=================
typedef union
{
   	unsigned char byte;
   	struct
   	{
   	   	u8 bit0 : 1;
   	   	u8 bit1 : 1;
   	   	u8 bit2 : 1;
   	   	u8 bit3 : 1;
   	   	u8 bit4 : 1;
   	   	u8 bit5 : 1;
   	   	u8 bit6 : 1;
   	   	u8 bit7 : 1;
   	} bits;
}bit_flag; 	
volatile bit_flag flag1;

//#define FLAG_TIMER1_100ms	flag1.bits.bit0

#endif


/**************************** end of file *********************************************/
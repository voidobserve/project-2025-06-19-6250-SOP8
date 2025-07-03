/******************************************************************************
;  *   	@型号				  : MC30P6250
;  *   	@创建日期             : 2021.08.04
;  *   	@公司/作者			  : SINOMCU-FAE
;  *   	@晟矽微技术支持       : 2048615934
;  *   	@晟矽微官网           : http://www.sinomcu.com/
;  *   	@版权                 : 2021 SINOMCU公司版权所有.
******************************************************************************/

#include "user.h"

#define OSCILLATION_PIN1 (P11D)
#define OSCILLATION_PIN2 (P14D)
#define OSCILLATION_PIN3 (P15D)

// #define OSCILLATION_PIN1 (P14D)
// #define OSCILLATION_PIN2 (P11D)
// #define OSCILLATION_PIN3 (P15D)

// #define OSCILLATION_PIN1 (P15D)
// #define OSCILLATION_PIN2 (P14D)
// #define OSCILLATION_PIN3 (P11D)

#define PWM_CONTROLLER1 (P12D)

#define KEY_DETECT_INIT() \
    do                    \
    {                     \
        DDR13 = 1;        \
        P13PU = 0;        \
    } while (0)
#define KEY_DET_PIN (P13D)
#define KEY_KB_INT_ENABLE (P13KE)
#define KEY_FILTTER_PARAM (0b1111)

#define RT_DETECT_ENABLE 1
#if RT_DETECT_ENABLE
#define RT_DETECT_INIT() \
    do                   \
    {                    \
        DDR10 = 1;       \
        P10PU = 0;       \
        P10PD = 1;       \
    } while (0)
#define RT_DETECT_ON() \
    do                 \
    {                  \
        P10PU = 0;     \
        P10PD = 1;     \
    } while (0) // 检测震动
#define RT_DETECT_OFF() \
    do                  \
    {                   \
        P10PU = 1;      \
        P10PD = 0;      \
    } while (0) // 检测充电

#define RT_DETECT_UNINIT()        \
    do                            \
    {                             \
        P10PU = 1; /* 关闭上拉 */ \
        P10PD = 1; /* 关闭下拉 */ \
    } while (0) // 检测充电

#define IS_RT_DETECT() (P10PD)
// #define RT_DETECT_UNINIT() RT_DETECT_OFF() // 进入休眠前调用
#define RT_CHARGE_DET_PIN (P10D)
#endif

// enum
// {
//     MODE_FADE = 0,  // 整体呼吸
//     MODE_ALWAYS_ON, // 整体常亮
//     MODE_FLASH,     // 整体闪烁
//     MODE_TRIGER,    // 震动闪灯

//     MODE_ALWAYS_ON_LIGHT1,       // 灯1常亮
//     MODE_ALWAYS_ON_LIGHT2,       // 灯2常亮
//     MODE_FLASH_LIGHT1,           // 灯1闪烁
//     MODE_FLASH_LIGHT2,           // 灯2闪烁
//     MODE_FLASH_LIGHT1_OR_LIGHT2, // 交替闪烁
//     MODE_FADE_LIGHT1,            // 灯1渐变
//     MODE_FADE_LIGHT2,            // 灯2渐变
//     MODE_FADE_IN_TURN,           // 同时交替渐变

//     MODE_MAX,
// };

// 定义灯光的模式
enum
{
    LIGHT_MODE_NONE = 0,
    LIGHT_MODE_ALWAYS_ON, // 常亮
    LIGHT_MODE_FLASH,     // 爆闪，200ms亮，200ms灭
    LIGHT_MODE_FLASH_2,   // 爆闪，100ms亮，100ms灭
    LIGHT_MODE_TRIGGER,   // 震动检测模式，
};

volatile uint8_t key_sleep_count;

volatile uint8_t key_filtter;
volatile uint8_t key_scan_count;
volatile uint8_t key_level;
volatile uint8_t key_press_10ms_count;

volatile uint8_t light_mode;
// volatile uint8_t light_mode_flash_dir;
// volatile uint16_t light_mode_flash_count;
// volatile uint8_t light_mode_fade_dir;
// volatile uint8_t light_mode_fade_count;
// volatile uint8_t pwm_switch_count;
// volatile uint8_t pwm_switch;
// volatile uint8_t pwm_duty;
// volatile uint8_t pwm2_duty;
// volatile uint8_t rt_trigger;

volatile uint8_t sleep_enalbe;

volatile uint8_t timer0_flag;
// volatile uint8_t timer1_count;

void params_init(void);

// void delay_1ms(void)
// {
//     volatile uint16_t _ms = 443;
//     while (_ms--)
//         ;
// }

// void delay_10ms(void)
// {
//     volatile uint16_t _ms = 4443;
//     while (_ms--)
//         ;
// }

// 前提条件：FCPU = FHOSC / 2
void delay_ms(u16 xms)
{
    while (xms)
    {
        u16 i = 535;
        while (i--)
        {
            Nop();
        }
        xms--; // 把 --操作放在while()判断条件外面，更节省空间

        __asm;
        clrwdt; // 喂狗
        __endasm;
    }
}

/************************************************
;  *    @Function Name       : C_RAM
;  *    @Description         : 初始化RAM
;  *    @IN_Parameter      	 :
;  *    @Return parameter    :
;  ***********************************************/
void C_RAM(void)
{
    __asm;
    movai 0x40;
    movra FSR;
    movai 48;
    movra 0x07;
    decr FSR;
    clrr INDF;
    djzr 0x07;
    goto $ - 3;
    clrr 0x07;
    __endasm;
}

/************************************************
;  *    @Function Name       : IO_Init
;  *    @Description         :
;  *    @IN_Parameter      	 :
;  *    @Return parameter    :
;  ***********************************************/
void IO_Init(void)
{
    // P1 = 0x00;    // 1:input 0:output
    P1 = 0x01 << 2; // P12默认输出高电平，不点亮灯光
    DDR1 = 0x00;    // 1:input 0:output
    PUCON = 0xff;   // 0:Effective 1:invalid
    PDCON = 0xff;   // 0:Effective 1:invalid
    ODCON = 0x00;   // 0:推挽输出  1:开漏输出

    KEY_DETECT_INIT();
#if RT_DETECT_ENABLE
    RT_DETECT_INIT();
#endif
}
/************************************************
;  *    @Function Name       : IO_Config
;  *    @Description         : 普通IO配置
;  *    @IN_Parameter        :
;  *    @Return parameter    :
;  ***********************************************/
void TIMER0_INT_Init(void)
{
    // FCPU 8MHz 到分频器前经过2分频，再到分频器的64分频  -> 理论上 0.000016s（16us）计数一次
    // 但仿真上是 8us计数一次
    // T0CR = 0x05; // 时钟为CPU时钟  预分频器分配给T0而不是WDT  定时器64分频
    T0CR = (0x01 << 2) | (0x01 << 0); // 时钟为CPU时钟  预分频器分配给T0而不是WDT  定时器64分频
    // T0CNT = 161;                      // (256 - 95); 递增计数器，需要手动装载
    T0CNT = 255 - 62; // 递增计数器，需要手动装载
    T0IE = 1;
}
/************************************************
;  *    @Function Name       : IO_Config
;  *    @Description         : 普通IO配置
;  *    @IN_Parameter        :
;  *    @Return parameter    :
;  ***********************************************/
#if 0
void TIMER1_PWM_Init(void)
{
    PWM1CR0 = 0x00; // 允许PWM1A端口输出波形，T1时钟不倍频
    PWM1CR1 = 0x00; // T1PWMA 信号电平不取反

    // 理论上 0.000000125s（0.125us） 计数一次
    T1LOAD = 124; // 124一直减到0，包括0向下溢出
    // T1CNT = 127;
    // T1DATA = 0;
    // T1DATB = 0;
    T1CR = 0xC0; // 使能定时器 Fcpu 1分频
}
#endif

/************************************************
;  *    @Function Name       : Sys_Init
;  *    @Description         : 系统初始化
;  *    @IN_Parameter      	 :
;  *    @Return parameter    :
;  ***********************************************/
void Sys_Init(void)
{
    GIE = 0;
    C_RAM();
    params_init();
    IO_Init();
    TIMER0_INT_Init();
    // TIMER1_PWM_Init();

    // delay_1ms();
    delay_ms(1);
    GIE = 1;
}
//=================================================================================
/**
 * @brief 初始化所有变量，因为复位会将RAM清空，需要在清RAM后重新初始化赋值
 */
void params_init(void)
{
    key_sleep_count = 0;

    key_filtter = 0xFF;
    key_level = 1;
    key_scan_count = 0;
    key_press_10ms_count = 0;

    sleep_enalbe = 1;
    // sleep_enalbe = 0;
    // light_mode = MODE_FADE; // 上电默认模式
    
    // light_mode = LIGHT_MODE_NONE; // 上电默认模式

    light_mode = LIGHT_MODE_ALWAYS_ON;

    // light_mode = MODE_ALWAYS_ON; // 上电默认常亮
}

#if 1
void key_scan(void)
{
    key_scan_count++;
    if (key_scan_count >= 10)
    {
        key_scan_count = 0;

        key_filtter <<= 1;
        if (KEY_DET_PIN)
            key_filtter |= 0x01;

        key_filtter &= KEY_FILTTER_PARAM;
        if (key_filtter == KEY_FILTTER_PARAM)
            key_level = 1;
        else if (key_filtter == 0)
            key_level = 0;

        if (key_level == 0)
        {
            if (key_press_10ms_count <= 200)
                // if (key_press_10ms_count < 200)
                key_press_10ms_count++;
            if (key_press_10ms_count == 200)
            { // 长按关机
                sleep_enalbe = 1;
            }
        }
        else
        {
            if (key_press_10ms_count > 20 && key_press_10ms_count < 200)
            { // 短按切换模式
#if 0
                light_mode++;
                if (light_mode >= MODE_MAX)
                    light_mode = MODE_ALWAYS_ON;

                { // 初始化模式动画计数
                    light_mode_flash_count = 0;
                    light_mode_flash_dir = 0;
                    light_mode_fade_dir = 0;
                    light_mode_fade_count = 0;
                }
#endif

                light_mode++;
                flag_is_enable_light_trigger_mode = 1;
                if (light_mode > LIGHT_MODE_TRIGGER)
                {
                    light_mode = LIGHT_MODE_ALWAYS_ON;
                }
            }
            key_press_10ms_count = 0;
        }
    }
}

void light_mode_handle(void)
{
    if (LIGHT_MODE_ALWAYS_ON == light_mode)
    {
        P12D = 0;
    }
    else if (LIGHT_MODE_FLASH == light_mode)
    {
        // P12D = 1;
    }
    else if (LIGHT_MODE_TRIGGER == light_mode)
    {
        // P12D = 0;
        // delay_ms(200);
        // P12D = 1;
        // delay_ms(200);
    }
}
#endif

#if 0
void light_deal_event(void)
{
    if (pwm_switch == 0)
        OSCILLATION_PIN2 = OSCILLATION_PIN1;
    else
        OSCILLATION_PIN2 = 0;
#ifdef OSCILLATION_PIN3
    if (pwm_switch == 1)
        OSCILLATION_PIN3 = OSCILLATION_PIN1;
    else
        OSCILLATION_PIN3 = 0;
#endif
    OSCILLATION_PIN1 = !OSCILLATION_PIN1;

    if (light_mode == MODE_ALWAYS_ON)
    {
        // pwm_duty = 64;
        pwm_duty = 9; // 测试硬件用，7~8%占空比
    }
    else if (light_mode == MODE_FLASH)
    {
        light_mode_flash_count++;
        if (light_mode_flash_count == 393) // 大概300ms
        {
            light_mode_flash_count = 0;
            light_mode_flash_dir ^= 1;
        }

        if (light_mode_flash_dir)
        {
            pwm_duty = 0;
        }
        else
        {
            pwm_duty = 64;
        }
    }
    else if (light_mode == MODE_FADE)
    {
        light_mode_fade_count++;
        if (light_mode_fade_count > 50)
        {
            light_mode_fade_count = 0;

            if (light_mode_fade_dir == 0)
            {
                if (pwm_duty < 74)
                    pwm_duty++;
                if (pwm_duty >= 74)
                    light_mode_fade_dir = 1;
            }
            else if (light_mode_fade_dir == 1)
            {
                if (pwm_duty > 1)
                    pwm_duty--;
                if (pwm_duty == 1)
                    light_mode_fade_dir = 0;
            }
        }
    }
    else if (light_mode == MODE_TRIGER)
    {
        if (rt_trigger && pwm_duty == 0)
        {
            rt_trigger--;
            pwm_duty = 64;
        }
        else if (pwm_duty > 0)
        {
            if (rt_trigger == 1)
            {
                if (pwm_switch == 0)
                    pwm_duty--;
                else
                {
                    T1DATA = 0;
                    timer1_count = 0;
                    T1IE = 1;
                    return;
                }
            }
            else if (rt_trigger == 0)
            {
                if (pwm_switch == 1)
                    pwm_duty--;
                else
                {
                    T1DATA = 0;
                    timer1_count = 0;
                    T1IE = 1;
                    return;
                }
            }
        }
    }
    else if (light_mode == MODE_ALWAYS_ON_LIGHT1)
    {
        if (pwm_switch == 0)
        {
            pwm_duty = 64;
        }
        else
        {
            pwm_duty = 0;
        }
    }
    else if (light_mode == MODE_ALWAYS_ON_LIGHT2)
    {
        if (pwm_switch == 0)
        {
            pwm_duty = 0;
        }
        else
        {
            pwm_duty = 64;
        }
    }
    else if (light_mode == MODE_FLASH_LIGHT1)
    {
        light_mode_flash_count++;
        if (light_mode_flash_count == 393) // 大概300ms
        {
            light_mode_flash_count = 0;
            light_mode_flash_dir ^= 1;
        }

        if (light_mode_flash_dir || pwm_switch)
        {
            pwm_duty = 0;
        }
        else
        {
            pwm_duty = 64;
        }
    }
    else if (light_mode == MODE_FLASH_LIGHT2)
    {
        light_mode_flash_count++;
        if (light_mode_flash_count == 393) // 大概300ms
        {
            light_mode_flash_count = 0;
            light_mode_flash_dir ^= 1;
        }

        if (light_mode_flash_dir || pwm_switch == 0)
        {
            pwm_duty = 0;
        }
        else
        {
            pwm_duty = 64;
        }
    }
    else if (light_mode == MODE_FLASH_LIGHT1_OR_LIGHT2)
    {
        light_mode_flash_count++;
        if (light_mode_flash_count == 393) // 大概300ms
        {
            light_mode_flash_count = 0;
            light_mode_flash_dir++;
            light_mode_flash_dir &= 0x03;
        }

        if (light_mode_flash_dir == 0)
        {
            if (pwm_switch == 0)
                pwm_duty = 0;
            else
                pwm_duty = 64;
        }
        else
        {
            if (pwm_switch == 0)
                pwm_duty = 64;
            else
                pwm_duty = 0;
        }
    }
    else if (light_mode == MODE_FADE_LIGHT1)
    {
        light_mode_fade_count++;
        if (light_mode_fade_count > 50)
        {
            light_mode_fade_count = 0;

            if (pwm_switch == 1)
            {
                T1DATA = 0;
                timer1_count = 0;
                T1IE = 1;
                return;
            }

            if (light_mode_fade_dir == 0)
            {
                if (pwm_duty < 74)
                    pwm_duty++;
                if (pwm_duty >= 74)
                    light_mode_fade_dir = 1;
            }
            else if (light_mode_fade_dir == 1)
            {
                if (pwm_duty > 1)
                    pwm_duty--;
                if (pwm_duty == 1)
                    light_mode_fade_dir = 0;
            }
        }
    }
    else if (light_mode == MODE_FADE_LIGHT2)
    {
        light_mode_fade_count++;
        if (light_mode_fade_count > 50)
        {
            light_mode_fade_count = 0;

            if (pwm_switch == 0)
            {
                T1DATA = 0;
                timer1_count = 0;
                T1IE = 1;
                return;
            }

            if (light_mode_fade_dir == 0)
            {
                if (pwm_duty < 74)
                    pwm_duty++;
                if (pwm_duty >= 74)
                    light_mode_fade_dir = 1;
            }
            else if (light_mode_fade_dir == 1)
            {
                if (pwm_duty > 1)
                    pwm_duty--;
                if (pwm_duty == 1)
                    light_mode_fade_dir = 0;
            }
        }
    }
    else if (light_mode == MODE_FADE_IN_TURN)
    {
        pwm2_duty = 74 - pwm_duty;

        if (pwm_switch == 0)
        {
            light_mode_fade_count++;
            if (light_mode_fade_count > 50)
            {
                light_mode_fade_count = 0;

                if (light_mode_fade_dir == 0)
                {
                    if (pwm_duty < 74)
                        pwm_duty++;
                    if (pwm_duty >= 74)
                        light_mode_fade_dir = 1;
                }
                else if (light_mode_fade_dir == 1)
                {
                    if (pwm_duty > 1)
                        pwm_duty--;
                    if (pwm_duty == 1)
                        light_mode_fade_dir = 0;
                }
            }
        }
        else
        {
            T1DATA = pwm2_duty;
            timer1_count = 0;
            T1IE = 1;
            return;
        }
    }

    pwm_switch_count++;
    if (pwm_switch_count == 2)
    {
        pwm_switch ^= 1;
        pwm_switch_count = 0;
    }

    T1DATA = pwm_duty;
    timer1_count = 0;
    T1IE = 1; // 开启Timer1计数中断，配合计数28个PWM周期后自动将PWM占空比设置为0
}
#endif

#if 1
#if RT_DETECT_ENABLE
void rt_detect_event(void)
{

    static volatile uint8_t Statue = 0;
    static volatile uint8_t pluse_count = 0;
    static volatile u8 pluse_time = 0;
    static volatile uint16_t switch_mode_count = 0;

    if (switch_mode_count > 0)
    {
        switch_mode_count--;
        if (switch_mode_count == 0)
        {
            if (IS_RT_DETECT())
            { // 切换到充电检测模式
                RT_DETECT_OFF();
                switch_mode_count = 3;
            }
            else
            { // 切换到震动检测模式
                RT_DETECT_ON();
                switch_mode_count = 40;
            }
            return; // 跳过这个周期，让上/下拉电阻有时间稳定电压
        }
    }
    else
    { // 一般不会进入这里，添加防止意外导致无法切换检测模式
        switch_mode_count = 1;
        return;
    }

    // 加入这一块语句会
    // if (0 == flag_is_enable_light_trigger_mode)
    // {
    //     // 不是震动检测模式或震动检测未使能，清0相关变量
    //     pluse_count = 0;
    //     pluse_time = 0;
    // }

    if (IS_RT_DETECT())
    // if (IS_RT_DETECT() && 0 == flag_is_enable_light_trigger_mode) /* 加上 flag_is_enable_light_trigger_mode 判断会导致进入 sleep_enalbe = 1，长按按键开机后马上进入关机 */
    { // 震动检测
        switch (Statue)
        {
        case 0:
            if (!RT_CHARGE_DET_PIN) // 由高电平转为低电平回复常态
            {
                Statue = 1;
            }
            break;

        case 1:
            if (RT_CHARGE_DET_PIN) // 由低电平转为高电平加1
            {
                Statue = 0;
                pluse_count++;
                pluse_time = 0;
            }
            else
            {
                pluse_time++;
                if (pluse_time >= 150)
                {
                    pluse_time = 0;
                    pluse_count = 0;
                }
            }
            break;
        }

        if (pluse_count >= 5) // lmd_buf[sys_ctl.lmd_num-1])//灵敏度调节参数,越少越敏感
        {
            pluse_count = 0;
            pluse_time = 0;

            // 触发
            // rt_trigger = 2;

            // light_mode = LIGHT_MODE_TRIGGER;
            flag_is_enable_light_trigger_mode = 1;
        }
    }
    else
    {
        // 充电检测
        if (RT_CHARGE_DET_PIN)
            sleep_enalbe = 1;
    }
}
#endif

#endif

void main(void)
{
    Sys_Init();

    // light_mode = LIGHT_MODE_TRIGGER;
    // flag_is_enable_light_trigger_mode = 1;

    while (1)
    {
        // __asm;
        // clrwdt;
        // __endasm;

#if 1

        if (timer0_flag)
        {
            timer0_flag = 0;
            // light_deal_event();
            light_mode_handle();
            key_scan();
#if RT_DETECT_ENABLE
            rt_detect_event();
#endif
        }

        if (sleep_enalbe)
        {
            // sleep
            // T1IE = 0;
            // T1DATA = 0;
            // delay_1ms();
            // T1EN = 0;
            GIE = 0;

            P12D = 1; // 关闭灯光

            OSCILLATION_PIN1 = 0;
            OSCILLATION_PIN2 = 0;
#ifdef OSCILLATION_PIN3
            OSCILLATION_PIN3 = 0;
#endif
            // PWM_CONTROLLER1 = 0; //

            key_sleep_count = 0;
            while (1)
            { // 等待松手
                // __asm;
                // clrwdt;
                // __endasm;

                // delay_1ms();
                delay_ms(1);
                if (KEY_DET_PIN)
                {
                    key_sleep_count++;
                    if (key_sleep_count > 100)
                    {
                        break;
                    }
                }
                else
                {
                    key_sleep_count = 0;
                }
            }

            key_sleep_count = 1;
            while (1)
            {
                // __asm;
                // clrwdt;
                // __endasm;

                if (KEY_DET_PIN == 0)
                {
                    // 长按两秒后开机并亮灯
                    // delay_10ms();
                    delay_ms(10);
                    if (key_sleep_count >= 200)
                    {
                        KBIF = 0;
                        KBIE = 1;
                        key_press_10ms_count = 200;
                        key_filtter = 0x00;
                        key_level = 0;
                        break;
                    }
                    key_sleep_count++;
                }
                else
                {
                    if (key_sleep_count > 0)
                    {
                        key_sleep_count--;
                        // delay_1ms();
                        delay_ms(1);
                        if (key_sleep_count == 0)
                        {
#if RT_DETECT_ENABLE
                            RT_DETECT_UNINIT();
#endif
                            KEY_KB_INT_ENABLE = 1;
                            KBIF = 0;
                            KBIE = 1;

                            Nop();
                            Nop();
                            Stop();
                            Nop();
                            Nop();
                            key_sleep_count = 1;
                        }
                    }
                }
            }

            // light_mode = MODE_FADE;    // 开机默认模式
            // light_mode = MODE_ALWAYS_ON; // 开机默认模式
            sleep_enalbe = 0;
            flag_is_enable_light_trigger_mode = 1;

#if RT_DETECT_ENABLE
            RT_DETECT_ON();
#endif
            KEY_KB_INT_ENABLE = 0;
            KBIE = 0;
            KBIF = 0;
            // T1EN = 1;
            GIE = 1;
        }

#endif
    }
}

/************************************************
;  *    @Function Name       : Interrupt
;  *    @Description         : The interrupt function
;  *    @IN_Parameter          	 :
;  *    @Return parameter      	:
;  ***********************************************/
void int_isr(void) __interrupt
{
    __asm;
    movra _abuf;
    swapar _STATUS;
    movra _statusbuf;
    __endasm;

    if ((T0IF) && (T0IE))
    {
        T0IF = 0;
        // T0CNT = 161; // (256 - 95);
        T0CNT = 255 - 62; // 递增计数器，需要手动装载 (约500us)

        // P12D = ~P12D; // 测试中断时间

        if (LIGHT_MODE_FLASH == light_mode)
        {
            static u16 cnt = 0;
            cnt++;
            // if (cnt < 200)
            if (cnt < 400)
            {
                P12D = 0;
            }
            // else if (cnt < 400)
            else if (cnt < 800)
            {
                P12D = 1;
            }
            else
            {
                cnt = 0;
            }
        }
        else if (LIGHT_MODE_FLASH_2 == light_mode)
        {
            static u16 cnt = 0;
            cnt++;
            if (cnt < 200)
            {
                P12D = 0;
            }
            else if (cnt < 400)
            {
                P12D = 1;
            }
            else
            {
                cnt = 0;
            }
        }
        else if (flag_is_enable_light_trigger_mode && LIGHT_MODE_TRIGGER == light_mode)
        {
            /*
                如果在震动检测模式，并且检测到了震动
            */

            // static u16 cnt = 0;
            static u16 loop_cnt = 0;

            // cnt++;
            loop_cnt++;
            // if (cnt < 200)
            // // if (cnt < 400)
            // {
            //     P12D = 0;
            // }
            // else if (cnt < 400)
            // // else if (cnt < 800)
            // {
            //     P12D = 1;
            // }
            // else
            // {
            //     cnt = 0;
            // }

            P12D = 0;

            if (loop_cnt >= 400)
            {
                P12D = 1; // 结束后关闭灯光
                // cnt = 0;
                loop_cnt = 0;
                flag_is_enable_light_trigger_mode = 0;
            }

            // if (loop_cnt >= 8)
            // {
            //     cnt = 0;
            //     loop_cnt = 0;
            //     flag_is_enable_light_trigger_mode = 0;
            // }
        }

        timer0_flag = 1;
    }

    // if ((T1IF) && (T1IE))
    // {
    //     T1IF = 0;

    //     // timer1_count++;
    //     // if (timer1_count > 28)
    //     // {
    //     //     T1IE = 0;
    //     //     T1DATA = 0;
    //     // }
    // }

    // if ((CMPIE) && (CMPIF))
    // {
    //     CMPIF = 0;
    // }

    __asm;
    swapar _statusbuf;
    movra _STATUS;
    swapr _abuf;
    swapar _abuf;
    __endasm;
}

/**************************** end of file *********************************************/
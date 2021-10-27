#ifndef _IIS2DH_REGISTERS_H
#define _IIS2DH_REGISTERS_H

//----------------------------------------------------------------------
//
//   Project:  Kionix Driver Work v2 (Zephyr RTOS sensor driver)
//
//  Repo URL:  https://github.com/tedhavelka/kionix-driver-demo
//
//      File:  iis2dh-registers.c
//
//----------------------------------------------------------------------


//----------------------------------------------------------------------
// - SECTION - IIS2DH configuration register defines
//----------------------------------------------------------------------

#if 0 // may not need these:
#endif
static uint8_t iis2dh_ctrl_reg1 = 0;       // 0x20
static uint8_t iis2dh_ctrl_reg2 = 0;       // 0x21
static uint8_t iis2dh_ctrl_reg3 = 0;       // 0x22
static uint8_t iis2dh_ctrl_reg4 = 0;       // 0x23
static uint8_t iis2dh_ctrl_reg5 = 0;       // 0x24
static uint8_t iis2dh_acc_status = 0;      // 0x27
static uint8_t iis2dh_fifo_ctrl_reg = 0;   // 0x2F


//
// IIS2DH_CTRL_REG1     (0x20)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// iis2dh.pdf page 33:
// [  ODR3  |   ODR2  |  ODR1  |  ODR1  |   LPEN   |    ZEN   |    YEN   |    XEN   ]  <-- IIS2DH_CTRL_REG1
//
//  +  ODR    Output Data Rate
//  +  LPEN   Low Power Enable
//  +  ZEN    Z-axis Enable, Y-axis Enable, X-axis Enable

#if 0
#define ODR_0_POWERED_DOWN                      ( 0 << 4 )
#define ODR_1_HZ                                ( 1 << 4 )
#define ODR_10_HZ                               ( 2 << 4 )
#define ODR_25_HZ                               ( 3 << 4 )
#define ODR_50_HZ                               ( 4 << 4 )
#define ODR_100_HZ                              ( 5 << 4 )
#define ODR_200_HZ                              ( 6 << 4 )
#define ODR_400_HZ                              ( 7 << 4 )

#define ODR_5p376_HZ_IN_LOW_POWER_MODE          ( 9 << 4 )
#else
enum iis2dh_output_data_rates_e
{
    ODR_0_POWERED_DOWN            = ( 0 << 4 ),
    ODR_1_HZ                      = ( 1 << 4 ),
    ODR_10_HZ                     = ( 2 << 4 ),
    ODR_25_HZ                     = ( 3 << 4 ),
    ODR_50_HZ                     = ( 4 << 4 ),
    ODR_100_HZ                    = ( 5 << 4 ),
    ODR_200_HZ                    = ( 6 << 4 ),
    ODR_400_HZ                    = ( 7 << 4 ),
    ODR_1620_HZ_IN_LOW_POWER_MODE = ( 8 << 4 ),
    ODR_5376_HZ_IN_LOW_POWER_MODE = ( 9 << 4 )   // <-- note 1344 Hz in high resolution and normal power modes
};
#endif

// Lower four bits of control register 1:
#define LOW_POWER_DISABLE                       ( 0 << 3 )
#define LOW_POWER_ENABLE                        ( 1 << 3 )
// Here select a power mode, this symbol permits pre-processor directives
// to assure High Resolution bit settable only when low power mode
// disabled:
#define POWER_MODE LOW_POWER_ENABLE

#define AXIS_Z_ENABLE                           ( 1 << 2 )
#define AXIS_Y_ENABLE                           ( 1 << 1 )
#define AXIS_X_ENABLE                           ( 1 << 0 )


//
// IIS2DH_CTRL_REG2     (0x21)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// HPF - HIGH PASS FILTER MODES (bits 7:6 in this iis2dh register)
#define HPF_NORMAL_RESET_BY_REG_0X26_READ       ( 0 << 6 )
#define HPF_REFERENCE_SIGNAL_FOR_FILTERING      ( 1 << 6 )
#define HPF_NORMAL_MODE                         ( 2 << 6 )
#define HPF_AUTORESET_ON_INTERRUPT_EVENT        ( 3 << 6 )
// HPFC - HIGH PASS FILTER CUT-OFF (bits 5:4, see iis2dh.pdf table 32 for cut-off frequencies)
#define HPFC_HIGH                               ( 0 << 4 )
#define HPFC_MIDDLE_HIGH                        ( 1 << 4 )
#define HPFC_MIDDLE_LOW                         ( 2 << 4 )
#define HPFC_LOW                                ( 3 << 4 )
// FDS - FILTER DATA SELECTION
#define FDS_FILTER_DATA_SELECTION_ENABLE        ( 1 << 3 )
#define FDS_FILTER_DATA_SELECTION_DISABLE            ( 0 )
// HPCLICK - HIGH PASS FILTER EN FOR CLICK FUNCTION
#define HIGH_PASS_EN_FOR_CLICK_FUNCTION         ( 1 << 2 )
#define HIGH_PASS_CLICK_DISABLE                      ( 0 )
// HIGH PASS FILTER EN FOR AOI FUNCTION ON INTERRUPT 2
#define HIGH_PASS_EN_FOR_AOI_FN_ON_INT2         ( 1 << 1 ) 
#define HIGH_PASS_AOI_INT2_DISABLE                   ( 0 )
// HIGH PASS FILTER EN FOR AOI FUNCTION ON INTERRUPT 1
#define HIGH_PASS_EN_FOR_AOI_FN_ON_INT1         ( 1 << 1 )
#define HIGH_PASS_AOI_INT1_DISABLE                   ( 0 )


//
// IIS2DH_CTRL_REG3     (0x22)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define FIFO_WATERMARK_INTERRUPT_ON_INT1_ENABLE ( 1 << 1 )
#define FIFO_OVERRUN_INTERRUPT_ON_INT1_ENABLE   ( 1 << 0 )


//
// IIS2DH_CTRL_REG4     (0x23)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Details of IIS2DH register 0x24 in iis2dh.pdf page 36 of 49, DocID027668 Rev 2:
// [   BDU  |   BLE   |   FS1  |   FS0  |    HR    |    ST1   |    ST0   |    SIM   ]  <-- IIS2DH_CTRL_REG4

#define BLOCK_DATA_UPDATE_NON_CONTINUOUS        ( 1 << 7 )
#define BLE_LSB_IN_LOWER_BYTE_IN_HIGH_RES_MODE  ( 0 << 6 )
#define BLE_MSB_IN_LOWER_BYTE_IN_HIGH_RES_MODE  ( 1 << 6 )
#define ACC_FULL_SCALE_2G                       ( 0 << 4 ) 
#define ACC_FULL_SCALE_4G                       ( 1 << 4 )
#define ACC_FULL_SCALE_8G                       ( 2 << 4 )
#define ACC_FULL_SCALE_16G                      ( 3 << 4 )

// page 16 of 49:  low power means 8-bit readings, normal power means 10-bit readings, high-resolution means 12-bit readings
//#define ACC_OPERATING_MODE_NORMAL                    ( 0 )
//#define ACC_OPERATING_MODE_HIGH_RES                  ( 1 )
//#define ACC_OPERATING_MODE_LOW_POWER                 ( 2 )
// Assure High Resolution mode disabled when per table 9, page 16 Low Power mode ENabled 
#if POWER_MODE == LOW_POWER_ENABLE
#define ACC_OPERATING_MODE_HIGH_RES                  ( 0 )
#else
#define ACC_OPERATING_MODE_HIGH_RES                  ( 1 )
#endif

// Self test
#define IIS2DH_SELF_TEST_NORMAL_MODE            ( 0 << 0 )
#define IIS2DH_SELF_TEST_0                      ( 1 << 0 )
#define IIS2DH_SELF_TEST_1                      ( 2 << 0 )
// SIM Serial Interface Mode
#define SPI_MODE_FOUR_WIRE                           ( 0 )
#define SPI_MODE_THREE_WIRE                          ( 1 )


//
// IIS2DH_CTRL_REG5     (0x24)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// [ REBOOT | FIFO_EN |   --   |   --   | LIR_INT1 | D4D_INT1 | LIR_INT2 | D4D_INT2 ]  <-- IIS2DH_CTRL_REG5

#define FIFO_ENABLE                             ( 1 << 6 )


//
// IIS2DH_STATUS_REG (0x27)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#define IIS2DH_XYZ_OVERRUN_FLAG                 ( 1 << 7 )
#define IIS2DH_Z_OVERRUN_FLAG                   ( 1 << 6 )
#define IIS2DH_Y_OVERRUN_FLAG                   ( 1 << 5 )
#define IIS2DH_X_OVERRUN_FLAG                   ( 1 << 4 )
#define IIS2DH_XYZ_DATA_AVAILABLE_FLAG          ( 1 << 3 )
#define IIS2DH_Z_DATA_AVAILABLE_FLAG            ( 1 << 2 )
#define IIS2DH_Y_DATA_AVAILABLE_FLAG            ( 1 << 1 )
#define IIS2DH_X_DATA_AVAILABLE_FLAG            ( 1 << 0 )


//
// IIS2DH_FIFO_CTRL_REG (0x2E)
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// [   FM1  |   FM0   |   TR   |  FTH4  |   FTH3   |   FTH2   |   FTH1   |   FTH0   ]  <-- IIS2DH_FIFO_CTRL_REG
//
//  + FM[1:0]    FIFO Mode selection - 00 Bypass, 01 FIFO, 10 Stream, 11 Stream-to-FIFO
//  + TR         Trigger selection   - 0 trigger on interrupt 1, 1 trigger on interrupt 2
//  + FTH[4:0]   ???

// FIFO MODES:
#define FIFO_MODE_BYPASS                        ( 0 << 6 )
#define FIFO_MODE_FIFO                          ( 1 << 6 )
#define FIFO_MODE_STREAM                        ( 2 << 6 )
#define FIFO_MODE_STREAM_TO_FIFO                ( 3 << 6 )
// FIFO TRIGGER INTERRUPT SELECTION:
#define FIFO_TRIGGER_ON_INT_1                   ( 0 << 5 )
#define FIFO_TRIGGER_ON_INT_2                   ( 1 << 5 )
// FIFO TRIGGER THRESHHOLD IN BITS [4:0], NOT REALLY EXPLAINED IN iis2dh.pdf - TMH
// (Can represent values from 0 to 31, number of elements the FIFO holds)
#define FIFO_TRIGGER_THRESHHOLD                      ( 0 )
 

// IIS2DH_FIFO_SRC_REG (0x2F)
#define FIFO_SRC_FSS_MASK                         ( 0x1F )
#define FIFO_SOURCE_OVERRUN                     ( 1 << 6 )



#endif // _IIS2DH_REGISTERS_H

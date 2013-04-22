#ifndef _IPMI_KCS_LIB_H
#define _IPMI_KCS_LIB_H

#define BASE_ADDRESS  0xCA2
#define KCS_DATA      (BASE_ADDRESS + 0x00)
#define KCS_CMD       (BASE_ADDRESS + 0x01)

#define BMC_KCS_RETRY_COUNT   1000
#define BMC_KCS_RETRY_PERIOD  5000

#define ONE       1
#define ZERO      0

#define OBF_BIT   BIT0
#define IBF_BIT   BIT1
#define SMS_ATN   BIT2
#define CD_BIT    BIT3
#define OEM1_BIT  BIT4
#define OEM2_BIT  BIT5
#define S0_BIT    BIT6
#define S1_BIT    BIT7

#define READ_OP   0x01
#define WRITE_OP  0x10

#define GET_STATUS  0x60
#define WRITE_START 0x61
#define WRITE_END   0x62
#define READ        0x68

typedef enum{
  KcsIdleState,
  KcsReadState,
  KcsWriteState,
  KcsErrorState
} KCS_STATE;

typedef struct{
  UINT8 Obf:    1;
  UINT8 Ibf:    1;
  UINT8 Smsatn: 1;
  UINT8 Cd:     1;
  UINT8 Oem1:   1;
  UINT8 Oem2:   1;
  UINT8 State:  2;
} KCS_STATUS;

#endif
#include <Uefi.h>
#include <Base.h>

#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Library/IoLib.h>

#include "IpmiKcsLib.h"

EFI_STATUS
KcsIoCtrl(
  UINT16 Register,
  UINT8 Direction,
  UINT8 *Result
  )
{
  if(Result == NULL){
    return EFI_INVALID_PARAMETER;
  }
  if(Direction == READ_OP){
    *Result = IoRead8(Register);
    return EFI_SUCCESS;
  }
  if(Direction == WRITE_OP){
    IoWrite8(Register, *Result);
    return EFI_SUCCESS;
  }
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
CheckStatus(
  UINT8     StatusBit,
  UINT8     StatusValue
  )
{
  UINTN      Retry;
  UINT8      KcsData;
  EFI_STATUS Status;

  Retry = BMC_KCS_RETRY_COUNT;

  while(Retry--){
    Status = KcsIoCtrl(KCS_CMD, READ_OP, &KcsData);
    if ( ((KcsData & StatusBit) && StatusValue) || (!(KcsData & StatusBit) && !StatusValue) ){
      break;
    }
    gBS->Stall(BMC_KCS_RETRY_PERIOD);
    if(Retry == 0){
      Print(L"Device Error, Wait TimeOut!\n");
      return EFI_TIMEOUT;
    }
  }
  return EFI_SUCCESS;
}

EFI_STATUS
CheckState(
  KCS_STATE   State,
  BOOLEAN     *Idle
  )
{
  KCS_STATUS  KcsStatus;
  EFI_STATUS  Status;

  *Idle = FALSE;
  Status = KcsIoCtrl(KCS_CMD, READ_OP, (UINT8 *)&KcsStatus);
  if(KcsStatus.State == State){
    return EFI_SUCCESS;
  }
  if(KcsStatus.State == KcsIdleState){
    *Idle = TRUE;
  }
  return EFI_DEVICE_ERROR;
}

EFI_STATUS
Error_Exit(
  VOID
  )
{
  UINTN      Count;
  EFI_STATUS Status;
  UINT8      KcsData;
  BOOLEAN    Idle;

  for(Count = 0; Count <= BMC_KCS_RETRY_COUNT; Count++){
    //
    //  Check IBF Zero
    //
    Status = CheckStatus(IBF_BIT, ZERO);
    if(EFI_ERROR(Status)){
      return Status;
    }
    //
    //  Send GET_STATUS to CMD
    //
    KcsData = GET_STATUS;
    Status = KcsIoCtrl(KCS_CMD, WRITE_OP, &KcsData);
    //
    //  check IBF zero
    //
    Status = CheckStatus(IBF_BIT, ZERO);
    if(EFI_ERROR(Status)){
      return Status;
    }
    //
    //  clear OBF
    //
    Status = KcsIoCtrl(KCS_DATA, READ_OP, &KcsData);
    //
    //  Send 0x00 to DATA Register
    //
    KcsData = 0x00;
    Status = KcsIoCtrl(KCS_DATA, WRITE_OP, &KcsData);
    //
    //  check IBF = 0
    //
    Status = CheckStatus(IBF_BIT, ZERO);
    if(EFI_ERROR(Status)){
      return Status;
    }
    Status = CheckState(KcsReadState, &Idle);
    if(EFI_ERROR(Status)){
      continue;
    }
    //
    //  check OBF = 1
    //
    Status = CheckStatus(OBF_BIT, ONE);
    if(EFI_ERROR(Status)){
      return Status;
    }
    //
    //  Read error Status code byte from DATA_OUT
    //
    Status = KcsIoCtrl(KCS_DATA, READ_OP, &KcsData);
    //
    //  Send READ Command.
    //
    KcsData = READ;
    Status = KcsIoCtrl(KCS_DATA, WRITE_OP, &KcsData);
    //
    //  check IBF = 0
    //
    Status = CheckStatus(IBF_BIT, ZERO);
    if(EFI_ERROR(Status)){
      return Status;
    }
    //
    //  check KcsIdleState, if not ,contiune next cycle.
    //
    Status = CheckState(KcsIdleState, &Idle);
    if(EFI_ERROR(Status)){
      continue;
    }
    //
    //  Check OBF = 1, and clear OBF
    //
    Status = CheckStatus(OBF_BIT, ONE);
    if(EFI_ERROR(Status)){
      return Status;
    }
    Status = KcsIoCtrl(KCS_DATA, READ_OP, &KcsData);
    return EFI_DEVICE_ERROR;
  }
  return EFI_DEVICE_ERROR;

}

EFI_STATUS
ReceiveFromKcs(
  UINT8   *Response,
  UINTN   *Size
  )
{
  BOOLEAN    Idle;
  EFI_STATUS Status;
  UINTN       Index;
  UINT8       KcsData;

  for(Index = 0; Index <= 0xFF; Index++){
    //
    //  First check IBF, for BMC has data to send.
    //
    Status = CheckStatus(IBF_BIT, ZERO);
    if(EFI_ERROR(Status)){
      return Status;
    }
    //
    //  Check if State is READ_STATE or IDLE_STATE, else will return Error_Exit.
    //
    Status = CheckState(KcsReadState, &Idle);
    if(EFI_ERROR(Status)){
      if(!Idle){
        return Error_Exit();
      }
      Status = CheckStatus(OBF_BIT, ONE);
      if(EFI_ERROR(Status)){
        return Status;
      }
      //
      //  Read Dummy data from BMC and finish receive phase.
      //
      Status = KcsIoCtrl(KCS_DATA, READ_OP, &KcsData);

      return EFI_SUCCESS;
    }
    //
    //  wait for OBF == 1
    //
    Status = CheckStatus(OBF_BIT, ONE);
    if(EFI_ERROR(Status)){
      return Status;
    }
    //
    //  Receive data.
    //
    Status = KcsIoCtrl(KCS_DATA, READ_OP, &KcsData);
    Response[Index] = KcsData;
    //
    //  Send READ command
    //
    KcsData = READ;
    Status = KcsIoCtrl(KCS_DATA, WRITE_OP, &KcsData);
    *Size = Index + 1;
  }
  Print(L"Error: Receive Data too large. more than 0xFF will be ignore.\n");
  return EFI_BUFFER_TOO_SMALL;
}

EFI_STATUS
SendToKcs(
  UINT8   *Request,
  UINTN   Size
  )
{
  UINT8      KcsData;
  BOOLEAN    Idle;
  UINTN      Index;
  EFI_STATUS Status;
  // 
  //  Ensure previous operation has finished.
  //
  Status = CheckStatus(IBF_BIT,ZERO);
  if(EFI_ERROR(Status)){
    return Status;
  }
  //
  //  Clear OBF
  //
  Status = KcsIoCtrl(KCS_DATA, WRITE_OP, &KcsData);
  //
  //  Write WR_START command to CMD
  //
  KcsData = WRITE_START;
  Status = KcsIoCtrl(KCS_CMD, WRITE_OP, &KcsData);
  //
  //  Wait for IBF
  //
  Status = CheckStatus(IBF_BIT,ZERO);
  if(EFI_ERROR(Status)){
    return Status;
  }
  //
  //  Check WRITE_STATE
  //
  Status = CheckState(KcsWriteState, &Idle);
  if(EFI_ERROR(Status)){
    return Error_Exit();
  }
  //
  //  Clear OBF
  //
  Status = KcsIoCtrl(KCS_DATA, READ_OP, &KcsData);
  for(Index = 0; Index < Size - 1; Index++){
    KcsData = Request[Index];
    Status = KcsIoCtrl(KCS_DATA, WRITE_OP, &KcsData);
    //
    //  Wait for IBF Zero.
    //
    Status = CheckStatus(IBF_BIT, ZERO);
    if(EFI_ERROR(Status)){
      return Status;
    }
    //
    //  Check WRITE_STATE
    //
    Status = CheckState(KcsWriteState, &Idle);
    if(EFI_ERROR(Status)){
      return Error_Exit();
    }
    //
    //  Clear OBF
    //
    Status = KcsIoCtrl(KCS_DATA, WRITE_OP, &KcsData);
  }
  //
  //  Last Byte to send, send WRITE_END command first.
  //
  KcsData = WRITE_END;
  Status = KcsIoCtrl(KCS_CMD, WRITE_OP, &KcsData);
  //
  //  Check IBF Zero.
  //
  Status = CheckStatus(IBF_BIT, ZERO);
  if(EFI_ERROR(Status)){
    return Status;
  }
  //
  //  Check WRITE_STATE
  //
  Status = CheckState(KcsWriteState, &Idle);
  if(EFI_ERROR(Status)){
    return Error_Exit();
  }
  //
  //  Clear OBF
  //
  Status = KcsIoCtrl(KCS_DATA, READ_OP, &KcsData);
  //
  //  Send last byte
  //
  Index = Size - 1;
  KcsData = Request[Index];
  Status = KcsIoCtrl(KCS_DATA, WRITE_OP, &KcsData);

  return EFI_SUCCESS;
}

EFI_STATUS
KcsTransfer(
  UINT8    *Request,
  UINT8    *Response,
  UINTN    *Size
  )
{
  EFI_STATUS Status;

  Status = SendToKcs(Request, *Size);
  if(EFI_ERROR(Status)){
    Print(L"Error: Send Data to BMC Failed\n");
    return Status;
  }

  Status = ReceiveFromKcs(Response, Size);
  if(EFI_ERROR(Status)){
    Print(L"Error: Receive Data From BMC Failed\n");
    return Status;
  }

  return EFI_SUCCESS;
}
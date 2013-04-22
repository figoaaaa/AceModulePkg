#include <Uefi.h>
#include <Base.h>

#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/IpmiLib.h>

#include "IpmiTool.h"

CONVERT_TABLE ConvertTable[CONVERTTABLESIZE] = {
  {0x0030, 0x00},       {0x0031, 0x01},
  {0x0032, 0x02},       {0x0033, 0x03},
  {0x0034, 0x04},       {0x0035, 0x05},
  {0x0036, 0x06},       {0x0037, 0x07},
  {0x0038, 0x08},       {0x0039, 0x09},
  
  {0x0041, 0x0A},       {0x0042, 0x0B},
  {0x0043, 0x0C},       {0x0044, 0x0D},
  {0x0045, 0x0E},       {0x0046, 0x0F},

  {0x0061, 0x0A},       {0x0062, 0x0B},
  {0x0063, 0x0C},       {0x0064, 0x0D},
  {0x0065, 0x0E},       {0x0066, 0x0F},
};

EFI_STATUS
GetHexValue(
  UINT16  Data16,
  UINT8   *Data
  )
{
  UINTN   Index;
  for(Index = 0; Index < CONVERTTABLESIZE; Index++){
    if(Data16 == ConvertTable[Index].Data16){
      *Data = ConvertTable[Index].Data8;
      return EFI_SUCCESS;
    }
  }
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
ConvertToBuffer(
  UINT16  *String,
  UINT8   *Data
  )
{
  UINT8      TempData1;
  UINT8      TempData2;
  EFI_STATUS Status;

  if(*(String+1) == '\0'){
    Status = GetHexValue(*String, Data);
    if(EFI_ERROR(Status)){
      return EFI_INVALID_PARAMETER;
    }
    return EFI_SUCCESS;
  }

  if(EFI_ERROR(GetHexValue(*String, &TempData1)) || EFI_ERROR(GetHexValue((*(String+1)), &TempData2))){
    return EFI_INVALID_PARAMETER;
  }
  *Data = (TempData1 << 0x04 | TempData2);

  return EFI_SUCCESS;
}



EFI_STATUS
FormlizeParameters(
  UINTN  Argc,
  UINT16 **Argv,
  UINT8  *CmdBuffer
  )
{
  UINTN         ArgcIndex;
  UINT16        *String;
  UINT8         Data;
  EFI_STATUS    Status;

  for(ArgcIndex = 1; ArgcIndex < Argc; ArgcIndex++){ 
    String = Argv[ArgcIndex];
    Status = ConvertToBuffer(String, &Data);
    if(EFI_ERROR(Status)){
      return EFI_INVALID_PARAMETER;
    }
    CmdBuffer[ArgcIndex-1] = Data;
  }
  return EFI_SUCCESS;
}

VOID
DisplayWelcome(
  VOID
  )
{
  Print(L"\n");
  Print(L"IpmiTool.efi    Version 0.1    Author: Ace Tsang\n");
  Print(L"Only implement a KCS interface with default IO Port 0xCA2/0xCA3\n");
  Print(L"Feedbacks are welcome. you can contact me with Email:Ace.Tsang@iCloud.com\n");
  Print(L"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  Print(L"Usage: IpmiTool.efi NetFnLUN CMD Data1 Data2 ... DataN\n");
  Print(L"       NetFunLUN    Command NetFun(Hight 6 bits) and LUN(Low 2 bits)\n");
  Print(L"       CMD          Command to perform\n");
  Print(L"       Data1-N      Data you want to sent, max 0xff(include NetFunLUN and Cmd)\n");
  Print(L"\n");
}

INTN
EFIAPI
ShellAppMain(
  IN UINTN        Argc,
  IN CHAR16       **Argv
  )
{
  EFI_STATUS Status;
  UINTN      Index;
  UINTN      Size;

  UINT8 CmdBuffer[BUFFERSIZE];
  UINT8 ResBuffer[BUFFERSIZE];

  Size = Argc - 1;

  if(Argc < 2){
    DisplayWelcome();
    return (0);
  }

  Status = FormlizeParameters(Argc, Argv, CmdBuffer);
  if(EFI_ERROR(Status)){
    Print(L"Error: Parameters Checked Failed\n");
    Print(L"       Only Hex Letters Accepted and less than 0xFF\n");
    return (0);
  }

  Status = KcsTransfer(CmdBuffer, ResBuffer, &Size);
  if(EFI_ERROR(Status)){
    return (0);
  }

  Print(L"Response: ");
  for(Index = 0; Index < Size; Index++){
    Print(L"%X ", ResBuffer[Index]);
  }
  Print(L"\n");

  return (0); 
}

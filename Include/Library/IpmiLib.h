#ifndef __IPMI_LIB_H__
#define __IPMI_LIB_H__

EFI_STATUS
KcsTransfer(
  UINT8   *Request,
  UINT8   *Response,
  UINTN   *Size
  );

#endif
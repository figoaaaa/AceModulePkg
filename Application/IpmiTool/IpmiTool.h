#ifndef __IPMI_TOOL_H__
#define __IPMI_TOOL_H__

#define BUFFERSIZE        0xFF
#define CONVERTTABLESIZE  22

typedef struct _CONVERT_TABLE  CONVERT_TABLE;
struct _CONVERT_TABLE{
  UINT16  Data16;
  UINT8   Data8;
};

#endif
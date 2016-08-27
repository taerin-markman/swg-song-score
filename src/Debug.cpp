#include "stdafx.h"
#include "Debug.h"

#ifdef _DEBUG
void DebugMessage(const char * const file, int line, const char * const text)
{
  char fmt[] = "%s %d: %s\n";
  char i[255];
  _snprintf_s(i, 254, fmt, file, line, text);
  OutputDebugString(i);
}
#endif

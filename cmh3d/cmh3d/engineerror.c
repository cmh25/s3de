#include "engineerror.h"
#include <stdio.h>
#include <stdarg.h>

static char m_message[512];

const char* GetLastEngineError(void) {
  return m_message;
}

void ClearEngineError(void) {
  m_message[0] = '\0';
}

void SetEngineError(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vsnprintf(m_message, sizeof(m_message), fmt, args);
  va_end(args);
}

#pragma once

#include <winternl.h>

extern "C" NTSYSAPI BOOLEAN RtlEqualUnicodeString(PCUNICODE_STRING String1, PCUNICODE_STRING String2, BOOLEAN CaseInSensitive);
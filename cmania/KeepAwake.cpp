#include "KeepAwake.h"
#ifdef _WIN32
#include <windows.h>

REASON_CONTEXT _PowerRequestContext{};
HANDLE _PowerRequest = 0;

void EnableConstantDisplayAndPower(bool enableConstantDisplayAndPower) {
	if (enableConstantDisplayAndPower) {
		_PowerRequestContext.Version = POWER_REQUEST_CONTEXT_VERSION;
		_PowerRequestContext.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
		_PowerRequestContext.Reason.SimpleReasonString = (wchar_t*)L"Cmania game";
		_PowerRequest = PowerCreateRequest(&_PowerRequestContext);
		PowerSetRequest(_PowerRequest, PowerRequestSystemRequired);
		PowerSetRequest(_PowerRequest, PowerRequestDisplayRequired);
	}
	else {
		PowerClearRequest(_PowerRequest, PowerRequestSystemRequired);
		PowerClearRequest(_PowerRequest, PowerRequestDisplayRequired);

		CloseHandle(_PowerRequest);
	}
}
#endif
#ifdef __linux__
void EnableConstantDisplayAndPower(bool enableConstantDisplayAndPower) {
	// TODO:警告
	
}
#endif
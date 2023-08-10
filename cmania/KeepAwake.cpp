#include <windows.h>
#include "KeepAwake.h"
REASON_CONTEXT _PowerRequestContext{};
HANDLE _PowerRequest = 0;

/// <summary>
/// Prevent screensaver, display dimming and power saving. This function wraps PInvokes on Win32 API.
/// </summary>
/// <param name="enableConstantDisplayAndPower">True to get a constant display and power - False to clear the settings</param>
void EnableConstantDisplayAndPower(bool enableConstantDisplayAndPower) {
	if (enableConstantDisplayAndPower) {
		// Set up the diagnostic string
		_PowerRequestContext.Version = POWER_REQUEST_CONTEXT_VERSION;
		_PowerRequestContext.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
		_PowerRequestContext.Reason.SimpleReasonString = (wchar_t*)L"CMania game."; // your reason for changing the power settings;

		// Create the request, get a handle
		_PowerRequest = PowerCreateRequest(&_PowerRequestContext);

		// Set the request
		PowerSetRequest(_PowerRequest, PowerRequestSystemRequired);
		PowerSetRequest(_PowerRequest, PowerRequestDisplayRequired);
	}
	else {
		// Clear the request
		PowerClearRequest(_PowerRequest, PowerRequestSystemRequired);
		PowerClearRequest(_PowerRequest, PowerRequestDisplayRequired);

		CloseHandle(_PowerRequest);
	}
}
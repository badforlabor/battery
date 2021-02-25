/**,
 * Auth :   liubo
 * Date :   2021/02/25 15:39
 * Comment: 参考资料  

	命令控制电源选项
		https://sourcedaddy.com/windows-7/configuring-power-management-settings-using-powercfg-utility.html




 */

#include <Windows.h>
#include <powrprof.h>
#include <iostream>
#include <string>
#include <windows.h>
#include <powrprof.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <locale.h>

#pragma comment(lib,"ntdll.lib")
#pragma comment(lib,"Powrprof.lib")

 // Prints all Power Plans, their subgroups, and settings under each subgroup
void GetAll() {

	GUID* active_power_scheme;
	HRESULT return_value = PowerGetActiveScheme(NULL, &active_power_scheme);

	GUID guid;
	DWORD guidSize = sizeof(guid);
	unsigned int schemeIndex = 0;
	while (PowerEnumerate(0, 0, 0, ACCESS_SCHEME, schemeIndex, (UCHAR*)&guid, &guidSize) == 0) {
		if (guid != *active_power_scheme)
		{
			schemeIndex++;
			continue;
		}

		// buffer contains the GUID
		UCHAR name[2048];
		DWORD nameSize = 2048;
		DWORD error;
		if ((error = PowerReadFriendlyName(0, &guid, &NO_SUBGROUP_GUID, 0, name, &nameSize)) == 0) {
			wprintf(L"%s", name);
			printf("\n");
		}
		else {
			printf("Error: %d\n", error);
		}

		GUID subguid;
		DWORD subguidSize = sizeof(subguid);
		unsigned int subgroupIndex = 0;
		while (PowerEnumerate(0, &guid, 0, ACCESS_SUBGROUP, subgroupIndex, (UCHAR*)&subguid, &subguidSize) == 0) {
			nameSize = 2048;
			if ((error = PowerReadFriendlyName(0, &guid, &subguid, 0, name, &nameSize)) == 0) {
				printf("  ");
				wprintf(L"%s", name);
				printf("\n");
			}
			else {
				printf("Subgroup Error: %d\n", error);
			}

			GUID settingguid;
			DWORD settingguidSize = sizeof(settingguid);
			unsigned int settingIndex = 0;
			while (PowerEnumerate(0, &guid, &subguid, ACCESS_INDIVIDUAL_SETTING, settingIndex, (UCHAR*)&settingguid, &settingguidSize) == 0) {
				nameSize = 2048;
				if ((error = PowerReadFriendlyName(0, &guid, &subguid, &settingguid, name, &nameSize)) == 0) {
					printf("    ");
					wprintf(L"%s", name);
					printf("\n");
				}
				else {
					printf("Setting Error: %d\n", error);
				}

				DWORD DCValueIndex = 0;
				DWORD DCDefaultIndex = 0;
				DWORD ACValueIndex = 0;
				DWORD ACDefaultIndex = 0;
				PowerReadDCValueIndex(0, &guid, &subguid, &settingguid, &DCValueIndex);
				PowerReadDCDefaultIndex(0, &guid, &subguid, &settingguid, &DCDefaultIndex);
				PowerReadACValueIndex(0, &guid, &subguid, &settingguid, &ACValueIndex);
				PowerReadACDefaultIndex(0, &guid, &subguid, &settingguid, &ACDefaultIndex);
				printf("    ");
				printf("  DC Value: %d, AC Value: %d, DC Default: %d, AC Default: %d\n", DCValueIndex, ACValueIndex, DCDefaultIndex, ACDefaultIndex);
				settingIndex++;
			}
			subgroupIndex++;
		}
		schemeIndex++;
	}
}

int main()
{
	setlocale(LC_ALL, "");

	extern int main4();
	main4();
	//return 0;

	GetAll();

	GUID* active_power_scheme;
	HRESULT return_value = PowerGetActiveScheme(NULL, &active_power_scheme);
	if (return_value != 0)
	{
		printf("Error getting active power scheme! (%i)\n", return_value);
		return -1;
	}

	DWORD activeSchemeNameBufferSize;
	if (PowerReadFriendlyName(NULL, active_power_scheme, NULL, NULL, NULL, &activeSchemeNameBufferSize) == ERROR_SUCCESS)
	{
		auto activeSchemeName = new WCHAR[activeSchemeNameBufferSize];
		PowerReadFriendlyName(NULL, active_power_scheme, NULL, NULL, (UCHAR*)activeSchemeName, &activeSchemeNameBufferSize);
		printf(("当前策略:%ls"), activeSchemeName);
		delete[] activeSchemeName;
	}

	DWORD old_ac_throttle_min;
	DWORD old_dc_throttle_min;
	DWORD old_ac_throttle_max;
	DWORD old_dc_throttle_max;
	PowerReadACValueIndex(NULL, active_power_scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_THROTTLE_MINIMUM, &old_ac_throttle_min);
	PowerReadACValueIndex(NULL, active_power_scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_THROTTLE_MAXIMUM, &old_ac_throttle_max);
	PowerReadDCValueIndex(NULL, active_power_scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_THROTTLE_MINIMUM, &old_dc_throttle_min);
	PowerReadDCValueIndex(NULL, active_power_scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_THROTTLE_MAXIMUM, &old_dc_throttle_max);


	printf("\n[*] old_ac_throttle_min -> %d", old_ac_throttle_min);
	printf("\n[*] old_ac_throttle_max -> %d", old_ac_throttle_max);

	LocalFree(active_power_scheme);
	return 0;
}

int main2()
{
	GUID *activeScheme;
	GUID targetScheme;
	time_t currentTime;
	struct tm timeInfo;
	char timeBuffer[80];

	time(&currentTime);
	localtime_s(&timeInfo, &currentTime);
	strftime(timeBuffer, 80, "[%H:%M:%S]: ", &timeInfo);

	std::cout << timeBuffer << "Welcome to the power profile watcher" << std::endl;

	for (int i = 0; ; i++)
	{
		GUID enumerateBuffer;
		DWORD enumerateBufferSize = sizeof(enumerateBuffer);
		DWORD enumerateResult = PowerEnumerate(NULL, NULL, NULL, ACCESS_SCHEME, i, (UCHAR*)&enumerateBuffer, &enumerateBufferSize);
		if (enumerateResult == ERROR_SUCCESS)
		{
			DWORD nameBufferSize;
			if (PowerReadFriendlyName(NULL, &enumerateBuffer, NULL, NULL, NULL, &nameBufferSize) == ERROR_SUCCESS)
			{
				UCHAR *name = new UCHAR[nameBufferSize];
				DWORD nameResult = PowerReadFriendlyName(NULL, &enumerateBuffer, NULL, NULL, name, &nameBufferSize);
				if (nameResult == ERROR_SUCCESS)
				{
					if (wcscmp((WCHAR*)name, L"Ultimate Performance") == 0)
					{
						targetScheme = enumerateBuffer;
						delete[] name;
						break;
					}
				}
				delete[] name;
			}
		}
		else
		{
			break;
		}
	}

	while (true)
	{
		time(&currentTime);
		localtime_s(&timeInfo, &currentTime);
		WCHAR *activeSchemeName = NULL;
		DWORD activeSchemeResult = PowerGetActiveScheme(NULL, &activeScheme);

		if (activeSchemeResult == ERROR_SUCCESS)
		{
			DWORD activeSchemeNameBufferSize;
			if (PowerReadFriendlyName(NULL, activeScheme, NULL, NULL, NULL, &activeSchemeNameBufferSize) == ERROR_SUCCESS)
			{
				activeSchemeName = new WCHAR[activeSchemeNameBufferSize];
				PowerReadFriendlyName(NULL, activeScheme, NULL, NULL, (UCHAR*)activeSchemeName, &activeSchemeNameBufferSize);
			}
			if (*activeScheme != targetScheme)
			{
				if (activeSchemeName != NULL)
				{
					strftime(timeBuffer, 80, "[%H:%M:%S]: ", &timeInfo);
					std::wcout << timeBuffer
						<< L"The active scheme was changed from \""
						<< activeSchemeName
						<< "\" to \"Ultimate Performance\""
						<< std::endl;
				}
				PowerSetActiveScheme(NULL, &targetScheme);
			}
			delete[] activeSchemeName;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(60000));
	}

	return 0;
}

typedef struct _SYSTEM_POWER_INFORMATION {
	ULONG MaxIdlenessAllowed;
	ULONG Idleness;
	LONG TimeRemaining;
	UCHAR CoolingMode;
} SYSTEM_POWER_INFORMATION, *PSYSTEM_POWER_INFORMATION;

int main1() {
	SYSTEM_POWER_INFORMATION pwInfo;
	CallNtPowerInformation(SystemPowerInformation, NULL, NULL, &pwInfo, sizeof(pwInfo));

	printf("-----------------------------\n   Power Policy parser   \n-----------------------------\n\n");

	switch (pwInfo.CoolingMode) {
	case 0:
		printf("[+] Cooling mode active\n");
		break;
	case 2:
		printf("[-] The system does not support CPU throttling, or there is no thermal zone defined in the system.\n");
	case 1:
		printf("[*] The system is currently in Passive cooling mode.\n");
	}
	POWER_POLICY pwPolicy;
	GLOBAL_POWER_POLICY pwGlPolicy;

	GetCurrentPowerPolicies(&pwGlPolicy, &pwPolicy);

	printf("[*] The video turn off after -> %lu seconds or %lu minutes\n", pwPolicy.user.VideoTimeoutAc, (pwPolicy.user.VideoTimeoutAc) / 60);
	printf("[*] Time before power to fixed disk drives is turned off -> %lu seconds or %lu minutes\n", pwPolicy.user.SpindownTimeoutAc, (pwPolicy.user.SpindownTimeoutAc) / 60);
	printf("[*] Fan throttle tolerance -> %uc", pwPolicy.user.FanThrottleToleranceAc);

	printf("\n[*] user.Revision -> %lu", pwPolicy.user.Revision);

	printf("\n[*] user.OptimizeForPowerAc -> %d", pwPolicy.user.OptimizeForPowerAc);
	printf("\n[*] user.ForcedThrottleAc -> %d", pwPolicy.user.ForcedThrottleAc);
	printf("\n[*] user.IdleSensitivityAc -> %d", pwPolicy.user.IdleSensitivityAc);
	printf("\n[*] mach.MinThrottleAc -> %d", pwPolicy.mach.MinThrottleAc);

	return 0;
}
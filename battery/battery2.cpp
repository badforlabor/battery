#pragma comment (lib, "setupapi.lib")

#include <iostream>
#include <string>

#include <Windows.h>
#include <SetupAPI.h>
#include <batclass.h>
#include <devguid.h>

enum class MyBatteryState
{
	Unknown,
	Empty,
	Full,
	Charging,
	Discharging,
};

struct MyBattery
{
	// Current battery state.
	int State = -1;
	// Current (momentary) capacity (in mWh).
	float Current = 0;
	// Last known full capacity (in mWh).
	float Full = 0;
	// Reported design capacity (in mWh).
	float Design = 0;
	// Current (momentary) charge rate (in mW).
	// It is always non-negative, consult .State field to check
	// whether it means charging or discharging.
	float ChargeRate = 0;
	// Current voltage (in V).
	float Voltage = 0;
	// Design voltage (in V).
	// Some systems (e.g. macOS) do not provide a separate
	// value for this. In such cases, or if getting this fails,
	// but getting `Voltage` succeeds, this field will have
	// the same value as `Voltage`, for convenience.
	float DesignVoltage = 0;

	void ShowInfo() const
	{
		printf("Status:%d, quantity:%.2f, %.2f, Percent=%.2f\n", State, 
			Current, Full, 
			Full > 0 ? Current / Full : 0);

		std::string str;
		float timeNum = 0;
		switch (State)
		{
		case (int)MyBatteryState::Discharging:
		{
			if (ChargeRate == 0)
			{
				printf("discharging at zero rate - will never fully discharge\n");
				return;
			}
			str = "remaining";
			timeNum = Current / ChargeRate;
		}
		break;

		case (int)MyBatteryState::Charging:
		{
			if (ChargeRate == 0)
			{
				printf("discharging at zero rate - will never fully discharge\n");
				return;
			}
			str = "until charged";
			timeNum = (Full - Current) / ChargeRate;
		}
		break;

		default:
			return;
		}

		printf("%s, %.2fh\n", str.c_str(), timeNum);
	}
};

static void ShowError()
{
	wchar_t buf[256] = { 0 };
	auto Err = GetLastError();
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, Err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buf, (sizeof(buf) / sizeof(wchar_t)), NULL);
	printf("%d:", Err);
	wprintf(buf);
}

MyBattery getBatteryInfo(int Idx)
{
	MyBattery RetInfo;

	GUID BatteryGuid = { 0x72631e54 ,0x78A4 ,0x11d0 , {0xbc, 0xf7, 0x00, 0xaa, 0x00, 0xb7, 0xb3, 0x2a} };
	auto hdev = SetupDiGetClassDevsW(&BatteryGuid, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	while (1)
	{
		SP_DEVICE_INTERFACE_DATA did;
		did.cbSize = sizeof(did);
		auto Ok = SetupDiEnumDeviceInterfaces(hdev, 0, &BatteryGuid, Idx, &did);

		if (!Ok)
		{
			ShowError();
			printf("err 1\n");
			break;
		}

		DWORD cbRequired = 0;
		Ok = SetupDiGetDeviceInterfaceDetailW(hdev, &did, nullptr, 0, &cbRequired, nullptr);
		//printf("required length=%d, ok=%d\n", cbRequired, Ok);
		if (Ok == FALSE && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			ShowError();
			printf("err 2\n");
			break;
		}
		
		auto buffer = new char[cbRequired];
		auto& d = *reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer);
		d.cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA_W);

		Ok = SetupDiGetDeviceInterfaceDetailW(hdev, &did, &d, cbRequired, &cbRequired, nullptr);
		if (!Ok)
		{
			ShowError();
			printf("err 3\n");
			break;
		}

		HANDLE handle = CreateFileW(d.DevicePath, GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED,
			0);
		//printf("device path:");
		//wprintf(d.DevicePath);
		//printf("\n");
		delete [] buffer;

		while (handle != INVALID_HANDLE_VALUE)
		{
			DWORD dwWait = 0;
			DWORD dwOut = 0;
			BATTERY_QUERY_INFORMATION queryInfo{};
			Ok = DeviceIoControl(handle, IOCTL_BATTERY_QUERY_TAG,
				&dwWait, dwWait,
				&queryInfo.BatteryTag, sizeof(queryInfo.BatteryTag),
				&dwOut, nullptr
			);
			if (!Ok)
			{
				ShowError();
				printf("err 4\n");
				break;
			}
			//printf("action-1, BatteryTag=%d\n", queryInfo.BatteryTag);

			if (queryInfo.BatteryTag == 0)
			{
				printf("battery tag not found\n");
				break;
			}

			BATTERY_INFORMATION batteryInfo{};
			Ok = DeviceIoControl(handle, IOCTL_BATTERY_QUERY_INFORMATION,
				&queryInfo, sizeof(BATTERY_QUERY_INFORMATION),
				&batteryInfo, sizeof(BATTERY_INFORMATION),
				&dwOut, nullptr);
			if (!Ok)
			{
				ShowError();
				printf("err 5\n");
				break;
			}
			//printf("action-2, InformationLevel=%d\n", queryInfo.InformationLevel);

			//printf("action-3, %ld, %ld\n", batteryInfo.FullChargedCapacity, batteryInfo.DesignedCapacity);

			RetInfo.Full = (float)batteryInfo.FullChargedCapacity;
			RetInfo.Design = (float)batteryInfo.DesignedCapacity;

			//printf("action-3-2, %.2f, %.2f\n", RetInfo.Full, RetInfo.Design);

			BATTERY_WAIT_STATUS bws{};
			bws.BatteryTag = queryInfo.BatteryTag;
			BATTERY_STATUS bs;
			Ok = DeviceIoControl(handle, IOCTL_BATTERY_QUERY_STATUS,
				&bws, sizeof(bws),
				&bs, sizeof(bs),
				&dwOut, nullptr);

			if (!Ok)
			{
				ShowError();
				printf("err 6\n");
				break;
			}
			RetInfo.Current = (float)bs.Capacity;
			RetInfo.ChargeRate = (float)bs.Rate;
			RetInfo.Voltage = (float)bs.Voltage;
			RetInfo.Voltage /= 1000.0f;
			RetInfo.State = bs.PowerState;

			//printf("action-4, bs.Voltage=%ld, State=%d\n", bs.Voltage, RetInfo.State);
			//printf("action-4-2, RetInfo.Current=%.2f, Voltage=%.2f\n", RetInfo.Current, RetInfo.Voltage);

			RetInfo.DesignVoltage = RetInfo.Voltage;
			break;
		}

		CloseHandle(handle);
		break;
	}
	   
	SetupDiDestroyDeviceInfoList(hdev);

	return RetInfo;
}

int main4()
{
	for (int i = 0; i < 10; i++)
	{
		const auto& Info = getBatteryInfo(i);
		if (Info.State == -1)
		{
			printf("done\n");
			break;
		}
		Info.ShowInfo();
	}

	return 0;
}
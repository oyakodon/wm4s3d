#include "Wiimote.hpp"

#include <Windows.h>
#include <setupapi.h>
#include <hidsdi.h>
#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <thread>

#include <cstdio>

using namespace std;

#pragma comment(lib,"hid")
#pragma comment(lib, "setupapi")

enum class OutputReport : unsigned char
{
	LEDs = 0x11,
	DataReportType = 0x12,
	IR = 0x13,
	SpeakerEnable = 0x14,
	Status = 0x15,
	WriteMemory = 0x16,
	ReadMemory = 0x17,
	SpeakerData = 0x18,
	SpeakerMute = 0x19,
	IR2 = 0x1a
};

enum class InputReport : unsigned char
{
	Status = 0x20,
	ReadData = 0x21,
	OutputReportAck = 0x22,
	Buttons = 0x30,
	ButtonsAccel = 0x31,
	IRAccel = 0x33,
	ButtonsExtension = 0x34,
	ExtensionAccel = 0x35,
	IRExtensionAccel = 0x37
};

const unsigned char IRblock[8][11] =
{
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x00,0xC0, 0x40,0x00},
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x0C,	0x00,0x00},
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x90,0x00,0x41,	0x40,0x00},
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0x64,0x00,0xfe, 0xfd,0x05},
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0x96,0x00,0xb4, 0xb3,0x04},
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0xaa,0x00,0x64, 0x63,0x03},
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0xc8,0x00,0x36, 0x35,0x03},
	{ 0x02,0x00,0x00,0x71,0x01,0x00,0x72,0x00,0x20, 0x1f,0x03}
};

Wiimote::Wiimote()
{
	static int _num = 0;
	m_wiimoteNum = _num++;

	wiihandle = nullptr;
	buttons.One = buttons.Two = buttons.A = buttons.B =
		buttons.Minus = buttons.Plus = buttons.Home =
		buttons.Up = buttons.Down = buttons.Left = buttons.Right = false;
	LEDs.LED1 = LEDs.LED2 = LEDs.LED3 = LEDs.LED4 = false;
	acc.x = acc.y = acc.z = 0;
	Battery = 0;
	nunchuk.acc.x = nunchuk.acc.y = nunchuk.acc.z = 0;
	nunchuk.C = nunchuk.Z = false;
	nunchuk.Joystick = { -1, -1 };
	extensionType = ExtensionType::None;
	m_rumble = m_isPlaying = false;
}

Wiimote::~Wiimote()
{
	if (wiihandle != nullptr)
	{
		close();
	}
}

void Wiimote::setWriteMethod()
{
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];

	if (!HidD_SetOutputReport(wiihandle, out, output_length))
	{
		writemethod = WriteMethod::Hid;
	}
	else
	{
		writemethod = WriteMethod::File;
		DWORD buff;
		WriteFile(
			(HANDLE)wiihandle,
			out,
			output_length,
			&buff,
			NULL
		);
	}

	HidD_GetInputReport(wiihandle, in, input_length);

	delete[] out;
	delete[] in;

	return;
}

void Wiimote::write(unsigned char* output_report)
{
	if (writemethod == WriteMethod::File)
	{
		DWORD buff;
		WriteFile(
			(HANDLE)wiihandle,
			output_report,
			output_length,
			&buff,
			NULL
		);
	}
	else
	{
		HidD_SetOutputReport(wiihandle, output_report, output_length);
	}
}

void Wiimote::read(unsigned char * input_report)
{
	DWORD buff;
	ReadFile(
		(HANDLE)wiihandle,
		input_report,
		input_length,
		&buff,
		(LPOVERLAPPED)NULL
	);
}

bool Wiimote::open()
{
	return open(false);
}

bool Wiimote::open(bool forceReport)
{
	//GUIDを取得
	GUID HidGuid;
	HidD_GetHidGuid(&HidGuid);

	//全てのHIDの情報セットを取得
	HDEVINFO hDevInfo;
	hDevInfo = SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE);

	//HIDを列挙
	SP_DEVICE_INTERFACE_DATA DevData;
	DevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	int detect_count = 0;
	for (int index = 0; SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &HidGuid, index, &DevData); index++)
	{
		//詳細を取得

		//データサイズの取得
		unsigned long size;
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevData, NULL, 0, &size, NULL);
		//メモリの確保
		PSP_INTERFACE_DEVICE_DETAIL_DATA Detail;
		Detail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)new char[size];
		Detail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

		//確保したメモリ領域に情報を取得
		SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevData, Detail, size, &size, 0);
		
		//とりあえずハンドルを取得
		HANDLE handle = CreateFile(
			Detail->DevicePath,
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			(LPSECURITY_ATTRIBUTES)NULL,
			OPEN_EXISTING,
			0,
			NULL
		);
		delete[] Detail;
		//エラー処理
		if (handle == INVALID_HANDLE_VALUE)
		{
			continue;
		}
		
		//アトリビュートを取得
		HIDD_ATTRIBUTES attr;
		attr.Size = sizeof(attr);
		if (HidD_GetAttributes(handle, &attr))
		{
			//VIDとPIDを比較してWiiリモコンかどうかを調べる
			if (attr.VendorID == VID && attr.ProductID == PID)
			{
				//Capsの取得
				HIDP_CAPS Capabilities;
				PHIDP_PREPARSED_DATA PreparsedData;
				NTSTATUS Result = HIDP_STATUS_INVALID_PREPARSED_DATA;
				
				if (HidD_GetPreparsedData(handle, &PreparsedData))
				{
					Result = HidP_GetCaps(PreparsedData, &Capabilities);
					HidD_FreePreparsedData(PreparsedData);
				}
				//Capsを取得できたか確認
				if (Result == HIDP_STATUS_SUCCESS)
				{
					//接続成功
					if (detect_count == m_wiimoteNum)
					{
						input_length = Capabilities.InputReportByteLength;
						output_length = Capabilities.OutputReportByteLength;
						wiihandle = handle;
						Sleep(100);
						setWriteMethod();
						th = std::thread(updateThread, this, forceReport);
						Sleep(100);
						return true;
					}
					else
					{
						detect_count++;
					}
				}
			}
		}

		//使わなかったハンドルをクローズ
		CloseHandle(handle);
	}

	return false;
}

void Wiimote::close()
{
	if (wiihandle != nullptr)
	{
		m_rumble = false;
		setLED(0x0);
		Sleep(30);
		CloseHandle(wiihandle);
		wiihandle = nullptr;
		th.join();
	}
}

void Wiimote::setRumble(bool on)
{
	m_rumble = on;
	reportLEDs();
}

bool Wiimote::isRumble()
{
	return m_rumble;
}

void Wiimote::rumbleThread(Wiimote* wii, int ms)
{
	wii->setRumble(true);
	Sleep(ms);
	wii->setRumble(false);
}

void Wiimote::rumbleForMiliseconds(int ms)
{
	std::thread(rumbleThread, this, ms).detach();
}

void Wiimote::setLED(bool first, bool second, bool third, bool fourth)
{
	LEDs.LED1 = first;
	LEDs.LED2 = second;
	LEDs.LED3 = third;
	LEDs.LED4 = fourth;

	reportLEDs();
}

void Wiimote::setLED(unsigned char LED)
{
	setLED(
		(LED & 0x01) ? true : false,
		(LED & 0x02) ? true : false,
		(LED & 0x04) ? true : false,
		(LED & 0x08) ? true : false
	);
}

void Wiimote::reportLEDs()
{
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];

	out[0] = (unsigned char)OutputReport::LEDs;
	out[1] =
		(LEDs.LED1 ? 0x10 : 0) |
		(LEDs.LED2 ? 0x20 : 0) |
		(LEDs.LED3 ? 0x40 : 0) |
		(LEDs.LED4 ? 0x80 : 0)
		| (m_rumble ? 0x01 : 0);

	mtx.lock();
	write(out);
	read(in);
	mtx.unlock();

	delete[] out;
	delete[] in;
}

void Wiimote::update()
{
	if (wiihandle == nullptr)
		return;

	unsigned char* in = new unsigned char[input_length];
	unsigned char* out = new unsigned char[output_length];

	mtx.lock();
	read(in);
	mtx.unlock();

	// printf("\rType: %X IR_X:%.3f Y:%.3f S:%d N_Acc_X:%.3f Y:%.3f Z:%.3f BTN_Z:%s C:%s Joy_X:%.3f Y:%.3f", in[0], pointer.getMaximumPos().pos.x, pointer.getMaximumPos().pos.y, pointer.getMaximumPos().size, nunchuk.acc.x, nunchuk.acc.y, nunchuk.acc.z, (nunchuk.Z ? "True" : "False"), (nunchuk.C ? "True" : "False"), nunchuk.Joystick.x, nunchuk.Joystick.y);
	// printf("\rType: %X Acc_X: %.3f Y: %.3f Z: %.3f", in[0], acc.x, acc.y, acc.z);

	switch ((InputReport)in[0])
	{
	case InputReport::Buttons:
		parseButtons(in);
		break;

	case InputReport::ButtonsAccel:
		parseButtons(in);
		parseAccel(in);
		break;

	case InputReport::IRAccel:
		parseButtons(in);
		parseAccel(in);
		parseIR(in);
		break;

	case InputReport::ButtonsExtension:
		parseButtons(in);
		parseExtension(in, 3);
		break;

	case InputReport::ExtensionAccel:
		parseButtons(in);
		parseAccel(in);
		parseExtension(in, 6);
		break;

	case InputReport::IRExtensionAccel:
		parseButtons(in);
		parseAccel(in);
		parseIR(in);
		parseExtension(in, 16);
		break;

	case InputReport::Status:

		parseButtons(in);
		Battery = (((100.0f * 48.0f * (float)((int)in[6] / 48.0f))) / 192.0f);

		LEDs.LED1 = (in[3] & 0x10) != 0;
		LEDs.LED2 = (in[3] & 0x20) != 0;
		LEDs.LED3 = (in[3] & 0x40) != 0;
		LEDs.LED4 = (in[3] & 0x80) != 0;

		if ((in[3] & 0x02) != 0)
		{
			initializeExtension();
		}
		else
		{
			mtx.lock();
			out[0] = (unsigned char)OutputReport::DataReportType;
			out[1] = 0x00;
			out[2] = 0x33;
			write(out);
			read(in);
			mtx.unlock();

			enableIR(IRMode::Extended, SENSITIVITY_MODE);

			extensionType = ExtensionType::None;

		}

		break;

	case InputReport::ReadData:
	case InputReport::OutputReportAck:
		break;

	default:
		string ex = "Unknown report type: " + in[0];
		throw exception(ex.c_str());
		break;
	}

	delete[] in;
	delete[] out;

}

void Wiimote::parseExtension(unsigned char* in, int offset)
{
	switch (extensionType)
	{
	case ExtensionType::Nunchuk:
		Point _joy;
		Acceletion _acc;

		_joy.x = in[offset];
		_joy.y = in[offset + 1];

		_acc.x = in[offset + 2];
		_acc.y = in[offset + 3];
		_acc.z = in[offset + 4];

		nunchuk.C = (in[offset + 5] & 0x02) == 0;
		nunchuk.Z = (in[offset + 5] & 0x01) == 0;

		nunchuk.acc.x = (float)((float)_acc.x - nunchukCalibrationInfo.AccelCalibration.X0) / ((float)nunchukCalibrationInfo.AccelCalibration.XG - nunchukCalibrationInfo.AccelCalibration.X0);
		nunchuk.acc.y = (float)((float)_acc.y - nunchukCalibrationInfo.AccelCalibration.Y0) / ((float)nunchukCalibrationInfo.AccelCalibration.YG - nunchukCalibrationInfo.AccelCalibration.Y0);
		nunchuk.acc.z = (float)((float)_acc.z - nunchukCalibrationInfo.AccelCalibration.Z0) / ((float)nunchukCalibrationInfo.AccelCalibration.ZG - nunchukCalibrationInfo.AccelCalibration.Z0);

		if (nunchukCalibrationInfo.MaxX != 0x00)
			nunchuk.Joystick.x = (float)((float)_joy.x - nunchukCalibrationInfo.MidX) / ((float)nunchukCalibrationInfo.MaxX - nunchukCalibrationInfo.MinX);

		if (nunchukCalibrationInfo.MaxY != 0x00)
			nunchuk.Joystick.y = (float)((float)_joy.y - nunchukCalibrationInfo.MidY) / ((float)nunchukCalibrationInfo.MaxY - nunchukCalibrationInfo.MinY);

		break;

	case ExtensionType::None:
	default:
		break;

	}

}

void Wiimote::parseButtons(unsigned char * in)
{
	buttons.One = (in[2] & 0x0002) ? true : false;
	buttons.Two = (in[2] & 0x0001) ? true : false;
	buttons.A = (in[2] & 0x0008) ? true : false;
	buttons.B = (in[2] & 0x0004) ? true : false;
	buttons.Minus = (in[2] & 0x0010) ? true : false;
	buttons.Plus = (in[1] & 0x0010) ? true : false;
	buttons.Home = (in[2] & 0x0080) ? true : false;
	buttons.Up = (in[1] & 0x0008) ? true : false;
	buttons.Down = (in[1] & 0x0004) ? true : false;
	buttons.Left = (in[1] & 0x0001) ? true : false;
	buttons.Right = (in[1] & 0x0002) ? true : false;
}

void Wiimote::parseAccel(unsigned char * in)
{
	unsigned int x, y, z;
	x = in[3];
	y = in[4];
	z = in[5];

	acc.x = (float)((float)x - accelCalibrationInfo.X0) / ((float)accelCalibrationInfo.XG - accelCalibrationInfo.X0);
	acc.y = (float)((float)y - accelCalibrationInfo.Y0) / ((float)accelCalibrationInfo.YG - accelCalibrationInfo.Y0);
	acc.z = (float)((float)z - accelCalibrationInfo.Z0) / ((float)accelCalibrationInfo.ZG - accelCalibrationInfo.Z0);
}

void Wiimote::parseIR(unsigned char * in)
{
	Wiimote::Pointers::IRPointer rawpos[4];

	rawpos[0].pos.x = in[6] | ((in[8] >> 4) & 0x03) << 8;
	rawpos[0].pos.y = in[7] | ((in[8] >> 6) & 0x03) << 8;

	switch (m_IRmode)
	{
	case IRMode::Basic:
		rawpos[1].pos.x = in[9] | ((in[8] >> 0) & 0x03) << 8;
		rawpos[1].pos.y = in[10] | ((in[8] >> 2) & 0x03) << 8;

		rawpos[2].pos.x = in[11] | ((in[13] >> 4) & 0x03) << 8;
		rawpos[2].pos.y = in[12] | ((in[13] >> 6) & 0x03) << 8;

		rawpos[3].pos.x = in[14] | ((in[13] >> 0) & 0x03) << 8;
		rawpos[3].pos.y = in[15] | ((in[13] >> 2) & 0x03) << 8;

		pointer[0].size = pointer[1].size = pointer[2].size = pointer[3].size = 0x00;

		pointer[0].found = !(in[6] == 0xff && in[7] == 0xff);
		pointer[1].found = !(in[9] == 0xff && in[10] == 0xff);
		pointer[2].found = !(in[11] == 0xff && in[12] == 0xff);
		pointer[3].found = !(in[14] == 0xff && in[15] == 0xff);

		break;

	case IRMode::Extended:
		rawpos[1].pos.x = in[9] | ((in[11] >> 4) & 0x03) << 8;
		rawpos[1].pos.y = in[10] | ((in[11] >> 6) & 0x03) << 8;
		rawpos[2].pos.x = in[12] | ((in[14] >> 4) & 0x03) << 8;
		rawpos[2].pos.y = in[13] | ((in[14] >> 6) & 0x03) << 8;
		rawpos[3].pos.x = in[15] | ((in[17] >> 4) & 0x03) << 8;
		rawpos[3].pos.y = in[16] | ((in[17] >> 6) & 0x03) << 8;

		pointer[0].size = (in[8] & 0x0f) == 15 ? -1 : in[8] & 0x0f;
		pointer[1].size = (in[11] & 0x0f) == 15 ? -1 : in[11] & 0x0f;
		pointer[2].size = (in[14] & 0x0f) == 15 ? -1 : in[14] & 0x0f;
		pointer[3].size = (in[17] & 0x0f) == 15 ? -1 : in[17] & 0x0f;

		pointer[0].found = !(in[6] == 0xff && in[7] == 0xff && in[8] == 0xff);
		pointer[1].found = !(in[9] == 0xff && in[10] == 0xff && in[11] == 0xff);
		pointer[2].found = !(in[12] == 0xff && in[13] == 0xff && in[14] == 0xff);
		pointer[3].found = !(in[15] == 0xff && in[16] == 0xff && in[17] == 0xff);

		break;
	}

	pointer[0].pos.x = rawpos[0].pos.x / 1023.5;
	pointer[1].pos.x = rawpos[1].pos.x / 1023.5;
	pointer[2].pos.x = rawpos[2].pos.x / 1023.5;
	pointer[3].pos.x = rawpos[3].pos.x / 1023.5;

	pointer[0].pos.y = rawpos[0].pos.y / 767.5;
	pointer[1].pos.y = rawpos[1].pos.y / 767.5;
	pointer[2].pos.y = rawpos[2].pos.y / 767.5;
	pointer[3].pos.y = rawpos[3].pos.y / 767.5;

}

void Wiimote::initializeExtension()
{
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];

	// 0xa400f0に0x55を書き込む (EXT INIT 1)
	out[0] = (unsigned char)OutputReport::WriteMemory;
	// レジスタから
	out[1] = 0x04;
	// アドレス
	out[2] = 0xa4;
	out[3] = 0x00;
	out[4] = 0xf0;
	// バイト数
	out[5] = 1;
	// データ(max16byte)
	out[6] = 0x55;
	write(out);
	read(in);

	// 0xa400fbに0x00を書き込む (EXT INIT 2)
	out[0] = (unsigned char)OutputReport::WriteMemory;
	// レジスタから
	out[1] = 0x04;
	// アドレス
	out[2] = 0xa4;
	out[3] = 0x00;
	out[4] = 0xfb;
	// バイト数
	out[5] = 1;
	// データ(max16byte)
	out[6] = 0x00;
	write(out);
	read(in);

	// 0xa400faからデータを読み込む (EXT TYPE)
	out[0] = (unsigned char)OutputReport::ReadMemory;
	// レジスタから
	out[1] = 0x04;
	// アドレス
	out[2] = 0xa4;
	out[3] = 0x00;
	out[4] = 0xfa;
	// サイズ
	out[5] = 0x00;
	out[6] = 0x06;
	write(out);
	read(in);

	while (in[0] != 0x21)
	{
		read(in);
	}

	extensionType = (ExtensionType)(((long long)in[6] << 40) | ((long long)in[7] << 32) |
		((long long)in[8]) << 24 | ((long long)in[9]) << 16 |
		((long long)in[10]) << 8 | in[11]);

	switch (extensionType)
	{
	case ExtensionType::Nunchuk:
		// ヌンチャク
		// SetReportType
		out[0] = (unsigned char)OutputReport::DataReportType;
		out[1] = 0x04;
		out[2] = 0x37;

		write(out);
		read(in);

		enableIR(IRMode::Basic, SENSITIVITY_MODE);

		// 0x04の0xa40020からデータを読み込む (EXT CALIBRATION)
		out[0] = (unsigned char)OutputReport::ReadMemory;
		// レジスタから
		out[1] = 0x04;
		// アドレス
		out[2] = 0xa4;
		out[3] = 0x00;
		out[4] = 0x20;
		// サイズ
		out[5] = 0x00;
		out[6] = 0x10;
		write(out);
		read(in);

		while (in[0] != 0x21)
		{
			read(in);
		}

		nunchukCalibrationInfo.AccelCalibration.X0 = in[6];
		nunchukCalibrationInfo.AccelCalibration.Y0 = in[7];
		nunchukCalibrationInfo.AccelCalibration.Z0 = in[8];
		nunchukCalibrationInfo.AccelCalibration.XG = in[10];
		nunchukCalibrationInfo.AccelCalibration.YG = in[11];
		nunchukCalibrationInfo.AccelCalibration.ZG = in[12];
		nunchukCalibrationInfo.MaxX = in[14];
		nunchukCalibrationInfo.MinX = in[15];
		nunchukCalibrationInfo.MidX = in[16];
		nunchukCalibrationInfo.MaxY = in[17];
		nunchukCalibrationInfo.MinY = in[18];
		nunchukCalibrationInfo.MidY = in[19];

		break;

		break;

	case ExtensionType::None:
	default:
		break;
	}

	delete[] out;
	delete[] in;
}

void Wiimote::updateThread(Wiimote* wii, bool forceReport)
{
	unsigned char* out = new unsigned char[wii->output_length];
	unsigned char* in = new unsigned char[wii->input_length];

	// キャリブレーションデータの取得
	out[0] = (unsigned char)OutputReport::ReadMemory;
	// レジスタから
	out[1] = 0x00;
	// アドレス
	out[2] = 0x00;
	out[3] = 0x00;
	out[4] = 0x16;
	// サイズ
	out[5] = 0x00;
	out[6] = 0x07;
	wii->write(out);
	wii->read(in);

	while (in[0] != 0x21)
	{
		wii->read(in);
	}

	wii->accelCalibrationInfo.X0 = in[6];
	wii->accelCalibrationInfo.Y0 = in[7];
	wii->accelCalibrationInfo.Z0 = in[8];
	wii->accelCalibrationInfo.XG = in[10];
	wii->accelCalibrationInfo.YG = in[11];
	wii->accelCalibrationInfo.ZG = in[12];

	Sleep(10);

	if (forceReport)
	{
		out[0] = (unsigned char)OutputReport::DataReportType;
		out[1] = 0x00;
		out[2] = (unsigned char)InputReport::IRAccel;

		wii->write(out);
		wii->read(in);

		Sleep(10);
	}

	// ステータス情報の取得
	out[0] = (unsigned char)OutputReport::Status;
	out[1] = 0x00;

	wii->write(out);
	wii->read(in);

	delete[] out;
	delete[] in;

	while (wii->wiihandle != nullptr)
	{
		wii->update();
		Sleep(2);
	}
}

void Wiimote::enableIR(IRMode mode, unsigned int sensitivity)
{
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];

	mtx.lock();

	m_IRmode = mode;

	//IRカメラの有効化
	out[0] = (unsigned char)OutputReport::IR;
	out[1] = 0x04 | (m_rumble ? 0x01 : 0x00);
	write(out);
	read(in);
	Sleep(60);

	out[0] = (unsigned char)OutputReport::IR2;
	out[1] = 0x04 | (m_rumble ? 0x01 : 0x00);
	write(out);
	read(in);
	Sleep(60);

	//0xb00030に0x08を書き込む
	out[0] = (unsigned char)OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x30;
	//バイト数
	out[5] = 1;
	//データ(max16byte)
	out[6] = 0x08;
	write(out);
	read(in);
	Sleep(60);

	//block1
	out[0] = (unsigned char)OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x00;
	//バイト数
	out[5] = 9;
	//データ(max16byte)
	out[6] = IRblock[sensitivity][0];
	out[7] = IRblock[sensitivity][1];
	out[8] = IRblock[sensitivity][2];
	out[9] = IRblock[sensitivity][3];
	out[10] = IRblock[sensitivity][4];
	out[11] = IRblock[sensitivity][5];
	out[12] = IRblock[sensitivity][6];
	out[13] = IRblock[sensitivity][7];
	out[14] = IRblock[sensitivity][8];
	write(out);
	read(in);
	Sleep(60);

	//block2
	out[0] = (unsigned char)OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x1a;
	//バイト数
	out[5] = 2;
	//データ(max16byte)
	out[6] = IRblock[sensitivity][9];
	out[7] = IRblock[sensitivity][10];
	write(out);
	read(in);
	Sleep(60);

	//mode number
	out[0] = (unsigned char)OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x33;
	//バイト数
	out[5] = 1;
	//データ(max16byte)
	out[6] = (unsigned char)mode;
	write(out);
	read(in);
	Sleep(60);

	//0xb00030に0x08を書き込む
	out[0] = (unsigned char)OutputReport::WriteMemory;
	//レジスタから
	out[1] = 0x04;
	//アドレス
	out[2] = 0xb0;
	out[3] = 0x00;
	out[4] = 0x30;
	//バイト数
	out[5] = 1;
	//データ(max16byte)
	out[6] = 0x08;
	write(out);
	read(in);
	Sleep(60);

	mtx.unlock();

	delete[] out;
	delete[] in;
}

void Wiimote::disableIR()
{
	unsigned char* out = new unsigned char[output_length];
	unsigned char* in = new unsigned char[input_length];

	mtx.lock();

	m_IRmode = IRMode::Off;

	out[0] = (unsigned char)OutputReport::IR;
	out[1] = (m_rumble ? 0x01 : 0x00);
	write(out);
	read(in);

	out[0] = (unsigned char)OutputReport::IR2;
	out[1] = (m_rumble ? 0x01 : 0x00);
	write(out);
	read(in);

	mtx.unlock();

	delete[] out;
	delete[] in;
}

void Wiimote::stopSound()
{
	m_isPlaying = false;
}

void Wiimote::playSound(char* filename, int volume, bool doReport)
{
	unsigned char* out = new unsigned char[output_length];

	m_isPlaying = true;
	volume = min(100, max(0, volume));
	unsigned char _vol = (unsigned char)(volume / 100.0 * 0xFF);

	mtx.lock();

	// スピーカー有効化
	out[0] = (unsigned char)OutputReport::SpeakerEnable;
	out[1] = 0x04;
	write(out);

	// スピーカーのミュート
	out[0] = (unsigned char)OutputReport::SpeakerMute;
	out[1] = 0x04;
	write(out);

	// 0xa20009に0x01 を書き込む
	out[0] = (unsigned char)OutputReport::WriteMemory;
	// レジスタから
	out[1] = 0x04;
	// アドレス
	out[2] = 0xa2;
	out[3] = 0x00;
	out[4] = 0x09;
	// バイト数
	out[5] = 1;
	// データ(max16byte)
	out[6] = 0x01;
	write(out);

	// 0xa20001に0x08 を書き込む
	out[0] = (unsigned char)OutputReport::WriteMemory;
	// レジスタから
	out[1] = 0x04;
	// アドレス
	out[2] = 0xa2;
	out[3] = 0x00;
	out[4] = 0x01;
	// バイト数
	out[5] = 1;
	// データ(max16byte)
	out[6] = 0x08;
	write(out);

	// 0xa20001に7-byte configuration を書き込む
	out[0] = (unsigned char)OutputReport::WriteMemory;
	// レジスタから
	out[1] = 0x04;
	// アドレス
	out[2] = 0xa2;
	out[3] = 0x00;
	out[4] = 0x01;
	// バイト数
	out[5] = 0x07;
	// データ(max16byte)
	out[6] = 0x00;
	out[7] = 0x40; // フォーマット。0x40なら8bitPCM、0x00ならADPCM
	out[8] = 0x70; // サンプリングレート
	out[9] = 0x17; // サンプリングレート
	out[10] = _vol; // ボリューム 0x00から0xffの範囲	
	out[11] = 0x00;
	out[12] = 0x00;
	write(out);

	// 0xa20008に0x01 を書き込む
	out[0] = (unsigned char)OutputReport::WriteMemory;
	// レジスタから
	out[1] = 0x04;
	// アドレス
	out[2] = 0xa2;
	out[3] = 0x00;
	out[4] = 0x08;
	// バイト数
	out[5] = 1;
	// データ(max16byte)
	out[6] = 0x01;
	write(out);

	// ミュート解除
	out[0] = (unsigned char)OutputReport::SpeakerMute;
	out[1] = 0x00;
	write(out);

	if (doReport) mtx.unlock();

	// 音声データ送信
	std::ifstream stream(filename, std::ios::binary);
	vector<char> hex;
	unsigned char data[1];

	while (!stream.fail())
	{
		stream.read(reinterpret_cast<char *>(data), 1);
		hex.push_back(static_cast<unsigned>(data[0]));
	}

	for (int i = 0; i < (int)hex.size() / 20; i++)
	{
		if (!m_isPlaying) break;
		if (doReport) mtx.lock();

		out[0] = (unsigned char)OutputReport::SpeakerData;
		out[1] = 0xa0;
		// データ本体
		for (int j = 0; j < 20; j++)
		{
			out[2 + j] = hex[i * 20 + j];
		}

		write(out);
		if (doReport) mtx.unlock();
		Sleep(doReport ? 1 : 2);
	}

	if (!doReport) mtx.unlock();

	delete[] out;

	m_isPlaying = false;

}

bool Wiimote::isPlaying()
{
	return m_isPlaying;
}

Wiimote::Pointers::IRPointer::IRPointer()
{
	pos.x = pos.y = -1;
	size = -1;
	found = false;
}

Wiimote::Pointers::IRPointer::IRPointer(const IRPointer& p)
{
	pos.x = p.pos.x;
	pos.y = p.pos.y;
	size = p.size;
	found = p.found;
}

Wiimote::Pointers::IRPointer Wiimote::Pointers::getMaximumPos()
{
	unsigned int num = 0;
	for (int i = 1; i < 4; i++)
	{
		if (pointers[i].found && pointers[num].size < pointers[i].size)
		{
			num = i;
		}
	}

	return pointers[num].found ? pointers[num] : IRPointer();
}

Wiimote::Pointers::IRPointer Wiimote::Pointers::getBarPos()
{
	IRPointer p;
	multimap<int, int> pos;
	
	for (int i = 0; i < 4; i++)
	{
		if (pointers[i].found)
		{
			pos.emplace(pointers[i].size, i);
		}
	}

	if (pos.size() < 2)
	{
		return IRPointer();
	}

	auto it1 = pos.begin(), it2 = it1++;

	p.pos.x = (pointers[it1->second].pos.x + pointers[it2->second].pos.x) / 2;
	p.pos.y = (pointers[it1->second].pos.y + pointers[it2->second].pos.y) / 2;
	p.size = (it1->first + it2->first) / 2;
	p.found = true;

	return p;
}

Wiimote::Pointers::Pointers()
{
	for (int i = 0; i < 4; i++)
	{
		pointers[i].size = 0;
		pointers[i].pos.x = pointers[i].pos.y = 0.0;
	}
}

Wiimote::Pointers::IRPointer& Wiimote::Pointers::operator[](unsigned int n)
{
	return pointers[n];
}

bool Wiimote::isOpened()
{
	return wiihandle != nullptr;
}

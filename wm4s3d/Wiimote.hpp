#pragma once

#include <vector>
#include <thread>
#include <mutex>

class Wiimote
{
public:
	/// <summary>
	/// 点 (x, y)
	/// </summary>
	class Point
	{
	public:
		double x, y;
	};

private:
	const unsigned short VID = 0x057e; // 任天堂
	const unsigned short PID = 0x0306; // Nintendo RVL-CNT-01

	unsigned int m_sensitivityMode = 6;

	bool m_isPlaying;
	bool m_rumble;

	enum class IRMode : unsigned char
	{
		Off = 0,
		Basic = 1,
		Extended = 3
	};

	class AccelCalibrationInfo
	{
	public:
		unsigned char X0, Y0, Z0;
		unsigned char XG, YG, ZG;
	} accelCalibrationInfo;

	class NunchukCalibrationInfo
	{
	public:
		AccelCalibrationInfo AccelCalibration;
		unsigned char MinX, MidX, MaxX;
		unsigned char MinY, MidY, MaxY;
	} nunchukCalibrationInfo;

	IRMode m_IRmode;

	void* wiihandle;
	unsigned short input_length, output_length;

	enum class WriteMethod
	{
		File,
		Hid
	} writemethod;

	void setWriteMethod();

	void write(unsigned char* output_report);
	void read(unsigned char* input_report);

	std::thread th;
	std::mutex mtx;
	static void updateThread(Wiimote* wii);
	static void rumbleThread(Wiimote* wii, int ms);
	void update();

	bool open();

	void parseAccel(unsigned char* in);
	void parseButtons(unsigned char* in);
	void parseIR(unsigned char* in);
	void parseExtension(unsigned char* in, int offset);
	void initializeExtension();

	void reportLEDs();

	void enableIR(IRMode mode, unsigned int sensitivity);
	void disableIR();

	// Bluetooth
	static std::thread th_scan;
	static void updateScanThread();

	static void disconnect();

	static std::vector<unsigned long> m_address;
	static std::vector<Wiimote*> m_wm;

	static bool m_scanning;
	static int m_connected;

public:
	///<summary>
	/// 入力ボタンの状態
	///</summary>
	class Button
	{
	public:
		bool One, Two, A, B, Minus, Plus, Home, Up, Down, Left, Right;
	} buttons;

	///<summary>
	/// LEDの状態
	///</summary>
	class LED
	{
	public:
		bool LED1, LED2, LED3, LED4;
	} LEDs;

	///<summary>
	/// 加速度
	///</summary>
	class Acceletion
	{
	public:
		double x, y, z;
	} acc;

	/// <summary>
	/// 接続されている拡張コントローラの種類
	/// </summary>
	enum class ExtensionType : long long
	{
		None = 0x000000000000,
		Nunchuk = 0x0000a4200000
	} extensionType;

	///<summary>
	/// バッテリー残量 (0-100)
	///</summary>
	double Battery;

	/// <summary>
	/// ヌンチャクの状態
	/// </summary>
	class Nunchuk
	{
	public:
		bool C;
		bool Z;
		Point Joystick;
		Acceletion acc;
	} nunchuk;

	///<summary>
	///赤外線センサから取得した位置情報
	///</summary>
	class Pointers
	{
	public:
		///<summary>
		/// 赤外線
		///</summary>
		class IRPointer
		{
		public:
			Point pos;
			int size;
			bool found;
			IRPointer();
			IRPointer(const IRPointer& pos);
		};

		Pointers();

		///<summary>
		/// 赤外線の点の座標を返す (0-3)
		///</summary>
		IRPointer& operator[](unsigned int n);

		///<summary>
		/// 赤外線の点の大きさが最大の点の座標を返す
		///</summary>
		IRPointer getMaximumPos();

		///<summary>
		/// 2点の赤外線座標の中間の座標を返す (センサーバーの中心座標)
		///</summary>
		IRPointer getBarPos();

	private:
		IRPointer pointers[4];

	} pointer;

	Wiimote();
	~Wiimote();

	/// <summary>
	/// Wiiリモコンの接続待機を開始する
	/// </summary>
	static void startScan();

	/// <summary>
	/// Wiiリモコンの接続待機を終了する
	/// </summary>
	static void stopScan();

	/// <summary>
	/// Wiiリモコンの接続が完了するまで待機する
	/// </summary>
	/// <param name="num">接続待機する台数</param>
	/// <returns>true: 接続完了, false: 未接続、接続待機中</returns>
	static bool waitConnect(const int num);

	/// <summary>
	/// 現在の接続台数を返す
	/// </summary>
	static int connectedCount();

	///<summary>
	/// Wiiリモコンの接続を閉じる
	///</summary>
	void close();

	///<summary>
	///振動のON・OFF
	///</summary>
	void setRumble(bool on);

	/// <summary>
	/// 振動しているかどうか
	/// </summary>
	bool isRumble();

	/// <summary>
	/// 指定ミリ秒、Wiiリモコンを振動させる
	/// </summary>
	/// <param name="ms">ミリ秒</param>
	void rumbleForMiliseconds(int ms);

	///<summary>
	///LEDを設定
	///</summary>
	void setLED(bool first, bool second, bool third, bool fourth);

	///<summary>
	///LED1:0x1, LED2:0x2, LED3:0x4, LED4:0x8
	///</summary>
	void setLED(unsigned char LED);

	///<summary>
	/// Wiiリモコンに接続しているか
	///</summary>
	bool isConnected();

	/// <summary>
	/// 再生を停止します
	/// </summary>
	void stopSound();

	/// <summary>
	/// スピーカーから音声ファイルを再生します
	/// </summary>
	/// <param name="filename">ファイル名(形式は、8bitPCM)</param>
	/// <param name="volume">音量。0-100</param>
	void playSound(const char* filename, const int volume, const bool doReport = false);

	/// <summary>
	/// スピーカーで音を再生中かどうか
	/// </summary>
	bool isPlaying();

};

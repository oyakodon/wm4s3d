#pragma once

#include <vector>
#include <thread>
#include <mutex>

class Wiimote
{
public:
	/// <summary>
	/// �_ (x, y)
	/// </summary>
	class Point
	{
	public:
		double x, y;
	};

private:
	const unsigned short VID = 0x057e; // �C�V��
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
	/// ���̓{�^���̏��
	///</summary>
	class Button
	{
	public:
		bool One, Two, A, B, Minus, Plus, Home, Up, Down, Left, Right;
	} buttons;

	///<summary>
	/// LED�̏��
	///</summary>
	class LED
	{
	public:
		bool LED1, LED2, LED3, LED4;
	} LEDs;

	///<summary>
	/// �����x
	///</summary>
	class Acceletion
	{
	public:
		double x, y, z;
	} acc;

	/// <summary>
	/// �ڑ�����Ă���g���R���g���[���̎��
	/// </summary>
	enum class ExtensionType : long long
	{
		None = 0x000000000000,
		Nunchuk = 0x0000a4200000
	} extensionType;

	///<summary>
	/// �o�b�e���[�c�� (0-100)
	///</summary>
	double Battery;

	/// <summary>
	/// �k���`���N�̏��
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
	///�ԊO���Z���T����擾�����ʒu���
	///</summary>
	class Pointers
	{
	public:
		///<summary>
		/// �ԊO��
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
		/// �ԊO���̓_�̍��W��Ԃ� (0-3)
		///</summary>
		IRPointer& operator[](unsigned int n);

		///<summary>
		/// �ԊO���̓_�̑傫�����ő�̓_�̍��W��Ԃ�
		///</summary>
		IRPointer getMaximumPos();

		///<summary>
		/// 2�_�̐ԊO�����W�̒��Ԃ̍��W��Ԃ� (�Z���T�[�o�[�̒��S���W)
		///</summary>
		IRPointer getBarPos();

	private:
		IRPointer pointers[4];

	} pointer;

	Wiimote();
	~Wiimote();

	/// <summary>
	/// Wii�����R���̐ڑ��ҋ@���J�n����
	/// </summary>
	static void startScan();

	/// <summary>
	/// Wii�����R���̐ڑ��ҋ@���I������
	/// </summary>
	static void stopScan();

	/// <summary>
	/// Wii�����R���̐ڑ�����������܂őҋ@����
	/// </summary>
	/// <param name="num">�ڑ��ҋ@����䐔</param>
	/// <returns>true: �ڑ�����, false: ���ڑ��A�ڑ��ҋ@��</returns>
	static bool waitConnect(const int num);

	/// <summary>
	/// ���݂̐ڑ��䐔��Ԃ�
	/// </summary>
	static int connectedCount();

	///<summary>
	/// Wii�����R���̐ڑ������
	///</summary>
	void close();

	///<summary>
	///�U����ON�EOFF
	///</summary>
	void setRumble(bool on);

	/// <summary>
	/// �U�����Ă��邩�ǂ���
	/// </summary>
	bool isRumble();

	/// <summary>
	/// �w��~���b�AWii�����R����U��������
	/// </summary>
	/// <param name="ms">�~���b</param>
	void rumbleForMiliseconds(int ms);

	///<summary>
	///LED��ݒ�
	///</summary>
	void setLED(bool first, bool second, bool third, bool fourth);

	///<summary>
	///LED1:0x1, LED2:0x2, LED3:0x4, LED4:0x8
	///</summary>
	void setLED(unsigned char LED);

	///<summary>
	/// Wii�����R���ɐڑ����Ă��邩
	///</summary>
	bool isConnected();

	/// <summary>
	/// �Đ����~���܂�
	/// </summary>
	void stopSound();

	/// <summary>
	/// �X�s�[�J�[���特���t�@�C�����Đ����܂�
	/// </summary>
	/// <param name="filename">�t�@�C����(�`���́A8bitPCM)</param>
	/// <param name="volume">���ʁB0-100</param>
	void playSound(const char* filename, const int volume, const bool doReport = false);

	/// <summary>
	/// �X�s�[�J�[�ŉ����Đ������ǂ���
	/// </summary>
	bool isPlaying();

};

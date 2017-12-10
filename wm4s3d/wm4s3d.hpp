#pragma once
#include <Siv3D.hpp>
#include "Wiimote.hpp"
#include <chrono>

class Button
{
public:
	Button();
	bool pressed;
	bool clicked;
	bool released;
	int32 pressedDuration;
	std::chrono::time_point<std::chrono::system_clock> pressedStart;
};

class Wii
{
private:
	enum class ButtonType
	{
		A, B, One, Two, Plus, Minus, Home, Up, Down, Left, Right, C, Z
	};

	std::unordered_map<ButtonType, bool> m_pressed;
	std::unordered_map<ButtonType, Button*> m_buttons;

	bool m_connected;

public:
	Wiimote controller;

	Wii();
	~Wii();

	/// <summary>
	/// Wii�����R���̃{�^�����Ȃǂ��X�V���܂� (���[�v���ɌĂяo��)
	/// </summary>
	void update();

	/// <summary>
	/// Wii�����R�����ڑ�����Ă��邩�ǂ���
	/// </summary>
	bool isConnected()
	{
		return controller.isConnected();
	}

	/// <summary>
	/// Wii�����R�����ڑ����ꂽ����true�A����ȊO��false��Ԃ��܂��B
	/// </summary>
	bool hasConnected()
	{
		if (!m_connected && isConnected())
		{
			m_connected = true;
			return true;
		}

		return false;
	}

	/// <summary>
	/// Wii�����R����U�������܂�
	/// </summary>
	/// <param name="ms">�U�������鎞�� (�~���b)</param>
	void rumble(int ms)
	{
		controller.rumbleForMiliseconds(ms);
	}

	/// <summary>
	/// Wii�����R����U�������܂�
	/// </summary>
	/// <param name="on">�U��</param>
	void rumble(bool on)
	{
		controller.setRumble(on);
	}

	/// <summary>
	/// Wii�����R����LED��_��/���������܂�
	/// </summary>
	/// <param name="one">������1�Ԗڂ�LED</param>
	/// <param name="two">������2�Ԗڂ�LED</param>
	/// <param name="three">������3�Ԗڂ�LED</param>
	/// <param name="four">������4�Ԗڂ�LED</param>
	void setLED(bool one, bool two, bool three, bool four)
	{
		controller.setLED(one, two, three, four);
	}

	/// <summary>
	/// Wii�����R����LED��_��/���������܂�
	/// </summary>
	/// <param name="led">4����LED�̏�� (����:0, �_��:1)</param>
	void setLED(const char* _led)
	{
		const String led = WidenAscii(_led);
		if (led.length > 4 || led.length == 0) return;

		controller.setLED(
			led[0] != '0',
			led[1] != '0',
			led[2] != '0',
			led[3] != '0'
		);
	}

	/// <summary>
	/// Wii�����R����LED��_��/���������܂�
	/// </summary>
	/// <param name="num">LED�ԍ� (0-3)</param>
	/// <param name="on">LED��ON/OFF</param>
	void setLED(int num, bool on)
	{
		bool one	= num == 0 ? on : getLED(0),
			 two	= num == 1 ? on : getLED(1),
			 three	= num == 2 ? on : getLED(2),
			 four	= num == 3 ? on : getLED(3);

		controller.setLED(one, two, three, four);
	}

	/// <summary>
	/// Wii�����R����LED�̏�Ԃ�Ԃ��܂�
	/// </summary>
	/// <param name="num">LED�ԍ� (0-3)</param>
	bool getLED(int num)
	{
		switch (num)
		{
			case 0: return controller.LEDs.LED1;
			case 1: return controller.LEDs.LED2;
			case 2: return controller.LEDs.LED3;
			case 3: return controller.LEDs.LED4;
			default: return false;
		}
	}

	/// <summary>
	/// �k���`���N���g���邩�ǂ���
	/// </summary>
	bool isNunchukConnected()
	{
		return controller.extensionType == Wiimote::ExtensionType::Nunchuk;
	}

	/// <summary>
	/// �E�B���h�E��ł̃|�C���^�[�̈ʒu���v�Z���ĕԂ��܂�
	/// </summary>
	Point getPointerInWindow();

	/// <summary>
	/// �E�B���h�E��ł̃|�C���^�[�̈ʒu���v�Z���ĕԂ��܂�
	/// </summary>
	/// <param name="shift">true�Ȃ���W��2�{����Window::Size()�̔������s�ړ����������W��Ԃ��܂�</param>
	Point getPointerInWindow(bool shift);

	/// <summary>
	/// �E�B���h�E��ł̃W���C�X�e�B�b�N�̈ʒu���v�Z���ĕԂ��܂�
	/// </summary>
	Point getJoystickInWindow();

	/// <summary>
	/// �����ꂩ�̃{�^���������ꂽ���ǂ���
	/// </summary>
	bool AnyKeyClicked();

	/// <summary>
	/// �{�^��
	/// </summary>
	Button buttonA;
	Button buttonB;
	Button buttonOne;
	Button buttonTwo;
	Button buttonPlus;
	Button buttonMinus;
	Button buttonHome;
	Button buttonUp;
	Button buttonDown;
	Button buttonLeft;
	Button buttonRight;
	Button nunchukC;
	Button nunchukZ;

	/// <summary>
	/// �|�C���^�[�̈ʒu (�v�Z���T�[�o�[)
	/// </summary>
	Vec2 pointer;

	/// <summary>
	///�k���`���N�̃W���C�X�e�B�b�N (�v�k���`���N)
	/// </summary>
	Vec2 joystick;

};

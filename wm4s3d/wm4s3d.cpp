#include "wm4s3d.hpp"

Button::Button()
{
	pressed = clicked = released = false;
	pressedDuration = 0;
	pressedStart = std::chrono::system_clock::now();
}

Wii::Wii()
{
	m_connected = false;

	m_isShaked = false;
	m_shakeThreshould = (int)Threshould::Normal;

	m_buttons[ButtonType::One] = &buttonOne;
	m_buttons[ButtonType::Two] = &buttonTwo;
	m_buttons[ButtonType::A] = &buttonA;
	m_buttons[ButtonType::B] = &buttonB;
	m_buttons[ButtonType::Minus] = &buttonMinus;
	m_buttons[ButtonType::Plus] = &buttonPlus;
	m_buttons[ButtonType::Home] = &buttonHome;
	m_buttons[ButtonType::Up] = &buttonUp;
	m_buttons[ButtonType::Down] = &buttonDown;
	m_buttons[ButtonType::Left] = &buttonLeft;
	m_buttons[ButtonType::Right] = &buttonRight;
	m_buttons[ButtonType::C] = &nunchukC;
	m_buttons[ButtonType::Z] = &nunchukZ;
}

Wii::~Wii()
{
	Wiimote::stopScan();

	if (controller.isConnected())
	{
		controller.close();
	}
}

void Wii::update()
{
	for (auto& i : m_pressed)
	{
		i.second = false;
	}

	if (controller.isConnected())
	{
		m_pressed[ButtonType::One] = controller.buttons.One;
		m_pressed[ButtonType::Two] = controller.buttons.Two;
		m_pressed[ButtonType::A] = controller.buttons.A;
		m_pressed[ButtonType::B] = controller.buttons.B;
		m_pressed[ButtonType::Minus] = controller.buttons.Minus;
		m_pressed[ButtonType::Plus] = controller.buttons.Plus;
		m_pressed[ButtonType::Home] = controller.buttons.Home;
		m_pressed[ButtonType::Up] = controller.buttons.Up;
		m_pressed[ButtonType::Down] = controller.buttons.Down;
		m_pressed[ButtonType::Left] = controller.buttons.Left;
		m_pressed[ButtonType::Right] = controller.buttons.Right;

		auto ir = controller.pointer.getBarPos();
		pointer = { ir.pos.x, ir.pos.y };
		joystick = { controller.nunchuk.Joystick.x, controller.nunchuk.Joystick.y };

		if (m_shakeWait.isActive())
		{
			m_isShaked = false;

			if (m_shakeWait.ms() >= 500)
			{
				m_shakeWait.reset();
			}
		}
		else if (m_prevAcc.has_value())
		{
			Vec3 now = acc();
			m_isShaked = Abs(now.x + now.y + now.z - m_prevAcc.value().x - m_prevAcc.value().y - m_prevAcc.value().z) * 60 >= m_shakeThreshould;

			if (m_isShaked)
			{
				m_shakeWait.start();
			}

		}

		m_prevAcc = acc();

		if (isNunchukConnected())
		{
			m_pressed[ButtonType::C] = controller.nunchuk.C;
			m_pressed[ButtonType::Z] = controller.nunchuk.Z;
		}

	}
	else
	{
		pointer = { -1, -1 };
		joystick = { -1, -1 };
	}

	for (const auto i : m_pressed)
	{
		m_buttons[i.first]->clicked = (!m_buttons[i.first]->pressed && i.second);
		m_buttons[i.first]->released = (m_buttons[i.first]->pressed && !i.second);
		m_buttons[i.first]->pressed = i.second;

		if (m_buttons[i.first]->clicked)
		{
			m_buttons[i.first]->pressedStart = std::chrono::system_clock::now();
		}

		if (m_buttons[i.first]->pressed)
		{
			m_buttons[i.first]->pressedDuration = (int32)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_buttons[i.first]->pressedStart).count();
		}
		else
		{
			m_buttons[i.first]->pressedDuration = 0;
		}
	}
}

Point Wii::getPointerInWindow()
{
	return getPointerInWindow(true);
}

Point Wii::getPointerInWindow(bool shift)
{
	Point pos;
	pos.x = (int)((1 - pointer.x) * Window::Size().x);
	pos.y = (int)(pointer.y * Window::Size().y);
	return shift ? (pos * 2).moveBy(-(Window::Size() / 2)) : pos;
}

Point Wii::getJoystickInWindow()
{
	Point pos;
	pos.x = (int)((joystick.x + 0.5) * Window::Size().x);
	pos.y = (int)((0.5 - joystick.y) * Window::Size().y);
	return pos;
}

bool Wii::AnyKeyClicked()
{
	if (!isConnected())
	{
		return false;
	}

	bool clicked = false;

	clicked |= buttonA.clicked;
	clicked |= buttonB.clicked;
	clicked |= buttonOne.clicked;
	clicked |= buttonTwo.clicked;
	clicked |= buttonPlus.clicked;
	clicked |= buttonMinus.clicked;
	clicked |= buttonHome.clicked;
	clicked |= buttonUp.clicked;
	clicked |= buttonDown.clicked;
	clicked |= buttonLeft.clicked;
	clicked |= buttonRight.clicked;

	if (isNunchukConnected())
	{
		clicked |= nunchukC.clicked;
		clicked |= nunchukZ.clicked;
	}

	return clicked;
}

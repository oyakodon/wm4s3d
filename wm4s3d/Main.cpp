#include <Siv3D.hpp>
#include "wm4s3d.hpp"

using namespace std;

void DrawIR(Wii& wii);

void Main()
{
	Wii::forceReport();
	Wii wii[4];
	Font guiFont(12), font(18);

	Window::Resize(960, 480);

	// 左上
	GUI guiControlloer(GUIStyle::Default);
	guiControlloer.addln(L"Controller", GUIRadioButton::Create(
	{ L"1P", L"2P", L"3P", L"4P" }, 0u));
	guiControlloer.add(L"hr", GUIHorizontalLine::Create(1));
	guiControlloer.horizontalLine(L"hr").style.color = Color(127);
	guiControlloer.addln(L"Vib", GUICheckBox::Create({ L"振動" }));
	guiControlloer.add(L"hr2", GUIHorizontalLine::Create(1));
	guiControlloer.horizontalLine(L"hr2").style.color = Color(127);
	guiControlloer.addln(L"Disable", GUICheckBox::Create({ L"非表示" }));

	const RoundRect WiiFront(120, 20, 115, 430, 10); // Wii本体 前面
	const RoundRect WiiBack(270, 20, 115, 430, 10); // Wii本体 後面

	// ボタン
	const Circle buttonA(177.5, 155, 17.5);
	const Circle buttonMinus(144.5, 230, 10);
	const Circle buttonPlus(210.5, 230, 10);
	const Circle buttonHome(177.5, 230, 10);
	const Circle buttonOne(177.5, 335, 12.5);
	const Circle buttonTwo(177.5, 375, 12.5);
	const Circle buttonPower(145, 40, 10);
	const Rect buttonUp(170, 60, 15, 20);
	const Rect buttonDown(170, 97, 15, 20);
	const Rect buttonLeft(149, 81, 20, 15);
	const Rect buttonRight(186, 81, 20, 15);
	const Rect buttonB(307, 80, 40, 65);

	// LED
	const Point ledOrigin = { 140, 405 };
	Array<Rect> leds;
	for (int i = 0; i < 4; i++)
	{
		leds.emplace_back(Rect(ledOrigin + Point(i * 22, 0), 10, 10));
	}

	// スピーカー
	const Rect speakerRect(170, 268, 18, 36);
	const Point speakerOrigin = { 173, 270 };
	Array<Circle> speakers;
	for (int i = 0; i < 6; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			speakers.emplace_back(Circle(speakerOrigin + Point(j * 6, i * 6), 1));
		}
	}

	// ヌンチャク
	const Polygon nunchuk_body
	{
		{ 40, -80 },{ 20, 60 },
		{ -20, 60 },{ -40, -80 }
	};
	const Circle nunchuk_thumb(760, 290, 30);
	const Ellipse nunchuk_C(875, 275, 18, 12);
	const Rect nunchuk_Z(850, 300, 50, 30);

	while (System::Update())
	{
		const unsigned idx = guiControlloer.radioButton(L"Controller").checkedItem.value();
		wii[idx].update();

		if (guiControlloer.checkBox(L"Vib").checked(0))
		{
			wii[idx].rumble(true);
		}
		else
		{
			wii[idx].rumble(false);
		}

		if (guiControlloer.checkBox(L"Disable").checked(0))
		{
			if (wii[idx].isConnected())
			{
				DrawIR(wii[idx]);
			}
			continue;
		}

		// 描画
		WiiFront.draw(Palette::White);
		WiiBack.draw(Palette::White);

		buttonA.draw(Palette::White).drawFrame(0.0, wii[idx].buttonA.pressed ? 5.0 : 1.0, Palette::Black);
		guiFont(L"A").drawCenter(buttonA.center, Palette::Black);
		buttonMinus.draw(Palette::White).drawFrame(0.0, wii[idx].buttonMinus.pressed ? 5.0 : 1.0, Palette::Black);
		guiFont(L"-").drawCenter(buttonMinus.center, Palette::Black);
		buttonPlus.draw(Palette::White).drawFrame(0.0, wii[idx].buttonPlus.pressed ? 5.0 : 1.0, Palette::Black);
		guiFont(L"+").drawCenter(buttonPlus.center, Palette::Black);
		buttonHome.draw(Palette::Skyblue).drawFrame(0.0, wii[idx].buttonHome.pressed ? 5.0 : 1.0, Palette::Black);
		buttonOne.draw(Palette::White).drawFrame(0.0, wii[idx].buttonOne.pressed ? 5.0 : 1.0, Palette::Black);
		guiFont(L"1").drawCenter(buttonOne.center, Palette::Black);
		buttonTwo.draw(Palette::White).drawFrame(0.0, wii[idx].buttonTwo.pressed ? 5.0 : 1.0, Palette::Black);
		guiFont(L"2").drawCenter(buttonTwo.center, Palette::Black);
		buttonB.draw(Palette::White).drawFrame(0.0, wii[idx].buttonB.pressed ? 5.0 : 1.0, Palette::Black);
		guiFont(L"B").drawCenter(buttonB.center + Vec2(0, 25), Palette::Black);
		buttonPower.draw(Palette::Red);

		buttonUp.draw(wii[idx].buttonUp.pressed ? Palette::Red : Palette::White).drawFrame(0.0, 1.0, Palette::Black);
		buttonDown.draw(wii[idx].buttonDown.pressed ? Palette::Red : Palette::White).drawFrame(0.0, 1.0, Palette::Black);
		buttonLeft.draw(wii[idx].buttonLeft.pressed ? Palette::Red : Palette::White).drawFrame(0.0, 1.0, Palette::Black);
		buttonRight.draw(wii[idx].buttonRight.pressed ? Palette::Red : Palette::White).drawFrame(0.0, 1.0, Palette::Black);

		for (auto sp : speakers)
		{
			sp.draw(wii[idx].controller.isPlaying() ? Palette::Red : Palette::Black);
		}

		if (speakerRect.leftClicked)
		{
			wii[idx].controller.playSound("sample.raw", 100);
		}

		for (int i = 0; i < 4; i++)
		{
			leds[i].draw(wii[idx].getLED(i) ? Palette::Deepskyblue : Palette::Gray).drawFrame(0.5, 0.0, Palette::Black);
			if (leds[i].leftClicked)
			{
				wii[idx].setLED(i, !wii[idx].getLED(i));
			}
		}

		if (wii[idx].isConnected())
		{
			if (wii[idx].isNunchukOK())
			{
				Circle(760, 290, 40).draw(Palette::White);
				Circle(760, 430, 20).draw(Palette::White);
				nunchuk_body.draw({ 760, 375 }, Palette::White);

				nunchuk_thumb.draw(Color(127));
				Circle(nunchuk_thumb.center + Vec2(wii[idx].controller.nunchuk.Joystick.x, -wii[idx].controller.nunchuk.Joystick.y) * 25, 20).draw();

				Circle(875, 290, 40).draw(Palette::White);
				Circle(875, 430, 20).draw(Palette::White);
				nunchuk_body.draw({ 875, 375 }, Palette::White);
				nunchuk_C.draw(Palette::White).drawFrame(0.0, wii[idx].buttonC.pressed ? 5.0 : 1.0, Palette::Black);
				guiFont(L"C").drawCenter(nunchuk_C.center, Palette::Black);
				nunchuk_Z.draw(Palette::White).drawFrame(0.0, wii[idx].buttonZ.pressed ? 5.0 : 1.0, Palette::Black);
				guiFont(L"Z").drawCenter(nunchuk_Z.center, Palette::Black);

				font(L"ヌンチャク").draw({ 680, 40 });
				font(L"加速度X: ", wii[idx].controller.nunchuk.acc.x).draw({ 680, 80 });
				font(L"　　　Y: ", wii[idx].controller.nunchuk.acc.y).draw({ 680, 120 });
				font(L"　　　Z: ", wii[idx].controller.nunchuk.acc.z).draw({ 680, 160 });
			}
			else
			{
				font(L"ヌンチャクが\n接続されていません。").draw({ 680, 40 }, Palette::Red);
			}

			font(L"加速度X: ", wii[idx].controller.acc.x).draw({ 400, 40 });
			font(L"　　　Y: ", wii[idx].controller.acc.y).draw({ 400, 80 });
			font(L"　　　Z: ", wii[idx].controller.acc.z).draw({ 400, 120 });
			font(L"バッテリー残量: ", ToString(wii[idx].controller.Battery, 1), L"%").draw({ 400, 200 });

			DrawIR(wii[idx]);

		}
		else
		{
			font(L"Wiiリモコンが接続されていません。").draw({ 400, 40 }, Palette::Red);
		}
	}

}

void DrawIR(Wii& wii)
{
	Wiimote::Pointers::IRPointer ir;
	for (int i = 0; i < 4; i++)
	{
		ir = wii.controller.pointer[i];
		if (ir.found)
		{
			Circle((1 - ir.pos.x) * Window::Width(), ir.pos.y * Window::Height(), ir.size == 0 ? 4 : ir.size + 3).draw(HSV(60 * i)).drawFrame(0.0, 1.0, Palette::Black);
		}
	}
}

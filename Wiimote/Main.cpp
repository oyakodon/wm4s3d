#include <iostream>
#include <cstdio>
#include "Wiimote.hpp"

using namespace std;

int main()
{
	Wiimote wii;

	cout << "Connecting...";
	// 一台接続されるまで待機
	while (!Wiimote::waitConnect(1)) ;

	// LED消灯
	wii.setLED(0x0);

	cout << " : OK" << endl;
	cout << "Running..." << endl;
	cout << "Battery : " << wii.Battery << endl;

	// HOMEボタンが押されるまで
	while (!wii.buttons.Home)
	{
		printf("\rA: %d", wii.buttons.A);
	}

	wii.close();

	return 0;
}


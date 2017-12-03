#include <iostream>
#include <cstdio>
#include "Wiimote.hpp"

using namespace std;

int main()
{
	Wiimote wii;

	cout << "Connecting...";
	// ���ڑ������܂őҋ@
	while (!Wiimote::waitConnect(1)) ;

	// LED����
	wii.setLED(0x0);

	cout << " : OK" << endl;
	cout << "Running..." << endl;
	cout << "Battery : " << wii.Battery << endl;

	// HOME�{�^�����������܂�
	while (!wii.buttons.Home)
	{
		printf("\rA: %d", wii.buttons.A);
	}

	wii.close();

	return 0;
}


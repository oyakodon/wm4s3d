#include <iostream>
#include <cstdio>
#include "Wiimote.hpp"

using namespace std;

int main()
{
	Wiimote wii;

	cout << "Connecting...";
	// ˆê‘äÚ‘±‚³‚ê‚é‚Ü‚Å‘Ò‹@
	while (!Wiimote::waitConnect(1)) ;

	// LEDÁ“”
	wii.setLED(0x0);

	cout << " : OK" << endl;
	cout << "Running..." << endl;
	cout << "Battery : " << wii.Battery << endl;

	// HOMEƒ{ƒ^ƒ“‚ª‰Ÿ‚³‚ê‚é‚Ü‚Å
	while (!wii.buttons.Home)
	{
		printf("\rA: %d", wii.buttons.A);
	}

	wii.close();

	return 0;
}


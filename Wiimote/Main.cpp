#include <iostream>
#include <cstdio>
#include "Wiimote.hpp"

using namespace std;

int main()
{
	Wiimote wii;

	cout << "Initializing...";
	if (!wii.open())
	{
		cerr << "An error occured in wii.open()" << endl;
		return 1;
	}
	else
	{
		wii.setLED(0x1);
	}
	cout << " : OK" << endl;
	cout << "Running..." << endl;
	cout << "Battery : " << wii.Battery << endl;



	while (!wii.buttons.Home)
	{
		printf("\rA: %d", wii.buttons.A);
	}

	wii.close();

	return 0;
}


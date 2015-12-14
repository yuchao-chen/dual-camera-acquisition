#include <iostream>
#include <string>

#include "sync_que.h"
#include "buffer.h"
#include "camera.h"
#include "data_handle.h"


#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <SisoDisplay.h>
#include <fitsio.h>

#include "board-and-dll-chooser.h"

int main( int argc, char *argv[] ) {

	std::cout << "\n--------- Initialising Devices ----------\n";
	data::SynchronisedQueue< data::Buffer > data_que;
	ic::Camera camera1( &data_que );
	ic::Camera camera0( &data_que );
	dh::DataHandle data_handle( &data_que );
	camera1.Connect(1);
	camera0.Connect(0);

	std::string input;
	while( true ) {
		std::cin.clear();
		std::cout << "\n--------------- Main Menu ---------------\n";
		std::cout << "    [*] 0. Play.\n";
		std::cout << "    [*] 1. Save.\n";
		std::cout << "    [*] 2. Stop.\n";		
		std::cout << "    [*] 3. Quit.\n";
		//if ( !working ) {
		//	std::cout << "    [*] 3. Reload the Configuration File.\n";
		//}
		std::cout << "  >> ";
		std::cin >> input;
		if ( !std::cin.fail() ) {
			if ( input == "0" ) {
				data_handle.Join();
				camera1.Join();
				camera0.Join();
				camera1.EnableSaving(false);
				camera0.EnableSaving(false);
				data_handle.Start();
				camera1.Start();
				camera0.Start();
			} else if ( input == "1" ) {
				camera1.Join();
				camera0.Join();
				data_handle.Join();
				camera1.EnableSaving(true);
				camera0.EnableSaving(true);
				data_handle.Start();
				camera1.Start();
				camera0.Start();
			} else if ( input == "2" ) {
				camera1.Join();
				camera0.Join();
				data_handle.Join();
			} else if ( input == "3" ) {
				break;
			}
		}

	}

	camera1.Join();
	camera0.Join();
	data_handle.Join();
	getchar();
	getchar();

	return 0;
}
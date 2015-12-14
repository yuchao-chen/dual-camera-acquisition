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
	ic::Camera camera( &data_que );
	dh::DataHandle data_handle( &data_que );
	camera.Connect(1);
	
	std::string input;
	while( true ) {
		std::cin.clear();
		std::cout << "\n--------------- Main Menu ---------------\n";
		std::cout << "    [*] 0. Start Acquisition.\n";
		std::cout << "    [*] 1. Stop Acquisition.\n";
		std::cout << "    [*] 2. Quit.\n";
		//if ( !working ) {
		//	std::cout << "    [*] 3. Reload the Configuration File.\n";
		//}
		std::cout << "  >> ";
		std::cin >> input;
		if ( !std::cin.fail() ) {
			if ( input == "0" ) {
				data_handle.Start();
				camera.Start();
			} else if ( input == "1" ) {
				camera.Join();
				data_handle.Join();
			} else if ( input == "2" ) {
				break;
			} else if ( input == "3" ) {

			}
		}

	}

	camera.Join();
	data_handle.Join();
	getchar();
	getchar();










	return 0;
}
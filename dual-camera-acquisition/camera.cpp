#include <Windows.h>

#include "camera.h"

#include <iostream>

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>

#include <iostream>
#include <string>
#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <SisoDisplay.h>
#include <fitsio.h>

#include "board-and-dll-chooser.h"



namespace ic {
	Camera::Camera( data::SynchronisedQueue< data::Buffer > *data_que )
		:thread_( NULL ),
		data_que_( data_que ),
		connection_lost_(true),
		fixed_frame_(150) {
			//std::cout << "Camera()" << std::endl;	
			
	}
	Camera::~Camera() {
		//std::cout << "~Camera()" << std::endl;
		if ( thread_ ) {
			thread_->interrupt();
			thread_->join();
			delete thread_;
		}
		if (!connection_lost_) {
			ShutDown();
		}
		
	}

	void Camera::operator() () try {
		if (connection_lost_) {
			if ( Connect() < 0 ) {
				return;
			}
		}
		



		Fg_Struct *fg						= NULL;
		int board_no						= SelectBoardDialog();
		int cam_port						= PORT_B;
		frameindex_t no_of_pictures_to_grab	= 1000;
		frameindex_t no_of_buffers			= 4;
		unsigned int width					= 1024;
		unsigned int height					= 1024;
		int sample_per_pixel				= 1;
		size_t byte_per_sample				= 1;
		const char *applet;
		switch (Fg_getBoardType(board_no)) {
		case PN_MICROENABLE4AS1CL:
			applet = "SingleAreaGray16";
			break;
		case PN_MICROENABLE4AD1CL:
		case PN_MICROENABLE4AD4CL:
		case PN_MICROENABLE4VD1CL:
		case PN_MICROENABLE4VD4CL:
			applet = "DualAreaGray16";
			break;
		case PN_MICROENABLE4AQ4GE:
		case PN_MICROENABLE4VQ4GE:
			applet = "QuadAreaGray16";
			break;
		case PN_MICROENABLE3I:
			applet = "DualAreaGray";
			break;
		case PN_MICROENABLE3IXXL:
			applet = "DualAreaGray12XXL";
			break;
		default:
			applet = "DualAreaGray16";
			break;
		}
		std::cout << " [+] " << std::string(applet) << std::endl;
		if ((fg = Fg_Init(applet, board_no)) == NULL) {
			fprintf(stderr, " [-] Error in Fg_Init: %s\n", Fg_getLastErrorDescription(NULL));
			return;
		}

		int disp_id0 = CreateDisplay(8*byte_per_sample*sample_per_pixel, width, height);
		SetBufferWidth(disp_id0, width, height);
		size_t total_buffer_size = width * height * sample_per_pixel * byte_per_sample * no_of_buffers;
		dma_mem *mem_hndl = Fg_AllocMemEx(fg, total_buffer_size, no_of_buffers);
		if (mem_hndl == NULL) {
			fprintf(stderr, " [-] Error in Fg_AllocMemEx: %s\n", Fg_getLastErrorDescription(fg));
			CloseDisplay(disp_id0);
			Fg_FreeGrabber(fg);
			return;
		}

		// Image width of the acquisition window.
		if (Fg_setParameter(fg, FG_WIDTH, &width, cam_port) < 0 ) {
			fprintf(stderr, " [-] Fg_setParameter(FG_WIDTH) failed: %s\n", Fg_getLastErrorDescription(fg));
			Fg_FreeMemEx(fg, mem_hndl);
			CloseDisplay(disp_id0);
			Fg_FreeGrabber(fg);
			return;
		}

		// Image height of the acquisition window.
		if (Fg_setParameter(fg,FG_HEIGHT,&height,cam_port) < 0 ) {
			fprintf(stderr, " [-] Fg_setParameter(FG_HEIGHT) failed: %s\n", Fg_getLastErrorDescription(fg));
			Fg_FreeMemEx(fg, mem_hndl);
			CloseDisplay(disp_id0);
			Fg_FreeGrabber(fg);
			return;
		}

		// Set CAMLINK input format 'DUAL 12-BIT'
		//unsigned int mode = 7;
		//Fg_setParameter(fg, FG_CAMERA_LINK_CAMTYPE, &mode, cam_port);
		//if (Fg_setParameter(fg, FG_CAMERA_LINK_CAMTYPE, &mode, cam_port) < 0 ) {
		//	fprintf(stderr, " [-] Fg_setParameter(FG_CAMERA_LINK_CAMTYPE) failed: %s\n", Fg_getLastErrorDescription(fg));
		//	Fg_FreeMemEx(fg, mem_hndl);
		//	CloseDisplay(disp_id0);
		//	Fg_FreeGrabber(fg);
		//	getchar();
		//	return FG_ERROR;
		//}
		//// Set exposure
		//float exposure = 3000;
		//if(Fg_setParameter(fg, FG_EXPOSURE, &exposure, cam_port) < 0){
		//	fprintf(stderr, " [-] Fg_setParameter(FG_EXPOSURE) failed: %s\n", Fg_getLastErrorDescription(fg));
		//}

		int bit_alignment = FG_LEFT_ALIGNED;
		if (Fg_setParameter(fg, FG_BITALIGNMENT, &bit_alignment, cam_port) < 0) {
			fprintf(stderr, " [-] Fg_setParameter(FG_FG_BITALIGNMENTHEIGHT) failed: %s\n", Fg_getLastErrorDescription(fg));
			Fg_FreeMemEx(fg, mem_hndl);
			CloseDisplay(disp_id0);
			Fg_FreeGrabber(fg);
			return;
		}



		if ((Fg_AcquireEx(fg, cam_port, no_of_pictures_to_grab, ACQ_STANDARD, mem_hndl)) < 0) {
			fprintf(stderr, " [-] Fg_AcquireEx() failed: %s\n", Fg_getLastErrorDescription(fg));
			Fg_FreeMemEx(fg, mem_hndl);
			CloseDisplay(disp_id0);
			Fg_FreeGrabber(fg);
			return;
		}

		frameindex_t last_pic_no = 0;
		frameindex_t cur_pic_no;
		int timeout = 4;
		while ((cur_pic_no = Fg_getLastPicNumberBlockingEx(fg, last_pic_no + 1, cam_port, timeout, mem_hndl)) < no_of_pictures_to_grab) {
			if (cur_pic_no < 0) {
				fprintf(stderr, " [-] Fg_getLastPicNumberBlockingEx(%li) failed: %s\n", last_pic_no + 1, Fg_getLastErrorDescription(fg));
				Fg_stopAcquire(fg,cam_port);
				Fg_FreeMemEx(fg, mem_hndl);
				CloseDisplay(disp_id0);
				Fg_FreeGrabber(fg);
				return;
			}
			last_pic_no = cur_pic_no;
			DrawBuffer(disp_id0, Fg_getImagePtrEx(fg, last_pic_no, cam_port, mem_hndl), static_cast<int>(last_pic_no), "");
			//WriteFITS("test.fits", static_cast<unsigned char*>(Fg_getImagePtrEx(fg, last_pic_no, cam_port, mem_hndl)), width, height);
		}

		CloseDisplay(disp_id0);
		Fg_stopAcquire(fg, cam_port);
		Fg_FreeMemEx(fg, mem_hndl);
		Fg_FreeGrabber(fg);


















//LARGE_INTEGER starting_time, ending_time, elapsed_microseconds;
//LARGE_INTEGER frequency;
//QueryPerformanceFrequency(&frequency); 
//QueryPerformanceCounter(&starting_time);
//
//		while( true ) {
//			boost::posix_time::ptime timestamp = boost::posix_time::microsec_clock::universal_time();
//			
//			boost::this_thread::interruption_point();
//		}
	} catch(const boost::thread_interrupted&) {
		std::cout << "\n  [+] Oops, Interrupted" << std::endl;

		std::cout << "\n  [+] Camera Stop Acquisition.\n";
	}

	int Camera::Start() {
		if ( !thread_ || !thread_->joinable() ) {
			thread_ = new boost::thread( boost::ref(*this) );
		}
		return 0;
	}

	int Camera::Join() {
		if ( thread_ && thread_->joinable() ) {
			thread_->interrupt();
			thread_->join();
		}
		return 0;
	}

	void Camera::ShutDown() {
		if (!connection_lost_) {

			connection_lost_ = true;
		}
	}

	int Camera::Connect() {
		connection_lost_ = false;
		return 0;
	}
}
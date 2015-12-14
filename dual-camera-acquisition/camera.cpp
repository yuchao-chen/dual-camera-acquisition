#include <Windows.h>

#include "camera.h"

#include <iostream>

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>

#include "board-and-dll-chooser.h"
#include "buffer.h"

namespace ic {
	Camera::Camera( data::SynchronisedQueue< data::Buffer > *data_que )
		:thread_( NULL ),
		data_que_( data_que ),
		connection_lost_(true),
		fixed_frame_(450),
		enable_saving_(false) {
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
	void Camera::EnableSaving(bool v) {
		enable_saving_ = v;
	}
	void Camera::operator() () try {
		if (connection_lost_) {
			return;
		}

		int sample_per_pixel				= 1;
		size_t byte_per_sample				= 1;
		frameindex_t no_of_pictures_to_grab	= 1000;
		frameindex_t no_of_buffers			= 4;

		disp_id_ = CreateDisplay(8*byte_per_sample*sample_per_pixel, width_, height_);
		//std::cout << "DISPLAY ID: " << disp_id_ << std::endl;
		SetBufferWidth(disp_id_, width_, height_);
		size_t total_buffer_size = width_ * height_ * sample_per_pixel * byte_per_sample * no_of_buffers;
		mem_hndl_ = Fg_AllocMemEx(fg_, total_buffer_size, no_of_buffers);
		if (mem_hndl_ == NULL) {
			fprintf(stderr, " [-] Error in Fg_AllocMemEx: %s\n", Fg_getLastErrorDescription(fg_));
			CloseDisplay(disp_id_);
			Fg_FreeGrabber(fg_);
			return;
		}

		// Image width of the acquisition window.
		if (Fg_setParameter(fg_, FG_WIDTH, &width_, cam_port_) < 0 ) {
			fprintf(stderr, " [-] Fg_setParameter(FG_WIDTH) failed: %s\n", Fg_getLastErrorDescription(fg_));
			Fg_FreeMemEx(fg_, mem_hndl_);
			CloseDisplay(disp_id_);
			Fg_FreeGrabber(fg_);
			return;
		}

		// Image height of the acquisition window.
		if (Fg_setParameter(fg_,FG_HEIGHT,&height_,cam_port_) < 0 ) {
			fprintf(stderr, " [-] Fg_setParameter(FG_HEIGHT) failed: %s\n", Fg_getLastErrorDescription(fg_));
			Fg_FreeMemEx(fg_, mem_hndl_);
			CloseDisplay(disp_id_);
			Fg_FreeGrabber(fg_);
			return;
		}

		// Set CAMLINK input format 'DUAL 12-BIT'
		unsigned int mode = FG_CL_DUALTAP_12_BIT;
		if (Fg_setParameter(fg_, FG_CAMERA_LINK_CAMTYPE, &mode, cam_port_) < 0) {
			fprintf(stderr, " [-] Fg_setParameter(FG_CAMERA_LINK_CAMTYPE) failed: %s\n", Fg_getLastErrorDescription(fg_));
		}
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
		if (Fg_setParameter(fg_, FG_BITALIGNMENT, &bit_alignment, cam_port_) < 0) {
			fprintf(stderr, " [-] Fg_setParameter(FG_FG_BITALIGNMENTHEIGHT) failed: %s\n", Fg_getLastErrorDescription(fg_));
			Fg_FreeMemEx(fg_, mem_hndl_);
			CloseDisplay(disp_id_);
			Fg_FreeGrabber(fg_);
			return;
		}
		
		if ((Fg_AcquireEx(fg_, cam_port_, GRAB_INFINITE, ACQ_STANDARD, mem_hndl_)) < 0) {
			fprintf(stderr, " [-] Fg_AcquireEx() failed: %s\n", Fg_getLastErrorDescription(fg_));
			Fg_FreeMemEx(fg_, mem_hndl_);
			CloseDisplay(disp_id_);
			Fg_FreeGrabber(fg_);
			return;
		}

		frameindex_t last_pic_no = 0;
		frameindex_t cur_pic_no;
		int timeout = 4;
		int index = 0;
		std::string obv_start_timestamp = "";
		std::string port_name = "PORTA";
		if (cam_port_ == 1) {
			port_name = "PORTB";
		}
		while ((cur_pic_no = Fg_getLastPicNumberBlockingEx(fg_, last_pic_no + 1, cam_port_, timeout, mem_hndl_))) {
			if (cur_pic_no < 0) {
				fprintf(stderr, " [-] Fg_getLastPicNumberBlockingEx(%li) failed: %s\n", last_pic_no + 1, Fg_getLastErrorDescription(fg_));
				Fg_stopAcquire(fg_, cam_port_);
				Fg_FreeMemEx(fg_, mem_hndl_);
				CloseDisplay(disp_id_);
				Fg_FreeGrabber(fg_);
				return;
			}
			last_pic_no = cur_pic_no;
			boost::posix_time::ptime timestamp = boost::posix_time::microsec_clock::universal_time();
			DrawBuffer(disp_id_, Fg_getImagePtrEx(fg_, last_pic_no, cam_port_, mem_hndl_), static_cast<int>(last_pic_no), "");
			if (enable_saving_) {
				data::Buffer buf;
				buf.config = data::AttributeTable::create();
				if ( obv_start_timestamp == "" ) {
					obv_start_timestamp = boost::posix_time::to_iso_string(timestamp);
				}
				buf.config->insert( "ObvStartTimestamp", obv_start_timestamp );
				buf.config->insert( "Timestamp", boost::posix_time::to_iso_string(timestamp) );
				buf.config->insert( "FrameIndex", index );
				buf.config->insert( "Width", width_ );
				buf.config->insert( "Height", height_ );
				buf.config->insert("PortName", port_name);
				int size = width_ * height_;
				buf.data = new unsigned short[size]();
				unsigned char *p = static_cast<unsigned char*>(Fg_getImagePtrEx(fg_, last_pic_no, cam_port_, mem_hndl_));
				for (int i = 0; i < size; i++) {
					buf.data[i] = p[i];
				}
				data_que_->Enqueue(buf);
				index++;
				if (index % fixed_frame_ == 0) {
					data_que_->WaitForEmpty();
					//std::cout << "WAITING FOR EMPTY!" << std::endl;
				}
			}
			boost::this_thread::interruption_point();
		}

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
		CloseDisplay(disp_id_);
		Fg_stopAcquire(fg_, cam_port_);
		Fg_FreeMemEx(fg_, mem_hndl_);
		Fg_FreeGrabber(fg_);
		mem_hndl_ = NULL;
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

	int Camera::Connect(int port) {
		connection_lost_	= true;
		fg_					= NULL;
		int board_no		= SelectBoardDialog();
		cam_port_			= port;

		width_					= 1024;
		height_					= 1024;
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
		if ((fg_ = Fg_Init(applet, board_no)) == NULL) {
			fprintf(stderr, " [-] Error in Fg_Init: %s\n", Fg_getLastErrorDescription(NULL));
			return -1;
		}

		connection_lost_ = false;
		return 0;
	}
}
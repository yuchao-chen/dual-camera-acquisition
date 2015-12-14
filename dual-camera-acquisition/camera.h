#ifndef CAMERA_H
#define CAMERA_H

#include <boost/thread/thread.hpp>

#include <fgrab_struct.h>
#include <fgrab_prototyp.h>
#include <fgrab_define.h>
#include <SisoDisplay.h>
#include <fitsio.h>

#include "buffer.h"
#include "sync_que.h"

namespace ic {
	class Camera {
	public:
		Camera( data::SynchronisedQueue< data::Buffer > *data_que );
		~Camera( void );

		void operator() ();
		int Join();
		int Start();

		void ShutDown();
		int Connect(int port);
		
		void EnableSaving(bool v);
	private:
		boost::thread *thread_;
		data::SynchronisedQueue< data::Buffer > *data_que_;

		bool connection_lost_;
		unsigned int width_, height_;
		int fixed_frame_;
		bool enable_saving_;

		Fg_Struct *fg_;
		int cam_port_;
		int disp_id_;
		dma_mem *mem_hndl_;
	};
}
#endif
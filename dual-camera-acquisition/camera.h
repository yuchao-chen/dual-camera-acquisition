#ifndef CAMERA_H
#define CAMERA_H

#include <boost/thread/thread.hpp>

#include "buffer.h"
#include "sync_que.h"

namespace ic {
	class Camera {
	public:
		enum Constants {
			kNumberOfBuffers = 10,
			kTemperature = 0
		};

		Camera( data::SynchronisedQueue< data::Buffer > *data_que );
		~Camera( void );

		void operator() ();
		int Join();
		int Start();

		void ShutDown();
		int Connect();

	private:
		boost::thread *thread_;
		data::SynchronisedQueue< data::Buffer > *data_que_;

		bool connection_lost_;
		int width_, height_;
		int fixed_frame_;
	};
}
#endif
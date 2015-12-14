#ifndef DATA_HANDLE_H
#define DATA_HANDLE_H

#include <boost/thread/thread.hpp>
#include "buffer.h"
#include "sync_que.h"
namespace dh {
	class DataHandle {
	public:
		DataHandle( data::SynchronisedQueue< data::Buffer > *data_que );
		~DataHandle();

		void operator() ();
		int Join();
		int Start();
	private:
		void SaveFITS( data::Buffer &buf );
		
		data::SynchronisedQueue< data::Buffer > *data_que_;
		boost::thread *thread_;

		std::string local_driver_;
	};
}
#endif
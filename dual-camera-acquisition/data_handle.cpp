#include "data_handle.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>

#include <fitsio.h>

#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/system/error_code.hpp>

namespace dh {
	std::string ZeroPadNumber(int num, int digit_num) {
		std::ostringstream ss;
		ss << std::setw(digit_num) << std::setfill('0') << num;
		std::string result = ss.str();
		if (result.length() > digit_num) {
			result.erase(0, result.length() - digit_num);
		}
		return result;
	}

	DataHandle::DataHandle( data::SynchronisedQueue< data::Buffer > *data_que ) 
		:thread_( NULL ),
		data_que_( data_que ),
		local_driver_( "J:/" ) {
			//std::cout << "DataHandle()" << std::endl;
	}
	DataHandle::~DataHandle() {
		//std::cout << "~DataHandle()" << std::endl;
		if ( thread_ ) {
			thread_->interrupt();
			thread_->join();
			delete thread_;
		}
	}

	void DataHandle::operator() () try {
		while( true ) {
			data::Buffer tmp = data_que_->Dequeue();
			//std::cout << " [+] FUCK!" << std::endl;
			SaveFITS( tmp );
			delete[] tmp.data;
			//std::cout << "\n Camera's Grabing Image." << std::endl;
			boost::this_thread::interruption_point();
		}
	} catch( const boost::thread_interrupted& ) {
		std::cout << "\n  [+] Oops, Interrupted" << std::endl;
		while ( !data_que_->Empty() ) {
			data::Buffer tmp = data_que_->Dequeue();
			SaveFITS( tmp );
			delete[] tmp.data;
		}
		std::cout << "\n  [+] Stop Handling Data.\n";
	}

	int DataHandle::Join() {
		if ( thread_ && thread_->joinable() ) {
			thread_->interrupt();
			thread_->join();
		}
		return 0;
	}
	int DataHandle::Start() {
		if ( !thread_ || !thread_->joinable() ) {
			thread_ = new boost::thread( boost::ref(*this) );
		}
		return 0;
	}

	void DataHandle::SaveFITS( data::Buffer &buf ) {
		//std::cout << buf.config->get_string( "Timestamp" );
		//std::string obv_start_timestamp = buf.config->get_string( "ObvStartTimestamp" );
		//std::string timestamp = buf.config->get_string( "Timestamp" );
		boost::posix_time::ptime obv_start_timestamp = boost::posix_time::from_iso_string( buf.config->get_string( "ObvStartTimestamp" ) );
		boost::posix_time::ptime timestamp = boost::posix_time::from_iso_string( buf.config->get_string( "Timestamp" ) );

		std::string path = local_driver_;
		path += ZeroPadNumber( obv_start_timestamp.date().year(), 4 ) + ZeroPadNumber( obv_start_timestamp.date().month(), 2 ) + ZeroPadNumber( obv_start_timestamp.date().day(), 2 ) + "/";
		path += ZeroPadNumber( obv_start_timestamp.time_of_day().hours(), 2 ) + ZeroPadNumber( obv_start_timestamp.time_of_day().minutes(), 2 ) + ZeroPadNumber( obv_start_timestamp.time_of_day().seconds(), 2 ) + "/";
		boost::system::error_code error;
		boost::filesystem::create_directories( path, error );

		path += "H_" + ZeroPadNumber( buf.config->get_long( "FrameIndex" ), 6 ) + ".fits";
		fitsfile *fptr;

		int status = 0;
		int bitpix = USHORT_IMG;
		long naxis = 2;
		long naxes[2];

		naxes[0] = buf.config->get_long( "Width" );
		naxes[1] = buf.config->get_long( "Height" );

		remove( path.c_str() );

		if (fits_create_file(&fptr, path.c_str(), &status)) {
			printf("Cannot create file: Error %ld",status);
		}

		if (fits_create_img(fptr, bitpix, naxis, naxes, &status)) {
			printf("Cannot create image: Error %ld", status);
		}
		long fpixel = 1;
		long nelements = naxes[0] * naxes[1];

		if (fits_write_img(fptr, TUSHORT, fpixel, nelements, buf.data, &status)) {
			printf("Cannot write file: Error %ld", status);
		}

		if (fits_close_file(fptr, &status)) {
			printf("Cannot close file: Error %ld", status);
		}
	}
}
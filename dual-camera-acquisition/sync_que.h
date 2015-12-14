#ifndef SYNC_QUE_H
#define SYNC_QUE_H

#include <queue>

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace data {
		template <typename T>
		class SynchronisedQueue {
		public:
			// Add data to the queue and notify others
			void Enqueue(const T& data){
				// Acquire lock on the queue
				boost::unique_lock<boost::mutex> lock(mutex_);
				// Add the data to the queue
				queue_.push(data);
				// Notify others that data is ready
				full_cond_.notify_one();
			} // Lock is automatically released here

			// Get data from the queue. Wait for data if not available
			T Dequeue()  {
				// Acquire lock on the queue
				boost::unique_lock<boost::mutex> lock(mutex_);
				// When there is no data, wait till someone fills it.
				// Lock is automatically released in the wait and obtained
				// again after the wait
				while (queue_.size() == 0) {
					full_cond_.wait(lock);
				}
				// Retrieve the data from the queue
				T result = queue_.front(); 
				queue_.pop();
				if (queue_.size() == 0) {
					empty_cond_.notify_all();
				}
				//std::cout << queue_.size() << std::endl;;
				return result;
			} // Lock is automatically released here

			bool Empty() {
				boost::unique_lock<boost::mutex> lock(mutex_);
				if (queue_.size() != 0) {
					return false;
				}
				return true;
			}

			void WaitForEmpty() {
				//std::cout << "Waiting for Empty" << std::endl;
				boost::unique_lock<boost::mutex> lock(mutex_);
				while (queue_.size() != 0) {
					empty_cond_.wait(lock);
				}
			}

		private:
			std::queue<T> queue_; // Use STL queue to store data
			boost::mutex mutex_; // The mutex to synchronise on
			boost::condition_variable full_cond_; // The condition to wait for
			boost::condition_variable empty_cond_;
		};
}
#endif
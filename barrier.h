//Серенко Никита 491

#include <mutex>
#include <atomic>


class barrier {   
public:   
    explicit barrier(size_t num_threads): capasity(num_threads), era(0), size(0){}   
    void enter(){
    	std::unique_lock<std::mutex> lock(mtx);
    	size_t era_of_thread = era;
    	if (++size == capasity)
    	{
    		era++;
    		size = 0;
    		cv.notify_all();
    		
    	}
    	else
    	{	
    		cv.wait(lock, [this, era_of_thread]{return era_of_thread != era;});
    	}

    }  
private:
	std::mutex mtx;
	size_t capasity;
	size_t era;
	std::condition_variable cv;
	size_t size;
};
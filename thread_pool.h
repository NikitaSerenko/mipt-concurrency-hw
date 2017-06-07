//Cеренко Никита 491

#include <mutex>
#include <atomic>
#include <deque>
#include <functional>
#include <future>
#include <exception>
#include <stdexcept>

template <class Value, class Container = std::deque<Value>>   
class thread_safe_queue {   
public:   
    explicit thread_safe_queue(size_t size) : capasity(size), shutdown_flag(false) {}   
    void enqueue(Value&& v){
    	std::unique_lock<std::mutex> lock(mtx);
    	if(shutdown_flag.load() == true) 
    		throw std::exception();
    	if (Q.size() == capasity){
    		not_full.wait(lock, [this](){
    			if (shutdown_flag.load() == true)
    				throw std::exception();
    			return (Q.size() != capasity);});
    	}
    	Q.push_back(std::move(v));
    	not_empty.notify_one(); 
    }   
    explicit thread_safe_queue(){}
    void pop(Value& v){
    	std::unique_lock<std::mutex> lock(mtx);
    	if (Q.empty()){
    		if (shutdown_flag.load() == true)
    			throw std::exception();
    		not_empty.wait(lock, [this](){
    			if(shutdown_flag.load() == true  && Q.empty()) 
    				throw std::exception();
    			return (!Q.empty());}); 
    	}
    	v = std::move(Q.front());
    	Q.pop_front();
    	not_full.notify_one();
    }   
    void shutdown(){
    	shutdown_flag.store(true);
    	not_empty.notify_all();
    	not_full.notify_all();
    }
    thread_safe_queue(const thread_safe_queue &) = delete;
	void operator = (const thread_safe_queue&) = delete;	
private:
	size_t capasity;
	std::condition_variable not_full;
	std::condition_variable not_empty;
	Container Q;
	std::mutex mtx;
	std::atomic<bool> shutdown_flag;
};



template <class R>
class task {
public:
    std::function <R()> funct;
    std::promise <R> prms;
    task(std::function <R()> _funct, std::promise <R> _prms): funct(_funct), prms(std::move(_prms)) {} 
    task(){}
};
    
template <class R>   
class thread_pool {   
public:   
    explicit thread_pool(size_t num_workers):number_of_threads(num_workers), _shutdown_flag(false){
        for (size_t i = 0; i < num_workers; i++) {
            vector_of_threads.emplace_back([this](){
                while (true) {
                    try {
                        task <R> get_task;
                        Q.pop(get_task);
                        try {
                            get_task.prms.set_value(get_task.funct());
                        }
                        catch (...) {
                            get_task.prms.set_exception(std::current_exception());
                        }
                    }
                    catch (...) {
                        return;
                    }
                }
            });
        }
    }  
    std::future<R> submit(std::function<R()> _funct){
        if (_shutdown_flag.load())
            throw std::exception();
        std::promise <R> _promise;
        std::future <R> _future = _promise.get_future();
        task<R> _task(_funct, std::move(_promise));
        Q.enqueue(std::move(_task));
        return std::move(_future);
    }  
    void shutdown(){
        Q.shutdown();
        _shutdown_flag.store(true);
        for (size_t i = 0; i < vector_of_threads.size(); i++)
            vector_of_threads[i].join();
    }
private:
    size_t number_of_threads;
    std::vector <std::thread> vector_of_threads;
    thread_safe_queue <task<R>> Q;
    std::atomic<bool> _shutdown_flag; 
};


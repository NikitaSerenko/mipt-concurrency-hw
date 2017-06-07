#include <iostream>
#include <atomic>
#include <vector>

template<class Value> 
class spsc_ring_buffer{
public:
	explicit spsc_ring_buffer(size_t size): capacity(size + 1), buf(std::vector<Value>(capacity)), head(0), tail(0){}

	bool enqueue(Value v){
		auto current_tail = tail.load(std::memory_order_relaxed);//тк поток producer один, то только он и вызывает эту функцию, а значит имеет новейшее значение tail c помощью relaxed 
		auto next_tail = (current_tail + 1) % capacity;
		
		if (next_tail == head.load(std::memory_order_acquire)) return false;//acquire-release synchronized.synchronizes-with как раз гарантирует, что поток, прочитавший значение, будет видеть все записи в память из другого потока до записи того значения, которое он прочитал
		
		buf[current_tail] = std::move(v);//случиться до release store и гарантирует что изменение вектора случиться до инкремента 
		tail.store(next_tail, std::memory_order_release);//Pair-synchronization с Consumer-thread
		return true;			
	};

	bool dequeue(Value& v){
		//Аналогично
		auto current_head = head.load(std::memory_order_relaxed);
		auto next_head = (current_head + 1) % capacity;

		if (current_head == tail.load(std::memory_order_acquire)) return false;

		v = std::move(buf[current_head]);
		head.store(next_head, std::memory_order_release);
		return true;
	};
private:
	size_t capacity;
	std::vector<Value> buf;
	std::atomic<size_t> head;
	std::atomic<size_t> tail;
};

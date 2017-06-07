#include <thread>
#include <atomic>
#include <array>
#include <vector>
#include <stack>

class peterson_mutex {
public:
	peterson_mutex() {
		want[0].store(false);
		want[1].store(false);
		victim.store(0);
	}

	void lock(int t) {
		want[t].store(true);
		victim.store(t);
		while (want[1 - t].load() && victim.load() == t) {
			std::this_thread::yield();
		}
	}

	void unlock(int t) {
		want[t].store(false);
	}
	
private:
	std::array<std::atomic<bool>, 2> want;
	std::atomic<int> victim;
};

class tree_mutex {
public:
	tree_mutex(size_t num_threads) {
		int n = (1 << (log(n - 1) + 1));
		size = 2 * n;
		peterson_mutex_tree = std::vector<peterson_mutex>(size, peterson_mutex());
	}

	size_t parent(size_t thread_index) {
		return thread_index / 2;
	}

	void lock(size_t thread_index) {
		thread_index += size;
		size_t curr_index = thread_index;
		do
		{
			size_t p_index = parent(index);
			if (curr_index % 2 == 0) 
				peterson_mutex_tree[p_index].lock(0);
			else 
				peterson_mutex_tree[p_index].lock(1);
			curr_index = p_index;
		} 
		while (index != 1);
	}

	void unlock(size_t thread_index) {
		std::stack<int> way_to_stream;
		std::stack<bool> stream;
		thread_index += peterson_mutex_tree.size();
		size_t curr_index = thread_index;
		while (cur_index != 1)
		{
			if (cur_index % 2 == 0) 
				stream.push(0);
			else 
				stream.push(1);
			cur_index = parent(cur_index);
			way_to_stream.push(cur_index);
		}
		while (!way_to_stream.empty())
		{
			cur_index = way_to_stream.top();
			way_to_stream.pop();
			bool cur_stream = stream.top();
			stream.pop();
			peterson_mutex_tree[cur_index].unlock(cur_stream);
		}
	}

private:
	std::vector<peterson_mutex> peterson_mutex_tree;
	size_t size;
};
#include <iostream>
#include <atomic>
#include <mutex>
#include <atomic>
#include <shared_mutex>
#include <forward_list>
#include <vector>



template <class Value, class Hash = std::hash<Value>>   
class striped_hash_set {   
public:   
    explicit striped_hash_set(size_t n_stripes, float l_factor = 0.8, size_t g_factor = 15):size(0), number_of_stripes(n_stripes), number_of_buckets(n_stripes), growth_factor(g_factor), load_factor(l_factor){
    	hash_funct = Hash();
		vector_of_buckets = std::vector<std::forward_list<std::pair<Value, int>>>(n_stripes);
		vector_of_stripes = std::vector<std::shared_timed_mutex>(n_stripes);
    } 


    void add(const Value& v){
    	//if (contains(v)) return;

    	auto hash_value = hash_funct(v);
		auto stripe = hash_value % number_of_stripes;
		

		std::unique_lock<std::shared_timed_mutex> write_lock(vector_of_stripes[stripe]);
		auto bucket = hash_value % number_of_buckets.load();
		vector_of_buckets[bucket].push_front(std::make_pair(v, hash_value));
		size += 1;
		write_lock.unlock();

		if (size / number_of_buckets.load() > load_factor) update();
    }


    void remove(const Value& v){
    	auto hash_value = hash_funct(v);
		auto stripe = hash_value % number_of_stripes;
		

		std::unique_lock<std::shared_timed_mutex> write_lock(vector_of_stripes[stripe]);
		auto bucket = hash_value % number_of_buckets.load();
		vector_of_buckets[bucket].remove(std::make_pair(v, hash_value));
		size -= 1;
		write_lock.unlock();
    }


    bool contains(const Value& v){
    	auto hash_value = hash_funct(v);
		auto stripe = hash_value % number_of_stripes;
		

		bool f = false;
		std::shared_lock<std::shared_timed_mutex> read_lock(vector_of_stripes[stripe]);
		auto bucket = hash_value % number_of_buckets.load();
		for (auto i = vector_of_buckets[bucket].begin(); i != vector_of_buckets[bucket].end(); i++){
			if ((*i).first == v)
			{
				f = true;
				break;
			}
		}
		read_lock.unlock();
		return f;
    }


	void update(){
		auto copy_number_of_buckets = number_of_buckets.load();
		std::vector<std::unique_lock<std::shared_timed_mutex> > all_lock;

		for (size_t i = 0; i < number_of_stripes; i++) {
			all_lock.emplace_back(std::unique_lock<std::shared_timed_mutex>(vector_of_stripes[i]));
		}

		if (copy_number_of_buckets != number_of_buckets.load()) return;

		number_of_buckets = number_of_buckets.load() * growth_factor;
		std::vector<std::forward_list<std::pair<Value, int>>> new_vector_of_buckets(number_of_buckets.load());

		for (auto i = vector_of_buckets.begin(); i != vector_of_buckets.end(); i++)
		{
			for (auto j = (*i).begin(); j != (*i).end(); j++) {
				auto hash_value = (*j).second;
				new_vector_of_buckets[hash_value % number_of_buckets.load()].push_front(*j);
			}
		}

		std::swap(new_vector_of_buckets, vector_of_buckets);
	}
private:
	Hash hash_funct;
	std::atomic<size_t> size;
	size_t number_of_stripes;
	std::atomic<size_t> number_of_buckets;
	size_t growth_factor;
	double load_factor;
	std::vector<std::forward_list<std::pair<Value, int>>> vector_of_buckets;
	std::vector<std::shared_timed_mutex> vector_of_stripes;
};
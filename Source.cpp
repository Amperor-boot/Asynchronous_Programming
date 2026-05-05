#include <algorithm>
#include <functional>
#include <future>
#include <iostream>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std;

//	================================================	//
//	First task:	Selection Sort with async min search	//
//	================================================	//

// Asynchronous function to find the index of the min element in a subrange
template <typename RandomIt>
void async_get_min_index(RandomIt begin, RandomIt end, promise<size_t> outcome_of_promise)
{
	try
	{
		if (begin == end)
			throw invalid_argument("Empty range passed to find min index");

		auto minIt = min_element(begin, end);
		size_t id = static_cast<size_t>(distance(begin, minIt));
		outcome_of_promise.set_value(id);
	}
	catch (...)
	{
		// passing any exception to the future
		outcome_of_promise.set_exception(current_exception());
	}
}

template <typename RandomIt>
void async_selection_sort(RandomIt begin, RandomIt end)
{
	size_t amount = static_cast<size_t>(distance(begin, end));
	for (size_t i = 0; i < amount; ++i)
	{
		// creating promise/future pair
		promise<size_t> p;
		future<size_t> f = p.get_future();

		// running async task to find min index in the unsorted part
		thread t(async_get_min_index<RandomIt>, i + begin, end, move(p));

		// waiting for outcome
		size_t localMinIndex = f.get();	// may throw if async task failed
		t.join();

		size_t minIndex = i + localMinIndex;	//	global index

		// swapping the found min element with the current position
		if (i != minIndex)
			iter_swap(i + begin, begin + minIndex);
	}
}

//	=================================	//
//	Second task:	Parallel for_each	//
//	=================================	//

template <typename ChosenIt, typename Function>
void parallel4each(ChosenIt first, ChosenIt last, Function&& f, size_t min_per_thread = 1000)
{
	size_t length = static_cast<size_t>(distance(first, last));
	if (length == 0) return;

	if (length >= min_per_thread)
	{
		ChosenIt middle = first;
		advance(middle, 0.5 * length);

		// running first half in a separate async task
		auto handle = async(launch::async, [=, &f]()
			{
				parallel4each(first, middle, f, min_per_thread);
			});

		// processing second half in current thread
		parallel4each(middle, last, forward<Function>(f), min_per_thread);

		// waiting for first half to finish
		handle.get();
	}
	else
		for_each(first, last, f);
}

void print_vector(vector<int16_t>& data)
{
	for (auto i : data)
		cout << i << " ";
	cout << endl;
}

//	==============================	//
//	Executing the main entry point	//
//	==============================	//
int main(int argc, char** argv)
{
	try
	{
		//	First task:	Selection Sort with async min search	//
		vector<int16_t> data = { -32, 7, -64, 777, -128 };
		cout << "Original unsorted array:\t";
		print_vector(data);

		async_selection_sort(data.begin(), data.end());

		cout << "Modified sorted array:\t\t";
		print_vector(data);
		cout << endl;

		//	Second task:	Parallel for_each	//
		vector<int16_t> numbers(30);
		iota(numbers.begin(), numbers.end(), 1);

		cout << "Before parallel for_each:\t";
		print_vector(numbers);

		parallel4each(numbers.begin(), numbers.end(), [](int16_t& x)
			{x *= 2; }, 5);

		cout << "After parallel for_each:\t";
		print_vector(numbers);
	}
	catch (const exception& ex)
	{
		cerr << "Error:\t" << ex.what() << endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
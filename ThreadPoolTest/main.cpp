#include<iostream>
#include<thread>
#include<condition_variable>
#include<future>
#include<atomic>
#include<vector>
#include<queue>
using namespace std;
typedef function<void()> Task;
class ThreadPool
{
public:
	ThreadPool::ThreadPool(size_t threads)
		: stop(false)
	{
		for (size_t i = 0; i<threads; ++i)
			workers.emplace_back(
			[this]
		{
			while (true)
			{
				function<void()> task;

				{
					unique_lock<mutex> lock(this->queue_mutex);
					this->condition.wait(lock,[this]{ return this->stop || !this->tasks.empty(); });
					if (this->stop && this->tasks.empty())
						return;
					task = tasks.front();
					this->tasks.pop();
				}

				task();
			}
		}
		);
	}
	template<class F, class... Args>
	auto enqueue(F f, Args... args)
		->future<typename result_of<F(Args...)>::type>
	{
		typedef typename result_of<F(Args...)>::type  return_type;
		auto task = make_shared<packaged_task<return_type()> >(bind(f, args...));
		future<return_type> res = task->get_future();
		{
			unique_lock<mutex> lock(queue_mutex);

			// 关闭线程池就不允许添加任务
			if (stop)
				throw runtime_error("enqueue on stopped ThreadPool");

			tasks.emplace([task](){ (*task)(); });
		}
		condition.notify_one();
		return res;
	}
	~ThreadPool()
	{
		{
			unique_lock<std::mutex> lock(queue_mutex);
			stop = true;
		}
		condition.notify_all();
		for (thread &worker : workers)
			worker.join();
	}
private:
	//线程池
	vector<thread> workers;
	//任务队列
	queue<function<void()>> tasks;//************
	//实现同步
	mutex queue_mutex;
	condition_variable condition;
	//关闭线程池
	bool stop;

};
int dosomething(int i)
{
	cout << "hello " << i << endl;
	this_thread::sleep_for(chrono::seconds(1));
	cout << "world " << i << endl;
	return i*i;
}
class A
{
public:
	A(int b)
	{
		cout << "construction" << endl;
	}
	A(const A &a){ cout << "copy construction" << endl; }
};
int main()
{

	ThreadPool pool(4);
	vector< std::future<int> > results;
	for (int i = 0; i < 8; i++)
	{
		results.emplace_back(pool.enqueue(dosomething, i));
	}
	for (auto && result : results)
		cout << result.get() << ' ';
	cout << std::endl;
	return 0;
}
#ifndef JOB_H
#define JOB_H

#include <defines.h>

#include <future>
#include <functional>
#include <type_traits>

#include <queue>
#include <vector>

class CORE_API JobQueue
{
public:
	using Job = std::function<void()>;

	JobQueue(size_t num_threads = std::thread::hardware_concurrency(), size_t max_queue_size = 0);
	~JobQueue();

	// 禁止拷贝和移动
	JobQueue(const JobQueue&) = delete;
	JobQueue& operator=(const JobQueue&) = delete;
	JobQueue(JobQueue&&) = delete;
	JobQueue& operator=(JobQueue&&) = delete;

	// 提交任务到队列
	template <typename F, typename... Args>
	auto submit(F&& f, Args&&... args)
		-> std::future<typename std::invoke_result<F, Args...>::type>;

	// 获取队列中的任务数量
	size_t getQueueSize() const;

	// 获取工作线程数量
	size_t getThreadCount() const;

	// 获取最大队列大小
	size_t getMaxQueueSize() const;

	// 设置最大队列大小
	void setMaxQueueSize(size_t max_size);

	// 等待所有任务完成
	void waitForAll();

	// 停止任务队列
	void stop();

	// 检查是否已停止
	bool isStopped() const;

private:
	void workerThread();

	std::vector<std::thread> _workers;
	std::queue<Job> _tasks;

	mutable std::mutex _queue_mutex;
	std::condition_variable _queue_empty_cv;
	std::condition_variable _queue_full_cv;
	std::condition_variable _finihsed;

	std::atomic<bool> _running;
	std::atomic<size_t> _active_jobs;
	size_t _max_queue_size;
};

template <typename F, typename... Args>
auto JobQueue::submit(F&& f, Args&&... args)
	-> std::future<typename std::invoke_result<F, Args...>::type>
{
	using return_type = typename std::invoke_result<F, Args...>::type;

	auto task = std::make_shared<std::packaged_task<return_type()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...));

	std::future<return_type> result = task->get_future();

	{
		std::unique_lock<std::mutex> lock(_queue_mutex);

		// 等待队列不满（如果设置了最大队列大小）
		_queue_full_cv.wait(lock, [this] {
			return _running || (_max_queue_size == 0 || _tasks.size() < _max_queue_size);
		});

		if (_running)
		{
			throw std::runtime_error("Cannot submit job to stopped queue");
		}

		if (_max_queue_size > 0 && _tasks.size() >= _max_queue_size)
		{
			throw std::runtime_error("Queue is full");
		}

		_tasks.emplace([task]() { (*task)(); });
		++_active_jobs;
	}

	_queue_empty_cv.notify_one();
	return result;
}

#endif // JOB_H
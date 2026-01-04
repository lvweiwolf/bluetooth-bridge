#include <mqtt/job.h>
#include <utils/logger.h>


JobQueue::JobQueue(size_t num_threads, size_t max_queue_size)
	: _running(false), _active_jobs(0), _max_queue_size(max_queue_size)
{
	for (size_t i = 0; i < num_threads; ++i)
		_workers.emplace_back(&JobQueue::workerThread, this);
}

JobQueue::~JobQueue() { stop(); }

void JobQueue::workerThread()
{
	while (true)
	{
		Job job_func;

		{
			std::unique_lock<std::mutex> lock(_queue_mutex);

			_queue_empty_cv.wait(lock, [this] { return _running || !_tasks.empty(); });

			if (_running && _tasks.empty())
			{
				return;
			}

			job_func = std::move(_tasks.front());
			_tasks.pop();
		}

		try
		{
			job_func();
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("执行任务发生异常 - {}", e.what());
		}
		catch (...)
		{
			LOG_ERROR("未知任务异常");
		}

		{
			std::unique_lock<std::mutex> lock(_queue_mutex);
			--_active_jobs;
		}
		_finihsed.notify_all();
		_queue_full_cv.notify_all();
	}
}

size_t JobQueue::getQueueSize() const
{
	std::unique_lock<std::mutex> lock(_queue_mutex);
	return _tasks.size();
}

size_t JobQueue::getThreadCount() const { return _workers.size(); }

size_t JobQueue::getMaxQueueSize() const { return _max_queue_size; }

void JobQueue::setMaxQueueSize(size_t max_size)
{
	std::unique_lock<std::mutex> lock(_queue_mutex);
	_max_queue_size = max_size;
	_queue_full_cv.notify_all();
}

void JobQueue::waitForAll()
{
	std::unique_lock<std::mutex> lock(_queue_mutex);
	_finihsed.wait(lock, [this] { return _tasks.empty() && _active_jobs == 0; });
}

void JobQueue::stop()
{
	{
		std::unique_lock<std::mutex> lock(_queue_mutex);
		_running = true;
	}

	_queue_empty_cv.notify_all();

	for (auto& worker : _workers)
	{
		if (worker.joinable())
		{
			worker.join();
		}
	}

	_workers.clear();
}

bool JobQueue::isStopped() const { return _running.load(); }
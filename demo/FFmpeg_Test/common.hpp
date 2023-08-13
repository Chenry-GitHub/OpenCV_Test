#ifndef FBC_FFMPEG_TEST_COMMON_HPP_
#define FBC_FFMPEG_TEST_COMMON_HPP_

#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <libavutil/error.h>

class Timer {
public:
	static long long getNowTime() { // milliseconds
		auto now = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
	}
};

inline void print_error_string(int value)
{
	static char errbuf[AV_ERROR_MAX_STRING_SIZE];
	memset(errbuf, 0, AV_ERROR_MAX_STRING_SIZE);
	fprintf(stderr, "Error occurred: code: %d, string: %s\n", value, av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, value));
}

typedef struct Buffer {
	unsigned char* data;
	unsigned int length;
} Buffer;

class BufferQueue {
public:
	BufferQueue() = default;
	~BufferQueue() {}

	void push(Buffer& buffer) {
		std::unique_lock<std::mutex> lck(mtx);
		queue.push(buffer);
		cv.notify_all();
	}

	void pop(Buffer& buffer) {
		std::unique_lock<std::mutex> lck(mtx);
		while (queue.empty()) {
			cv.wait(lck);
		}
		buffer = queue.front();
		queue.pop();
	}

	size_t size() const {
		return queue.size();
	}

private:
	std::queue<Buffer> queue;
	std::mutex mtx;
	std::condition_variable cv;
};

class PacketScaleQueue {
public:
	PacketScaleQueue() = default;

	~PacketScaleQueue() {
		Buffer buffer;

		while (getPacketSize() > 0) {
			popPacket(buffer);
			delete[] buffer.data;
		}

		while (getScaleSize() > 0) {
			popScale(buffer);
			delete[] buffer.data;
		}
	}

	void init(unsigned int buffer_num = 16, size_t buffer_size = 1024 * 1024 * 4) {
		for (auto i = 0; i < buffer_num; ++i) {
			Buffer buffer = { new unsigned char[buffer_size], buffer_num};
			pushPacket(buffer);
		}
	}

	void pushPacket(Buffer& buffer) { packet_queue.push(buffer); }
	void popPacket(Buffer& buffer) { packet_queue.pop(buffer); }
	size_t getPacketSize() const { return packet_queue.size(); }

	void pushScale(Buffer& buffer) { scale_queue.push(buffer); }
	void popScale(Buffer& buffer) { scale_queue.pop(buffer); }
	size_t getScaleSize() const { return scale_queue.size(); }

private:
	BufferQueue packet_queue, scale_queue;
};

#endif // FBC_FFMPEG_TEST_COMMON_HPP_


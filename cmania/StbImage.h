#pragma once
#include <utility>
#include <memory>
extern "C" {
	unsigned char* stbi_load(char const* filename, int* x, int* y, int* comp, int req_comp);
	int stbir_resize_uint8(const unsigned char* input_pixels, int input_w, int input_h, int input_stride_in_bytes,
		unsigned char* output_pixels, int output_w, int output_h, int output_stride_in_bytes,
		int num_channels);
}
class Image
{
private:
	unsigned char* scan0 = nullptr;
	int w = 0;
	int h = 0;
	int c = 0;

public:
	// Default constructor
	Image() = default;

	// Constructor with specified width, height, and channels
	Image(int w, int h, int channels = 3)
		: w(w), h(h), c(channels), scan0(new unsigned char[w * h * channels])
	{}

	// Constructor with specified pointer, width, height, and channels
	Image(void* scan0, int w, int h, int channels = 3)
		: w(w), h(h), c(channels), scan0((unsigned char*)scan0)
	{}

	// Constructor with specified filename and channels
	Image(const char* filename, int channels = 3)
	{
		int _;
		scan0 = stbi_load(filename, &w, &h, &_, channels);
		c = channels;
	}

	// Copy constructor
	Image(const Image& other)
		: w(other.w), h(other.h), c(other.c)
	{
		scan0 = new unsigned char[w * h * c];
		std::memcpy(scan0, other.scan0, w * h * c);
	}

	// Move constructor
	Image(Image&& other) noexcept
		: w(other.w), h(other.h), c(other.c), scan0(other.scan0)
	{
		other.scan0 = nullptr;
	}

	// Copy assignment operator
	Image& operator=(Image& rhs)
	{
		if (this != &rhs) {
			if (scan0 != 0)
				delete[] scan0;
			w = rhs.w;
			h = rhs.h;
			c = rhs.c;
			scan0 = rhs.scan0;
			rhs.scan0 = nullptr;
		}
		return *this;
	}

	// Move assignment operator
	Image& operator=(Image&& rhs) noexcept
	{
		if (this != &rhs) {
			if (scan0 != 0)
				delete[] scan0;
			w = rhs.w;
			h = rhs.h;
			c = rhs.c;
			scan0 = rhs.scan0;
			rhs.scan0 = nullptr;
		}
		return *this;
	}

	// Resize the image
	Image Resize(int w, int h)
	{
		auto scan0_2 = new unsigned char[w * h * c];
		stbir_resize_uint8(scan0, this->w, this->h, this->w * this->c, scan0_2, w, h, w * c, c);
		return { scan0_2,w,h ,c };
	}

	unsigned char* Scan0()
	{
		return scan0;
	}
	int Width()
	{
		return w;
	}
	int Height()
	{
		return h;
	}
	int Channels()
	{
		return c;
	}

	// Destructor
	~Image()
	{
		if (scan0 != 0)
			delete[] scan0;
	}
};
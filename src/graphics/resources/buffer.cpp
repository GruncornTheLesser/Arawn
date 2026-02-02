#define ARAWN_IMPLEMENTATION
#include <graphics/resources/buffer.h>


Arawn::Buffer::Buffer(std::size_t size, VK_ENUM(VkBufferUsageFlags) usage, VK_ENUM(VmaMemoryUsage) mem_usage) {
	VkBufferCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 
		.size = size, 
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VmaAllocationCreateInfo alloc {
		.usage = mem_usage
	};

	VK_ASSERT(vmaCreateBuffer(engine.allocator, &info, &alloc, &buffer, &memory, nullptr));
}
Arawn::Buffer::~Buffer() {
	if (buffer != nullptr) {
		vmaDestroyBuffer(engine.allocator, buffer, memory);
	}
}

Arawn::Buffer::Buffer(Buffer&& other) noexcept {
	buffer = other.buffer;
	memory = other.memory;

	other.buffer = nullptr;
}
Arawn::Buffer& Arawn::Buffer::operator=(Buffer&& other) noexcept {
	if (other.buffer != nullptr) {
		vmaDestroyBuffer(engine.allocator, buffer, memory);
	}
	
	buffer = other.buffer;
	memory = other.memory;
	
	other.buffer = nullptr;
	
	return *this;
}
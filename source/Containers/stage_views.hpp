#pragma once
#include "VulkanAPI/barrier.hpp"
#include "VulkanAPI/buffer.hpp"
#include "VulkanAPI/image.hpp"

enum class EResourceOp
{
	WriteOnly,
	ReadWrite,
	ReadOnly
};

class GStageViews
{
public:
	struct image_resource
	{
		std::shared_ptr<GVkImageView> view;
		VkPipelineStageFlagBits2 stage;
		VkAccessFlagBits2 access;
		VkImageLayout layout;
		EResourceOp Op;
	};

	struct buffer_resource
	{
		std::shared_ptr<GVkBufferView> view;
		VkPipelineStageFlagBits2 stage;
		VkAccessFlagBits2 access;
		EResourceOp Op;
	};

public:
	using image_iterator = std::vector<image_resource>::iterator;
	using buffer_iterator = std::vector<buffer_resource>::iterator;

	using image_const_iterator = std::vector<image_resource>::const_iterator;
	using buffer_const_iterator = std::vector<buffer_resource>::const_iterator;

private:
	std::vector<buffer_resource> _buffers;
	std::vector<image_resource> _images;

private:
	bool _merge_buffer(std::shared_ptr<GVkBufferView> view, EResourceOp Op, VkPipelineStageFlagBits2 stage, VkAccessFlagBits2 access)
	{
		for (auto it = _buffers.begin(); it != _buffers.end(); it++)
		{
			if (*view == *it->view)
			{
				assert(Op == it->Op);

				it->access = it->access | access;
				it->stage  = it->stage | stage;

				return true;
			}
		}

		return false;
	}

	bool _merge_image(std::shared_ptr<GVkImageView> view, EResourceOp Op, VkPipelineStageFlagBits2 stage, VkAccessFlagBits2 access)
	{
		for (auto it = _images.begin(); it != _images.end(); it++)
		{
			if (*view == *it->view)
			{
				assert(Op == it->Op);

				it->layout = VK_IMAGE_LAYOUT_GENERAL;
				it->access = it->access | access;
				it->stage  = it->stage | stage;

				return true;
			}
		}

		return false;
	}

public:
	void add(std::shared_ptr<GVkBufferView> view, EResourceOp Op, VkPipelineStageFlagBits2 stage, VkAccessFlagBits2 access)
	{
		if (!_merge_buffer(view, Op, stage, access))
			_buffers.emplace_back(view, stage, access, Op);
	}

	void add(std::shared_ptr<GVkImageView> view, EResourceOp Op, VkImageLayout layout, VkPipelineStageFlagBits2 stage, VkAccessFlagBits2 access)
	{
		for (uint32_t layer = view->BaseLayer(); layer < view->LastLayer(); layer++)
		{
			for (uint32_t level = view->BaseLevel(); level < view->LastLevel(); level++)
			{
				auto newview = GVkImage::ToView(view->GetRoot(), level, layer, 1, 1, view->GetType());

				if (!_merge_image(newview, Op, stage, access))
					_images.emplace_back(newview, stage, access, layout, Op);
			}
		}
	}

	size_t images_size() const
	{
		return _images.size();
	}

	size_t buffers_size() const
	{
		return _buffers.size();
	}

	void clear()
	{
		_buffers.clear();
		_images.clear();
	}

	buffer_iterator buffer_begin()
	{
		return _buffers.begin();
	}

	buffer_iterator buffer_end()
	{
		return _buffers.end();
	}

	image_iterator image_begin()
	{
		return _images.begin();
	}

	image_iterator image_end()
	{
		return _images.end();
	}

	buffer_const_iterator buffer_cbegin() const
	{
		return _buffers.begin();
	}

	buffer_const_iterator buffer_cend() const
	{
		return _buffers.end();
	}

	image_const_iterator image_cbegin() const
	{
		return _images.begin();
	}

	image_const_iterator image_cend() const
	{
		return _images.end();
	}

	template<typename T>
	std::vector<T>::iterator begin()
	{
		return {};
	}

	template<>
	std::vector<image_resource>::iterator begin<image_resource>()
	{
		return image_begin();
	}

	template<>
	std::vector<buffer_resource>::iterator begin<buffer_resource>()
	{
		return buffer_begin();
	}

	template<typename T>
	std::vector<T>::iterator end()
	{
		return {};
	}

	template<>
	std::vector<image_resource>::iterator end<image_resource>()
	{
		return image_end();
	}

	template<>
	std::vector<buffer_resource>::iterator end<buffer_resource>()
	{
		return buffer_end();
	}

	bool is_valid(image_iterator& it) const
	{
		return it != _images.end();
	}

	bool is_valid(image_const_iterator& it) const
	{
		return it != _images.cend();
	}

	bool is_valid(buffer_iterator& it) const
	{
		return it != _buffers.end();
	}

	bool is_valid(buffer_const_iterator& it) const
	{
		return it != _buffers.cend();
	}

	bool empty() const
	{
		return _images.empty() && _buffers.empty();
	}

	bool contains(std::shared_ptr<GVkImageView> view) const
	{
		for (auto& image : _images)
		{
			if (*view == *image.view)
				return true;
		}

		return false;
	}

	bool contains(std::shared_ptr<GVkBufferView> view) const
	{
		for (auto& buffer : _buffers)
		{
			if (*view == *buffer.view)
				return true;
		}

		return false;
	}

	image_iterator find(std::shared_ptr<GVkImageView> view)
	{
		for (auto it = _images.begin(); it != _images.end(); it++)
		{
			if (*view == *it->view)
				return it;
		}

		return _images.end();
	}

	buffer_iterator find(std::shared_ptr<GVkBufferView> view)
	{
		for (auto it = _buffers.begin(); it != _buffers.end(); it++)
		{
			if (*view == *it->view)
				return it;
		}

		return _buffers.end();
	}

	image_const_iterator find(std::shared_ptr<GVkImageView> view) const
	{
		for (auto it = _images.cbegin(); it != _images.cend(); it++)
		{
			if (*view == *it->view)
				return it;
		}

		return _images.cend();
	}

	buffer_const_iterator find(std::shared_ptr<GVkBufferView> view) const
	{
		for (auto it = _buffers.cbegin(); it != _buffers.cend(); it++)
		{
			if (*view == *it->view)
				return it;
		}

		return _buffers.cend();
	}
};
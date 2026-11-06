#pragma once
#include "VulkanAPI/barrier.hpp"
#include "VulkanAPI/buffer.hpp"
#include "VulkanAPI/image.hpp"
#include <iterator>

enum class EResourceOp
{
	WriteOnly,
	ReadWrite,
	ReadOnly
};

class GStageViews
{
	friend class resource_iterator;

public:
	struct stage_resource
	{
	public:
		VkPipelineStageFlagBits2 stage;
		VkAccessFlagBits2 access;
		EResourceOp Op;

	public:
		virtual ~stage_resource() {}
		virtual IVkObj* ptr() const = 0;
		virtual bool is_texture() const = 0;
		virtual bool compare(const stage_resource& other) const { return false; }
	};

	struct resource_iterator : public std::forward_iterator_tag
	{
		friend class GStageViews;

	private:
		GStageViews* _array;
		size_t _index;

	private:
		resource_iterator(GStageViews* a, size_t i)
			: _array(a)
			, _index(i)
		{ }

		resource_iterator(const GStageViews* a, size_t i)
			: _array(const_cast<GStageViews*>(a))
			, _index(i)
		{ }

	public:
		stage_resource& operator*() 
		{ 
			if (_index < _array->_images.size())
				return _array->_images[_index];
			
			return _array->_buffers[_index - _array->_images.size()]; 
		}

		const stage_resource& operator*() const
		{
			if (_index < _array->_images.size())
				return _array->_images[_index];

			return _array->_buffers[_index - _array->_images.size()];
		}

		stage_resource* operator->() { return &this->operator*(); }

		friend bool operator== (const resource_iterator& a, const resource_iterator& b) { return a._array == b._array && a._index == b._index; };
		friend bool operator!= (const resource_iterator& a, const resource_iterator& b) { return a._array != b._array || a._index != b._index; };

		resource_iterator operator++(int) { auto tmp = *this; ++(*this); return tmp; }
		resource_iterator& operator++() { _index++; return *this; }
	};

	struct image_resource : public stage_resource
	{
	public:
		std::shared_ptr<GVkImageView> view;
		VkImageLayout layout;

	public:
		virtual ~image_resource() {}

		IVkObj* ptr() const override
		{
			return view.get();
		}

		bool is_texture() const override
		{
			return true;
		}

		bool compare(const stage_resource& other) const override
		{
			if (other.is_texture())
				return *static_cast<const image_resource&>(other).view == *view;

			return false; 
		}
	};

	struct buffer_resource : public stage_resource
	{
	public:
		std::shared_ptr<GVkBufferView> view;

	public:
		virtual ~buffer_resource() {}

		IVkObj* ptr() const override
		{
			return view.get();
		}

		bool is_texture() const override
		{
			return false;
		}

		bool compare(const stage_resource& other) const override
		{
			if (!other.is_texture())
				return *static_cast<const buffer_resource&>(other).view == *view;

			return false;
		}
	};

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
		{
			buffer_resource res{};
			res.access = access;
			res.stage = stage;
			res.view = view;
			res.Op = Op;

			_buffers.emplace_back(std::move(res));
		}
	}

	void add(std::shared_ptr<GVkImageView> view, EResourceOp Op, VkImageLayout layout, VkPipelineStageFlagBits2 stage, VkAccessFlagBits2 access)
	{
		for (uint32_t layer = view->BaseLayer(); layer < view->LastLayer(); layer++)
		{
			for (uint32_t level = view->BaseLevel(); level < view->LastLevel(); level++)
			{
				auto newview = GVkImage::ToView(view->GetRoot(), level, layer, 1, 1, view->GetType());

				if (!_merge_image(newview, Op, stage, access))
				{
					image_resource res{};
					res.layout = layout;
					res.access = access;
					res.stage = stage;
					res.view = view;
					res.Op = Op;

					_images.emplace_back(std::move(res));
				}
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

	size_t size() const
	{
		return images_size() + buffers_size();
	}

	void clear()
	{
		_buffers.clear();
		_images.clear();
	}

	bool empty() const
	{
		return _images.empty() && _buffers.empty();
	}

	resource_iterator buffer_begin() const
	{
		return resource_iterator(this, images_size());
	}

	resource_iterator buffer_end() const
	{
		return resource_iterator(this, size());
	}

	resource_iterator image_begin() const
	{
		return resource_iterator(this, 0);
	}

	resource_iterator image_end() const
	{
		return buffer_begin();
	}

	resource_iterator begin() const
	{
		return resource_iterator(this, 0);
	}

	resource_iterator end() const
	{
		return resource_iterator(this, _images.size() + _buffers.size());
	}

	resource_iterator find_similar(const stage_resource& resource) const
	{
		for (auto it = begin(); it != end(); it++)
		{
			if (it->compare(resource))
				return it;
		}

		return end();
	}
};
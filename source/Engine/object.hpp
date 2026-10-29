#pragma once
#include <concepts>
#include <future>
#include <set>

class GObject
{
private:
	std::unordered_map<GObject*, std::function<void(void)>> _changeCallbacks;
	std::set<GObject*> _connections;

private:
	void clear_connections(GObject* obj)
	{
		_changeCallbacks.erase(obj);
		_connections.erase(obj);
	}

	void open_connection(GObject* obj)
	{
		_connections.insert(obj);
	}

	bool has_connections(GObject* obj)
	{
		bool has_change = _changeCallbacks.contains(obj);
		return has_change;
	}

protected:
	void invokeChange()
	{
		for (auto& [obj, f] : _changeCallbacks)
			f();
	}

public:
	GObject() = default;

	GObject(const GObject& other)
	{
	}

	void operator=(const GObject& other)
	{
	}

	virtual ~GObject()
	{
		for (auto& connection : _connections)
			connection->clear_connections(this);
	}

	template<typename T, typename Callback = void(T::*)()>
	requires std::derived_from<T, GObject>
	void onChange(T* object, Callback func)
	{
		if (func)
		{
			_changeCallbacks[object] = [object, func]() {
				std::invoke(func, object);
			};

			object->open_connection(this);
			open_connection(object);
		}
		else
		{
			_changeCallbacks.erase(object);

			if (!has_connections(object))
				clear_connections(object);
		}
	}
};
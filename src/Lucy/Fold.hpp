#pragma once

template <typename T, typename... Args>
bool any_of(const T &v, Args &&...args)
{
	return ((v == args) || ...);
}

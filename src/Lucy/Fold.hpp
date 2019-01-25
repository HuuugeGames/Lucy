#pragma once

template <typename T, typename... Args>
bool anyOf(const T &v, Args &&...args)
{
	return ((v == args) || ...);
}

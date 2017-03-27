#pragma once

namespace Fiber 
{
	struct Handle;

	bool Update();
	bool Active();

	Handle* Spawn(void(*fn)(void*), void* arg, size_t stackSize = 0);

	void Yield();
	Handle* Self();

	void Suspend(Handle* fiber);
	void Resume(Handle* fiber);
};
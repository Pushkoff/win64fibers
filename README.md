# Win64 coroutine/fiber without asm

It uses RtlCaptureContext/RtlRestoreContext to switch context.

### Usage

```
#include <stdio.h>
#include <thread>
#include "Fiber.h"

void fn1(void* any)
{
	printf("fn1 start\n");
	Fiber::Yield();
	printf("fn1 end\n");
}

int main()
{
	Fiber::Handle* c1 = Fiber::Spawn(fn1, (void*)1);

	while (Fiber::Active())
	{
		if (Fiber::Update() == false)
			std::this_thread::sleep_for(1s);
	}
	return 0;
}
```

### Api

```
Handle* Spawn(void(*fn)(void*), void* arg, size_t stackSize = 0)
```
Creates new fiber
-- fn: a pointer to the function to be executed by the fiber
-- arg:
-- stackSize: desired stack size of fiber. if 0 stack size will have default length (about 28kb).

```
bool Active()
```
Returns true, if there is created and not finished fibers.

```
bool Update();
```
Get first ready fiber and run it. Returns true if any fiber was runned. Returns false if there is no ready fibers.

```
void Yield();
```
Suspends the execution of the current fiber. If fiber in ready state, it will be continued at next Update calls.

```
Handle* Self();
```
Returns handle of current fiber.

```
void Suspend(Handle* fiber);
```
Set fiber to waitable state. Have no effect up to Yield is called. 

```
void Resume(Handle* fiber);
```
Set fiber to ready state and add to ready queue. It will be continued at next Update calls.
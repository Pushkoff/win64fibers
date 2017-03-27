#include <stdio.h>
#include <thread>
#include "Fiber.h"
#include <exception>
#include <stdexcept>

void fn1(void* any)
{
	printf("fn1 start\n");
	Fiber::Yield();
	printf("fn1 end\n");
}

void fn2(void* any)
{
	printf("fn2 start\n");
	Fiber::Yield();
	printf("fn2 end\n");
}

void fn3(void* any)
{
	for (int i = 0; i < 10; i++)
	{
		printf("fn3 %d\n", i);
		Fiber::Handle* c2 = Fiber::Spawn(fn2, (void*)(unsigned long long)i);
		Fiber::Yield();
	}
}

void fn4(void* any)
{
	printf("fn4 start\n");
	try {
		throw std::runtime_error("Test exception");
	}
	catch (std::runtime_error e) {
		printf("fn 4 Runtime error %s\n", e.what());
	}
	printf("fn4 end\n");
}

void fn5(void* any)
{
	printf("fn5 start\n");
	{
		char buf[32 * 1024];
		snprintf(buf, sizeof(buf), "use buf 0x%llX", (unsigned long long)any);
		printf("%s\n", buf);
	}
	printf("fn5 end\n");
}

using namespace std::chrono_literals;

int main()
{
	Fiber::Handle* c1 = Fiber::Spawn(fn1, (void*)1);
	Fiber::Handle* c2 = Fiber::Spawn(fn2, (void*)2);
	Fiber::Handle* c3 = Fiber::Spawn(fn3, (void*)3);
	Fiber::Handle* c4 = Fiber::Spawn(fn4, (void*)4);
	Fiber::Handle* c5 = Fiber::Spawn(fn5, (void*)5);

	while (Fiber::Active())
	{
		if (Fiber::Update() == false)
			std::this_thread::sleep_for(1s);
	}
	return 0;
}
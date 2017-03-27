#include <Windows.h>
#undef Yield
#include <new>

namespace Fiber
{
	struct Handle
	{
		struct State
		{
			enum Type
			{
				Init,
				Active,
				Wait,
				Terminated
			};
		};

		Handle* nextReady = nullptr;
		State::Type state = State::Init;
		::CONTEXT ctx;
		void *context;
		void *stackBegin, *stackEnd;
		
		Handle(void * context_, void * stackBegin_, void * stackEnd_) noexcept : context(context_), stackBegin(stackBegin_), stackEnd(stackEnd_)
		{
			ZeroMemory(&ctx, sizeof(ctx));
			ctx.ContextFlags = CONTEXT_ALL | CONTEXT_XSTATE;
		}

		~Handle()
		{
		}
	};

	static int& count() noexcept {
		static int val = 0;
		return val;
	}

	bool Active() noexcept {
		return count() > 0;
	}

	static Handle* main() noexcept
	{
		static thread_local Handle val(0, 0, 0);
		return &val;
	}

	static Handle*& current() noexcept
	{
		static thread_local Handle* val = nullptr;
		return val;
	}

	static Handle*& readyHead() noexcept
	{
		static Handle* value = nullptr;
		return value;
	}

	static Handle*& readyTail() noexcept
	{
		static Handle* value = nullptr;
		return value;
	}

	static void AddReady(Handle& c) noexcept
	{
		c.nextReady = nullptr;
		if (readyTail() != nullptr)
			readyTail()->nextReady = &c;
		else
			readyHead() = &c;
		readyTail() = &c;
	}

	static Handle* GetReady() noexcept
	{
		Handle* ret = readyHead();
		if (ret != nullptr)
		{
			if (readyHead() == readyTail())
				readyHead() = readyTail() = nullptr;
			else
				readyHead() = readyHead()->nextReady;
			ret->nextReady = nullptr;
		}
		return ret;
	}

	static void SaveTIB(Handle& fiber)
	{
		_NT_TIB* tib = (_NT_TIB*)::NtCurrentTeb();
		fiber.stackBegin = tib->StackLimit;
		fiber.stackEnd = tib->StackBase;
	}

	static void RestoreTIB(Handle& fiber)
	{
		_NT_TIB* tib = (_NT_TIB*)::NtCurrentTeb();
		tib->StackLimit = fiber.stackBegin;
		tib->StackBase = fiber.stackEnd;
	}

	static void Jmp(Handle& to, Handle& from) noexcept
	{
		SaveTIB(*current());
		current() = &to;
		::RtlCaptureContext(&from.ctx);
		if (current() != &from)
		{
			::RtlRestoreContext(&to.ctx, NULL);
		}
		RestoreTIB(*current());
	}

	void Yield() noexcept
	{
		if (current() != main())
			Jmp(*main(), *current());
	}

	static void __declspec(noreturn) __declspec(nothrow) __declspec(noinline) _init(Handle* c, void(*fn)(void*), void* arg) noexcept
	{
		Handle& self = *c;
		RestoreTIB(self);
		__try
		{ 
			fn(arg);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
		};
		self.state = Handle::State::Terminated;
		Jmp(*main(), self);
	}

	Handle* Spawn(void(*fn)(void*), void* arg, size_t stackSize)
	{
		SYSTEM_INFO si;
		::GetSystemInfo(&si);
		const size_t pageSize = si.dwPageSize;

		if (stackSize == 0)
			stackSize = 7 * pageSize - sizeof(Handle);  // 7 pages for stack + Handle, 1 Guard
		else if (stackSize < 4096)
			stackSize = 4096;

		const size_t contextSize = ((pageSize + sizeof(Handle) + stackSize + (pageSize - 1)) / pageSize) * pageSize; // GUARD + Stack + Align + Handle 
		char* Context = (char*)::VirtualAlloc(0, contextSize, MEM_COMMIT, PAGE_READWRITE);
		char* ContextEnd = Context + contextSize;
		DWORD oldProtect;
		::VirtualProtect(Context, pageSize, PAGE_READWRITE | PAGE_GUARD, &oldProtect);

		DWORD64 stackBegin = (DWORD64)(Context + sizeof(DWORD64) - 1) & (~DWORD64(15));
		DWORD64 stackEnd = (DWORD64)(ContextEnd - sizeof(Handle)) & (~DWORD64(15));
		Handle* fiber = new ((void*)stackEnd) Handle(Context, (void *)stackBegin, (void *)stackEnd);

		DWORD64* stackPtr = (DWORD64*)(stackEnd);
		*(--stackPtr) = 0; // R9
		*(--stackPtr) = 0; // R8
		*(--stackPtr) = 0; // Rdx
		*(--stackPtr) = 0; // Rcx
		*(--stackPtr) = 0; // Rip

		::RtlCaptureContext(&fiber->ctx);
		fiber->ctx.Rax = 0;
		fiber->ctx.Rbx = 0;
		fiber->ctx.Rsi = 0;
		fiber->ctx.Rdi = 0;
		fiber->ctx.Rbp = 0;
		fiber->ctx.R9 = 0;
		fiber->ctx.R10 = 0;
		fiber->ctx.R11 = 0;
		fiber->ctx.R12 = 0;
		fiber->ctx.R13 = 0;
		fiber->ctx.R14 = 0;
		fiber->ctx.R15 = 0;

		fiber->ctx.Rip = (DWORD64)_init;
		fiber->ctx.Rcx = (DWORD64)fiber;
		fiber->ctx.Rdx = (DWORD64)fn;
		fiber->ctx.R8 = (DWORD64)arg;
		fiber->ctx.Rsp = (DWORD64)stackPtr;
		fiber->state = Handle::State::Active;
		count()++;
		AddReady(*fiber);
		return fiber;
	}

	static void Destroy(Handle* c)
	{
		void* context = c->context;
		c->~Handle();
		::VirtualFree(context, 0, MEM_RELEASE);
		count()--;
	}

	Handle* Self()
	{
		return (current() != main()) ? current() : nullptr;
	}

	void Suspend(Handle* fiber)
	{
		if (fiber == nullptr)
			fiber = current();

		if (fiber != main())
			fiber->state = Handle::State::Wait;
	}

	void Resume(Handle* fiber)
	{
		if (fiber == nullptr)
			fiber = current();

		if (fiber != main() && fiber->state == Handle::State::Wait)
			AddReady(*fiber);
	}

	bool Update()
	{
		Handle* fiber = GetReady();
		if (fiber)
		{
			current() = main();
			Jmp(*fiber, *main());
			switch (fiber->state)
			{
			case Handle::State::Terminated:
				Destroy(fiber);
				break;
			case Handle::State::Active:
				AddReady(*fiber);
				break;
			}
		}
		return fiber != nullptr;
	}
};
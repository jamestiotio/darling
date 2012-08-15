#include "UndefinedFunction.h"
#include <unistd.h>
#include <sys/mman.h>
#include <stdexcept>
#include <dlfcn.h>

UndefMgr::UndefMgr(int entries)
: m_nNext(0)
{
	int ps = getpagesize();
	void* mem;
	
	int bytes = entries * sizeof(UndefinedFunction);
	bytes = (bytes + ps - 1) / ps * ps;
	
	mem = ::mmap(0, bytes, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	if (mem == MAP_FAILED)
		throw std::runtime_error("Failed to map pages for UndefMgr");
	
	m_pMem = static_cast<UndefinedFunction*>(mem);
	m_nMax = bytes / sizeof(UndefinedFunction);
	m_nBytes = bytes;
}

UndefMgr::~UndefMgr()
{
	::munmap(m_pMem, m_nBytes);
}

void* UndefMgr::generateNew(const char* name)
{
	if (m_nNext >= m_nMax)
		throw std::runtime_error("UndefMgr buffer full");
	
	m_pMem[m_nNext].init(name);
	return &m_pMem[m_nNext++];
}

void UndefinedFunction::init(const char* name)
{
	_asm1[0] = 0x48;
	_asm1[1] = 0xbf;
	pStderr = stderr;
	_asm2[0] = 0x48;
	_asm2[1] = 0xbe;
	pErrMsg = "Undefined function called: %s\n";
	_asm3[0] = 0x48;
	_asm3[1] = 0xba;
	pFuncName = name;
	_asm4[0] = 0x48;
	_asm4[1] = 0xbb;
	pFprintf = dlsym(RTLD_DEFAULT, "fprintf");
	_asm5[0] = 0x48;
	_asm5[1] = 0x31;
	_asm5[2] = 0xc0;
	_asm5[3] = 0xff;
	_asm5[4] = 0xd3;
	_asm5[5] = 0x48;
	_asm5[6] = 0x31;
	_asm5[7] = 0xc0;
	_asm5[8] = 0xc3;
}

#ifdef TEST
int main()
{
	UndefMgr* mgr = new UndefMgr;
	int (*func)() = (int(*)()) mgr->generateNew("TestFunction");
	int (*func2)() = (int(*)()) mgr->generateNew("TestFunction2");
	
	func();
	func2();
	
	delete mgr;
	return 0;
}
#endif

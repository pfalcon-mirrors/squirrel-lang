extern "C" void __cxa_pure_virtual()
{
}

void operator delete(void *p)
{
    free(p);
}

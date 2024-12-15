#include <logger.h>
#include <debug/assert.h>

int main()
{
    KTRACE("Test message: %.3f.", 3.1415f);
    KDEBUG("Test message: %.3f.", 3.1415f);
    KINFOR("Test message: %.3f.", 3.1415f);
    KWARNG("Test message: %.3f.", 3.1415f);
    KERROR("Test message: %.3f.", 3.1415f);
    // KFATAL("Test message: %.3f.", 3.1415f);
    KASSERT(1==0);
    return 0;
}

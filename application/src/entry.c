#include <logger.h>
#include <debug/assert.h>

int main()
{
    log_output_hook_set(null);
    log_output_hook_set_default();

    KTRACE("Test message: %.3f.", 3.1415f);
    KDEBUG("Test message: %.3f.", 3.1415f);
    KINFOR("Test message: %.3f.", 3.1415f);
    KWARNG("Test message: %.3f.", 3.1415f);
    KERROR("Test message: %.3f.", 3.1415f);
    // KFATAL("Test message: %.3f.", 3.1415f);
    KASSERT_MSG(1==0,"Test message!");
    return 0;
}

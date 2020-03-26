#include "coroutine_launcher.h"

namespace hbco {

CoroutineLauncher::CoroutineLauncher() {}

CoroutineLauncher::~CoroutineLauncher() {
    ReleaseResources();
}

}
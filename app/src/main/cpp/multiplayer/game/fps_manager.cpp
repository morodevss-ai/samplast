#include "fps_manager.h"
#include "../main.h"

uint8_t FPSManager::targetFPS = 120;
bool FPSManager::initialized = false;

void FPSManager::Initialize() {
    if (!initialized) {
        SetFPS(120);
        initialized = true;
    }
}

void FPSManager::SetFPS(uint8_t fps) {
    targetFPS = fps;
    ForceUpdate();
}

void FPSManager::Update() {
    // این تابع در هر فریم فراخوانی می‌شود و فریم ریت را تنظیم می‌کند
    static int counter = 0;
    counter++;
    
    // هر 10 فریم یکبار فریم ریت را تنظیم می‌کنیم تا عملکرد بهینه باشد
    if (counter >= 10) {
        ForceUpdate();
        counter = 0;
    }
}

void FPSManager::ForceUpdate() {
    // تنظیم مستقیم فریم ریت با استفاده از تابع ApplyFPSPatch
    extern void ApplyFPSPatch(uint8_t fps);
    ApplyFPSPatch(targetFPS);
}
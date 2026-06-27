//
// Created by x1y2z on 11.01.2023.
//

#include "TextureDatabaseRuntime.h"
#include "../vendor/armhook/patch.h"

TextureDatabaseRuntime* TextureDatabaseRuntime::Load(const char *withName, bool fullyLoad, TextureDatabaseFormat forcedFormat) {
    FLog("Load %s..", withName);
    return CHook::CallFunction<TextureDatabaseRuntime*>(g_libGTASA + (VER_x32 ? 0x001EA864 + 1 : 0x28771C), withName, fullyLoad, forcedFormat);
}

void TextureDatabaseRuntime::ChangeDir(bool isLoader) {
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8374 : 0x713F87));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8374 : 0x713F87)), isLoader? "loader/texdb/%s/%s.txt" : "texdb/%s/%s.txt");

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8824 : 0x71402B));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8824 : 0x71402B)), isLoader? "loader/texdb/%s/src/%s.png" : "texdb/%s/src/%s.png");

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8384 : 0x713F97));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8384 : 0x713F97)), isLoader? "loader/texdb/%s/%s.txt.bak" : "texdb/%s/%s.txt.bak");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87F0 : 0x713FB3));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87F0 : 0x713FB3)), isLoader? "loader/texdb/%s/%s.unc.tmb" : "texdb/%s/%s.unc.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87DC : 0x713FC7));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87DC : 0x713FC7)), isLoader? "loader/texdb/%s/%s.dxt.tmb" : "texdb/%s/%s.dxt.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87C8 : 0x713FDB));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87C8 : 0x713FDB)), isLoader? "loader/texdb/%s/%s.360.tmb" : "texdb/%s/%s.360.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87B4 : 0x713FEF));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87B4 : 0x713FEF)), isLoader? "loader/texdb/%s/%s.ps3.tmb" : "texdb/%s/%s.ps3.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87A0 : 0x714003));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E87A0 : 0x714003)), isLoader? "loader/texdb/%s/%s.pvr.tmb" : "texdb/%s/%s.pvr.tmb");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E878C : 0x714017));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E878C : 0x714017)), isLoader? "loader/texdb/%s/%s.etc.tmb" : "texdb/%s/%s.etc.tmb");

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C34 : 0x71403F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8C34 : 0x71403F)), isLoader? "loader/texdb/%s/%s.dxt" : "texdb/%s/%s.dxt");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C24 : 0x71404F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8C24 : 0x71404F)), isLoader? "loader/texdb/%s/%s.360" : "texdb/%s/%s.360");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C14 : 0x71405F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8C14 : 0x71405F)), isLoader? "loader/texdb/%s/%s.ps3" : "texdb/%s/%s.ps3");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C04 : 0x71406F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8C04 : 0x71406F)), isLoader? "loader/texdb/%s/%s.pvr" : "texdb/%s/%s.pvr");
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8BF4 : 0x71407F));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x1E8BF4 : 0x71407F)), isLoader? "loader/texdb/%s/%s.etc" : "texdb/%s/%s.etc");

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x61B46C : 0x757A30));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x61B46C : 0x757A30)), isLoader? "LOADER\\TEXDB" : "TEXDB");
}

void TextureDatabaseRuntime::Register(TextureDatabaseRuntime *toRegister) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x1E9B48 + 1 : 0x2865D8), toRegister);
//    if (std::find(registered.dataPtr, registered.dataPtr + registered.numEntries, toRegister) != registered.dataPtr + registered.numEntries) {
//        return; // ��� ���������������, �������
//    }
//
//    if (registered.numAlloced < registered.numEntries + 1) {
//        size_t newAlloc = ((3 * (registered.numEntries + 1)) >> 1) + 3;
//        TextureDatabaseRuntime** newData = static_cast<TextureDatabaseRuntime**>(malloc(sizeof(TextureDatabaseRuntime*) * newAlloc));
//
//        if (registered.dataPtr) {
//            std::memcpy(newData, registered.dataPtr, sizeof(TextureDatabaseRuntime*) * registered.numEntries);
//            free(registered.dataPtr);
//        }
//
//        registered.dataPtr = newData;
//        registered.numAlloced = newAlloc;
//    }
//
//    registered.dataPtr[registered.numEntries++] = toRegister;
}

void TextureDatabaseRuntime::Unregister(TextureDatabaseRuntime *toUnregister) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x1E9C00 + 1 : 0x2866A4), toUnregister);
}

RwTexture* TextureDatabaseRuntime::GetTexture(const char *name) {
    return ((RwTexture*(*)(const char*))(g_libGTASA + (VER_x32 ? 0x001E9C64 + 1 : 0x286718)))(name);
}

void TextureDatabaseRuntime::UpdateStreaming(float deltaTime, bool flush) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x1E9518 + 1 : 0x285DBC), deltaTime, flush);
}

TextureDatabaseRuntime* TextureDatabaseRuntime::GetDatabase(const char *dbName) {
    return CHook::CallFunction<TextureDatabaseRuntime*>(g_libGTASA + (VER_x32 ? 0x1EAC0C + 1 : 0x287AF4), dbName);
//    for (unsigned int i = 0; i < TextureDatabaseRuntime::loaded.numEntries; ++i) {
//        TextureDatabaseRuntime *currentDatabase = TextureDatabaseRuntime::loaded.dataPtr[i];
//        if (strcmp(currentDatabase->name, dbName) == 0) {
//            return currentDatabase;
//        }
//    }
//
//    return nullptr;
}

void TextureDatabaseRuntime::InjectHooks() {
//    CHook::Write(g_libGTASA + (VER_x32 ? 0x675FCC : 0x84A020), &detailTextures);
//    CHook::Write(g_libGTASA + (VER_x32 ? 0x676720 : 0x84AEB0), &storedTexels);
//    CHook::Write(g_libGTASA + (VER_x32 ? 0x679768 : 0x850EE8), &loaded);
//    CHook::Write(g_libGTASA + (VER_x32 ? 0x679E70 : 0x851CF8), &registered);
}

RwBool TextureAnnihilate(RwTexture *texture) {
    return CHook::CallFunction<RwBool>(g_libGTASA + (VER_x32 ? 0x1DB33C + 1 : 0x273A1C), texture);
}

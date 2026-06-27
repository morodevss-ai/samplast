//
// Created by x1y2z on 01.07.2023.
//

#pragma once

#include "common.h"
#include "TxdDef.h"
#include "Pool.h"

typedef CPool<TxdDef> CTxdPool;

class CTxdStore {
public:
    static inline CTxdPool* ms_pTxdPool;

public:
    struct ScopedTXDSlot {
        ScopedTXDSlot(int32 id) {
            if (id >= 0) {
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(id);
                m_Valid = true;
            } else {
                m_Valid = false;
            }
        }

        ScopedTXDSlot(const char* txdName) {
            int32 id = CTxdStore::FindTxdSlot(txdName);
            if (id >= 0) {
                CTxdStore::PushCurrentTxd();
                CTxdStore::SetCurrentTxd(id);
                m_Valid = true;
            } else {
                m_Valid = false;
            }
        }

        ~ScopedTXDSlot() {
            if (m_Valid) {
                CTxdStore::PopCurrentTxd();
            }
        }

    private:
        bool m_Valid = false;
    };

public:
    static void InjectHooks();

    static void PushCurrentTxd();
    static void PopCurrentTxd();
    static void SetCurrentTxd(int32 index);

    static RwTexture* TxdStoreFindCB(const char* name);
    static RwTexture* TxdStoreLoadCB(const char* name, const char* mask);

    static bool PluginAttach();
    static void Initialise();
    static void Shutdown();
    static void GameShutdown();

    static int32 FindTxdSlot(const char* name);
    static int32 FindTxdSlot(uint32 hash);

    static int32 AddTxdSlot(const char* name, const char *dbName, bool keepCPU);
    static void RemoveTxdSlot(int32 index) {
        RemoveTxd(index);
    }
    static void RemoveTxd(int32 index);

    static int32 GetNumRefs(int32 index);

    static int32 FindOrAddTxdSlot(const char* name, const char* dbName) {
        auto slot = CTxdStore::FindTxdSlot(name);
        if (slot == -1) slot = CTxdStore::AddTxdSlot(name, dbName, false);
        return slot;
    }
};
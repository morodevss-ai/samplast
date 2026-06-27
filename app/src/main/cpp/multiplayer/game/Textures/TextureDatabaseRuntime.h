//
// Created by x1y2z on 11.01.2023.
//

#pragma once
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <memory>
#include <algorithm>
#include <utility>
#include <type_traits>
#include <cstdint>
#include <fstream>
#include <queue>
#include <list>
#include "game/RW/RenderWare.h"
#include "OSFile.h"

#include "TextureDatabase.h"


class TextureDatabaseRuntime : public TextureDatabase {
public:
    TDBArray<uint32>   priorityStreamingQueue;
    TDBArray<uint32>   renderedStreamingQueue;
    TDBArray<uint32>   unrenderedStreamingQueue;
    TDBArray<uint32>   deletionQueue;
    OSFile             streamFile;
    TDBArray<uint32>   fullDataOffsets;
    TDBArray<uint32>   hashOffsets;
    uint32             numHashes;

public:
    static inline uint32 storedTexels{};
    static inline TDBArray<TextureDatabaseRuntime*> loaded;
    static inline TDBArray<TextureDatabaseRuntime*> registered;
    static inline TDBArray<RwTexture*> detailTextures;

public:
    static void InjectHooks();

    static std::vector<std::string> ReadTextureDBNamesFromCFG(const char *filename) {
        std::vector<std::string> names;
        std::ifstream infile(filename);
        std::string line;

        while (std::getline(infile, line)) {
            line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch){
                return !std::isspace(ch);
            }));
            line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch){
                return !std::isspace(ch);
            }).base(), line.end());
            if (!line.empty() && line.front() != '#') {
                names.push_back(line);
            }
        }

        return names;
    }

    static TextureDatabaseRuntime* Load(const char *withName, bool fullyLoad, TextureDatabaseFormat forcedFormat);
    static TextureDatabaseRuntime* GetDatabase(const char *dbName);
    static void UpdateStreaming(float deltaTime, bool flush);

    static void Register(TextureDatabaseRuntime *thiz);
    static void Unregister(TextureDatabaseRuntime *toUnregister);

    static RwTexture *GetTexture(const char *name);
    static void ChangeDir(bool isLoader);
};

VALIDATE_SIZE(TextureDatabaseRuntime, (VER_x32 ? 0xC8 : 0x118));
VALIDATE_SIZE(TDBArray<TextureDatabaseRuntime*>, (VER_x32 ? 0xC : 0x10));

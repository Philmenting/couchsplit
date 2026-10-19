// SPDX-License-Identifier: GPL-3.0-or-later
// Real emulator configuration code; only the Platform file boundary is local.
#include "Config.h"
#include "Platform.h"
#include "toml/toml.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <thread>
#include <vector>

static std::string testDirectory;
namespace melonDS::Platform {
struct FileHandle {std::FILE* stream;};
std::string GetLocalFilePath(const std::string& name) {return testDirectory+"/"+name;}
bool FileExists(const std::string& path) {return std::filesystem::exists(std::filesystem::u8path(path));}
bool CheckFileWritable(const std::string&) {return true;}
FileHandle* OpenLocalFile(const std::string&,FileMode) {return nullptr;} // no legacy INI inputs
bool IsEndOfFile(FileHandle* file) {return std::feof(file->stream);}
bool FileReadLine(char* text,int count,FileHandle* file) {return std::fgets(text,count,file->stream)!=nullptr;}
bool CloseFile(FileHandle* file) {const bool ok=std::fclose(file->stream)==0;delete file;return ok;}
}

int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);QTemporaryDir data;assert(data.isValid());
    testDirectory=data.path().toUtf8().toStdString();assert(Config::Load());
    Config::GetGlobalTable().SetBool("Initialized",true);
    std::atomic<bool> start{false},failed{false};std::vector<std::thread> threads;
    for(int worker=0;worker<4;worker++)threads.emplace_back([&,worker]{
        while(!start.load())std::this_thread::yield();
        try {
            auto table=Config::GetGlobalTable().GetTable("Concurrent.Thread"+std::to_string(worker));
            for(int n=0;n<500;n++) {
                // Getters also modify the real TOML tree when a default is absent.
                table.GetInt("Default"+std::to_string(n));
                table.SetInt("Count",n);table.SetString("Name","worker");table.SetBool("Ready",true);
                auto array=table.GetArray("Array");array.SetInt(0,n);assert(array.GetInt(0)==n);
                if(n%25==0)Config::Save();
            }
        } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';failed=true;}
    });
    threads.emplace_back([&]{
        while(!start.load())std::this_thread::yield();
        try {for(int n=0;n<100;n++)Config::Save();}
        catch(const std::exception& error){std::cerr<<error.what()<<'\n';failed=true;}
    });
    start=true;for(auto& thread:threads)thread.join();assert(!failed.load());Config::Save();
    auto parsed=toml::parse(std::filesystem::u8path(testDirectory+"/melonDS.toml"));
    for(int worker=0;worker<4;worker++) {
        auto& table=parsed.at("Concurrent").at("Thread"+std::to_string(worker));
        assert(table.at("Count").as_integer()==499&&table.at("Array").at(0).as_integer()==499);
    }
    std::cout<<"PASS: production config getters/setters/arrays and concurrent disk serialization preserve parseable settings\n";
}

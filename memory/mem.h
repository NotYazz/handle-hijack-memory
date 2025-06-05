#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <string_view>
#include "handle.h"

class Memory
{
private:
    DWORD processId = 0;
    HANDLE processHandle = nullptr;

public:
    Memory(std::string_view processName) noexcept
    {
        processId = GetPID(processName.data());
        processHandle = HijackExistingHandle(processId);
    }

    ~Memory()
    {
        if (processHandle)
            ::CloseHandle(processHandle);
    }

    std::uintptr_t GetModuleBase(std::string_view moduleName) const noexcept
    {
        MODULEENTRY32 entry = {};
        entry.dwSize = sizeof(MODULEENTRY32);
        const HANDLE snapShot = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);

        std::uintptr_t result = 0;
        while (::Module32Next(snapShot, &entry))
        {
            if (!moduleName.compare(entry.szModule))
            {
                result = reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
                break;
            }
        }

        if (snapShot)
            ::CloseHandle(snapShot);

        return result;
    }

    template <typename T>
    const T read(const std::uintptr_t address) const noexcept
    {
        T value = {};
        ::ReadProcessMemory(processHandle, reinterpret_cast<const void*>(address), &value, sizeof(T), NULL);
        return value;
    }

    template <typename T>
    void write(const std::uintptr_t address, const T& value) const noexcept
    {
        ::WriteProcessMemory(processHandle, reinterpret_cast<void*>(address), &value, sizeof(T), NULL);
    }
};

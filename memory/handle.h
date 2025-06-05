#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include "defs.h"

OBJECT_ATTRIBUTES InitObjectAttributes(PUNICODE_STRING name, ULONG attributes, HANDLE hRoot, PSECURITY_DESCRIPTOR security)
{
    OBJECT_ATTRIBUTES obj;
    obj.Length = sizeof(OBJECT_ATTRIBUTES);
    obj.ObjectName = name;
    obj.Attributes = attributes;
    obj.RootDirectory = hRoot;
    obj.SecurityDescriptor = security;
    obj.SecurityQualityOfService = nullptr;
    return obj;
}

SYSTEM_HANDLE_INFORMATION* hInfo = nullptr;
HANDLE procHandle = nullptr;
HANDLE hProcess = nullptr;
HANDLE HijackedHandle = nullptr;

DWORD GetPID(LPCSTR procName)
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, FALSE);
    if (hSnap && hSnap != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnap, &procEntry))
        {
            do
            {
                if (lstrcmpi(procEntry.szExeFile, procName) == 0)
                {
                    CloseHandle(hSnap);
                    return procEntry.th32ProcessID;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
        CloseHandle(hSnap);
    }
    return 0;
}

bool IsHandleValid(HANDLE handle)
{
    return handle && handle != INVALID_HANDLE_VALUE;
}

void CleanUpAndExit(const std::string& msg)
{
    delete[] reinterpret_cast<BYTE*>(hInfo);
    if (procHandle) CloseHandle(procHandle);
    std::cout << msg << std::endl;
    system("pause");
    exit(1);
}

HANDLE HijackExistingHandle(DWORD dwTargetProcessId)
{
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    fnRtlAdjustPrivilege RtlAdjustPrivilege = (fnRtlAdjustPrivilege)GetProcAddress(ntdll, "RtlAdjustPrivilege");
    fnNtQuerySystemInformation NtQuerySystemInformation = (fnNtQuerySystemInformation)GetProcAddress(ntdll, "NtQuerySystemInformation");
    fnNtDuplicateObject NtDuplicateObject = (fnNtDuplicateObject)GetProcAddress(ntdll, "NtDuplicateObject");
    fnNtOpenProcess NtOpenProcess = (fnNtOpenProcess)GetProcAddress(ntdll, "NtOpenProcess");

    BOOLEAN OldPriv;
    NTSTATUS NtRet = RtlAdjustPrivilege(SeDebugPriv, TRUE, FALSE, &OldPriv);

    OBJECT_ATTRIBUTES objAttr = InitObjectAttributes(nullptr, 0, nullptr, nullptr);
    CLIENT_ID clientID = { 0 };
    DWORD size = sizeof(SYSTEM_HANDLE_INFORMATION);
    hInfo = reinterpret_cast<SYSTEM_HANDLE_INFORMATION*>(new BYTE[size]);
    ZeroMemory(hInfo, size);

    do
    {
        delete[] reinterpret_cast<BYTE*>(hInfo);
        size = static_cast<DWORD>(size * 1.5);
        hInfo = reinterpret_cast<SYSTEM_HANDLE_INFORMATION*>(new BYTE[size]);
        ZeroMemory(hInfo, size);
        Sleep(1);
    } while ((NtRet = NtQuerySystemInformation(SystemHandleInformation, hInfo, size, NULL)) == STATUS_INFO_LENGTH_MISMATCH);

    if (!NT_SUCCESS(NtRet)) CleanUpAndExit("NtQuerySystemInformation failed");

    for (ULONG i = 0; i < hInfo->HandleCount; ++i)
    {
        DWORD handleCount;
        GetProcessHandleCount(GetCurrentProcess(), &handleCount);
        if (handleCount > 50) CleanUpAndExit("Too many open handles");

        if (!IsHandleValid((HANDLE)(uintptr_t)hInfo->Handles[i].Handle)) continue;
        if (hInfo->Handles[i].ObjectTypeNumber != ProcessHandleType) continue;

        clientID.UniqueProcess = reinterpret_cast<void*>((uintptr_t)hInfo->Handles[i].ProcessId);
        clientID.UniqueThread = nullptr;

        if (procHandle) CloseHandle(procHandle);

        NtRet = NtOpenProcess(&procHandle, PROCESS_DUP_HANDLE, &objAttr, &clientID);
        if (!IsHandleValid(procHandle) || !NT_SUCCESS(NtRet)) continue;

        NtRet = NtDuplicateObject(procHandle, (HANDLE)(uintptr_t)hInfo->Handles[i].Handle, NtCurrentProcess, &HijackedHandle, PROCESS_ALL_ACCESS, 0, 0);
        if (!IsHandleValid(HijackedHandle) || !NT_SUCCESS(NtRet)) continue;

        if (GetProcessId(HijackedHandle) != dwTargetProcessId)
        {
            CloseHandle(HijackedHandle);
            continue;
        }

        hProcess = HijackedHandle;
        break;
    }

    if (!IsHandleValid(hProcess)) CleanUpAndExit("Hijack failed");
    return hProcess;
}

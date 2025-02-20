#pragma once

#ifdef _WIN32

#include <Core/Exception.hpp>

#include <Windows.h>
#include <map>
#include <ntstatus.h>
#include <set>
#include <shlwapi.h>
#include <string>
#include <sstream>
#include <winternl.h>

namespace eLibrary {
    typedef NTSTATUS (NTAPI *NtCloseType)(IN HANDLE Handle);
    typedef NTSTATUS (NTAPI *NtCreateKeyType)(OUT HANDLE *KeyHandle, IN ACCESS_MASK DesiredAccess, IN OBJECT_ATTRIBUTES *ObjectAttributes, ULONG TitleIndex, IN OPTIONAL UNICODE_STRING *Class, IN ULONG CreateOptions, OUT OPTIONAL PULONG Disposition);
    typedef NTSTATUS (NTAPI *NtLoadDriverType)(IN UNICODE_STRING *DriverServiceName);
    typedef NTSTATUS (NTAPI *NtOpenProcessType)(OUT HANDLE *ProcessHandle, IN ACCESS_MASK DesiredAccess, IN OBJECT_ATTRIBUTES *ObjectAttributes, IN CLIENT_ID *ClientId);
    typedef NTSTATUS (NTAPI *NtOpenThreadType)(OUT HANDLE *ThreadHandle, IN ACCESS_MASK DesiredAccess, IN OBJECT_ATTRIBUTES *ObjectAttributes, IN CLIENT_ID *ClientId);
    typedef NTSTATUS (NTAPI *NtSetValueKeyType)(IN HANDLE KeyHandle, IN UNICODE_STRING *ValueName, IN OPTIONAL ULONG TitleIndex, IN ULONG Type, IN OPTIONAL PVOID Data, IN ULONG DataSize);
    typedef NTSTATUS (NTAPI *NtTerminateProcessType)(IN OPTIONAL HANDLE ProcessHandle, IN NTSTATUS ExitStatus);
    typedef NTSTATUS (NTAPI *NtTerminateThreadType)(IN OPTIONAL HANDLE ThreadHandle, IN NTSTATUS ExitStatus);
    typedef NTSTATUS (NTAPI *NtUnloadDriverType)(IN UNICODE_STRING *DriverServiceName);
    typedef NTSTATUS (NTAPI *RtlAdjustPrivilegeType)(IN ULONG Privilege, IN BOOLEAN Enable, IN BOOLEAN CurrentThread, OUT OPTIONAL BOOLEAN *Enabled);
    typedef NTSTATUS (NTAPI *RtlInitUnicodeStringType)(OUT UNICODE_STRING *DestinationString, IN PCWSTR SourceString);

#define SeLoadDriverPrivilege 0xa

    class NtModule final : public Object {
    private:
        mutable std::map<String, FARPROC> ModuleFunctionMapping;
        HMODULE ModuleHandle;
    protected:
        NtModule(HMODULE ModuleHandleSource) : ModuleHandle(ModuleHandleSource) {}
    public:
        ~NtModule() noexcept {
            if (ModuleHandle) {
                FreeLibrary(ModuleHandle);
                ModuleHandle = nullptr;
            }
        }

        static NtModule doLoad(const String &ModulePath) {
            HMODULE ModuleHandle = LoadLibraryW(ModulePath.toWString().c_str());
            if (!ModuleHandle) throw Exception(String(u"MtModule::doLoad(const String&)"));
            return NtModule(ModuleHandle);
        }

        FARPROC getFunction(const String &FunctionName) const {
            if (ModuleFunctionMapping.contains(FunctionName)) return ModuleFunctionMapping[FunctionName];
            FARPROC FunctionObject = GetProcAddress(ModuleHandle, FunctionName.toU8String().c_str());
            if (!FunctionObject) throw Exception(String(u"NtModule::getFunction() GetProcAddress"));
            return ModuleFunctionMapping[FunctionName] = FunctionObject;
        }
    };

    static NtModule ModuleNtDll = NtModule::doLoad(String(u"ntdll.dll"));

    class NtFile final : public Object {
    private:
        HANDLE FileHandle;
    protected:
        NtFile(HANDLE FileHandleSource) : FileHandle(FileHandleSource) {
            if (!FileHandleSource) throw Exception(String(u"NtFile::NtFile(HANDLE) FileHandleSource"));
        }
    public:
        enum class NtFileAccess {
            AccessAll = FILE_ALL_ACCESS,
            AccessAttributeExtendRead = FILE_READ_EA,
            AccessAttributeExtendWrite = FILE_WRITE_EA,
            AccessAttributeRead = FILE_READ_ATTRIBUTES,
            AccessAttributeWrite = FILE_WRITE_ATTRIBUTES,
            AccessDataAppend = FILE_APPEND_DATA,
            AccessDataRead = FILE_READ_DATA,
            AccessDataWrite = FILE_WRITE_DATA,
            AccessDelete = DELETE,
            AccessRead = FILE_READ_ACCESS,
            AccessSynchronize = SYNCHRONIZE,
            AccessWrite = FILE_WRITE_ACCESS
        };

        enum class NtFileAttribute {
            AttributeCompressed = FILE_ATTRIBUTE_COMPRESSED,
            AttributeHidden = FILE_ATTRIBUTE_HIDDEN,
            AttributeNormal = FILE_ATTRIBUTE_NORMAL,
            AttributeOffline = FILE_ATTRIBUTE_OFFLINE,
            AttributeReadonly = FILE_ATTRIBUTE_READONLY,
            AttributeSystem = FILE_ATTRIBUTE_SYSTEM,
            AttributeTemporary = FILE_ATTRIBUTE_TEMPORARY
        };

        enum class NtFileDisposition {
            DispositionCreate = FILE_CREATE,
            DispositionOpen = FILE_OPEN,
            DispositionOpenIf = FILE_OPEN_IF,
            DispositionOverwrite = FILE_OVERWRITE,
            DispositionOverwriteIf = FILE_OVERWRITE_IF,
            DispositionSupersede = FILE_SUPERSEDE
        };

        enum class NtFileOption {
            OptionDeleteOnClose = FILE_DELETE_ON_CLOSE,
            OptionNoCompression = FILE_NO_COMPRESSION,
            OptionSequential = FILE_SEQUENTIAL_ONLY,
            OptionWriteThrough = FILE_WRITE_THROUGH
        };

        enum class NtFileShare {
            ShareDelete = FILE_SHARE_DELETE,
            ShareNone = 0,
            ShareRead = FILE_SHARE_READ,
            ShareWrite = FILE_SHARE_WRITE
        };

        ~NtFile() noexcept {
            if (FileHandle) {
                NtClose(FileHandle);
                FileHandle = nullptr;
            }
        }

        static NtFile doCreate(const String &FilePath, bool FilePathCaseSenstive, const NtFileAccess &FileAccess, const NtFileAttribute &FileAttribute, const NtFileDisposition &FileDisposition, const NtFileOption &FileOption, const NtFileShare &FileShare) {
            HANDLE FileHandle;
            IO_STATUS_BLOCK FileStatusBlock;
            OBJECT_ATTRIBUTES FileObjectAttribute;
            UNICODE_STRING FilePathString;
            RtlInitUnicodeString(&FilePathString, FilePath.toWString().c_str());
            InitializeObjectAttributes(&FileObjectAttribute, &FilePathString, FilePathCaseSenstive ? 0 : OBJ_CASE_INSENSITIVE, nullptr, nullptr)
            if (!NT_SUCCESS(NtCreateFile(&FileHandle, (ACCESS_MASK) FileAccess, &FileObjectAttribute, &FileStatusBlock, nullptr, (ULONG) FileAttribute, (ULONG) FileShare, (ULONG) FileDisposition, (ULONG) FileOption, nullptr, 0)))
                throw Exception(String(u"NtFile::doCreate(const String&, bool, const NtFileAccess&, const NtFileAttribute&, const NtFileDisposition&, const NtFileOption&, const NtFileShare&) NtCreateFile"));
            return NtFile(FileHandle);
        }

        static NtFile doOpen(const String &FilePath, bool FilePathCaseSenstive, const NtFileAccess &FileAccess, const NtFileOption &FileOption, const NtFileShare &FileShare) {
            HANDLE FileHandle;
            IO_STATUS_BLOCK FileStatusBlock;
            OBJECT_ATTRIBUTES FileObjectAttribute;
            UNICODE_STRING FilePathString;
            RtlInitUnicodeString(&FilePathString, FilePath.toWString().c_str());
            InitializeObjectAttributes(&FileObjectAttribute, &FilePathString, FilePathCaseSenstive ? 0 : OBJ_CASE_INSENSITIVE, nullptr, nullptr)
            if (!NT_SUCCESS(NtOpenFile(&FileHandle, (ACCESS_MASK) FileAccess, &FileObjectAttribute, &FileStatusBlock, (ULONG) FileShare, (ULONG) FileOption)))
                throw Exception(String(u"NtFile::doOpen() NtOpenFile"));
            return NtFile(FileHandle);
        }
    };

    class NtProcess final : public Object {
    private:
        HANDLE ProcessHandle;
    public:
        NtProcess(HANDLE ProcessHandleSource) : ProcessHandle(ProcessHandleSource) {
            if (!ProcessHandleSource) throw Exception(String(u"NtProcess::NtProcess(HANDLE) ProcessHandleSource"));
        }

        ~NtProcess() noexcept {
            if (ProcessHandle) {
                NtClose(ProcessHandle);
                ProcessHandle = nullptr;
            }
        }

        static NtProcess doOpen(DWORD ProcessID) {
            NtOpenProcessType NtOpenProcess = (NtOpenProcessType) ModuleNtDll.getFunction(String(u"NtOpenProcess"));
            CLIENT_ID ProcessClientID{ULongToHandle(ProcessID), nullptr};
            HANDLE ProcessHandle;
            OBJECT_ATTRIBUTES ProcessObjectAttribute;
            InitializeObjectAttributes(&ProcessObjectAttribute, nullptr, 0, nullptr, nullptr)
            if (!NT_SUCCESS(NtOpenProcess(&ProcessHandle, PROCESS_ALL_ACCESS, &ProcessObjectAttribute, &ProcessClientID))) throw Exception(String(u"NtProcess::doOpen(DWORD) NtOpenProcess"));
            return NtProcess(ProcessHandle);
        }

        void doTerminate(NTSTATUS ProcessStatus) const {
            NtTerminateProcessType NtTerminateProcess = (NtTerminateProcessType) ModuleNtDll.getFunction(String(u"NtTerminateProcess"));
            if (!NT_SUCCESS(NtTerminateProcess(ProcessHandle, ProcessStatus))) throw Exception(String(u"NtProcess::doTerminate(NTSTATUS) NtTerminateProcess"));
        }
    };

    class NtService final : public Object {
    public:
        enum class NtServiceErrorControl {
            ControlCritical = SERVICE_ERROR_CRITICAL,
            ControlIgnore = SERVICE_ERROR_IGNORE,
            ControlNormal = SERVICE_ERROR_NORMAL,
            ControlSevere = SERVICE_ERROR_SEVERE
        };

        enum class NtServiceStartType {
            TypeAuto = SERVICE_AUTO_START,
            TypeBoot = SERVICE_BOOT_START,
            TypeDemand = SERVICE_DEMAND_START,
            TypeDisabled = SERVICE_DISABLED,
            TypeSystem = SERVICE_SYSTEM_START
        };

        enum class NtServiceType {
            TypeKernelDriver = SERVICE_KERNEL_DRIVER
        };
    private:
        SC_HANDLE ServiceHandle;
        String ServiceName;
        String ServicePath;
        NtServiceErrorControl ServiceErrorControl;
        NtServiceStartType ServiceStartType;
        NtServiceType ServiceType;
        std::set<String> ServiceDependencySet;
    public:
        NtService(SC_HANDLE ServiceHandleSource, const String &ServiceNameSource, const String &ServicePathSource, const NtServiceErrorControl &ServiceErrorControlSource, const NtServiceStartType &ServiceStartTypeSource, const NtServiceType &ServiceTypeSource) : ServiceHandle(ServiceHandleSource), ServiceName(ServiceNameSource), ServicePath(ServicePathSource), ServiceErrorControl(ServiceErrorControlSource), ServiceStartType(ServiceStartTypeSource), ServiceType(ServiceTypeSource) {
            if (!ServiceHandleSource) throw Exception(String(u"NtService::NtService(SC_HANDLE, const String&, const String&, const String&, const NtServiceErrorControl&, const NtServiceStartType&, const NtServiceType&) ServiceHandleSource"));
        }

        ~NtService() noexcept {
            if (ServiceHandle) {
                CloseServiceHandle(ServiceHandle);
                ServiceHandle = nullptr;
            }
        }

        void addDependency(const NtService &ServiceDependency) noexcept {
            ServiceDependencySet.insert(ServiceDependency.ServiceName);
        }

        void doControl(DWORD ServiceControl) const {
            if (!ControlService(ServiceHandle, ServiceControl, nullptr))
                throw Exception(String(u"NtService::doControl(DWORD) ControlService"));
        }

        void doDelete() const {
            if (!DeleteService(ServiceHandle))
                throw Exception(String(u"NtService::doDelete() DeleteService"));
        }

        void doStart(DWORD ServiceArgumentCount, void **ServiceArgumentList) const {
            if (!StartServiceW(ServiceHandle, ServiceArgumentCount, (const wchar_t**) ServiceArgumentList))
                throw Exception(String(u"NtService::doStart(DWORD, void**) StartServiceW"));
        }

        NtServiceErrorControl getServiceErrorControl() const noexcept {
            return ServiceErrorControl;
        }

        String getServiceName() const noexcept {
            return ServiceName;
        }

        String getServicePath() const noexcept {
            return ServicePath;
        }

        NtServiceStartType getServiceStartType() const noexcept {
            return ServiceStartType;
        }

        NtServiceType getServiceType() const noexcept {
            return ServiceType;
        }

        DWORD getServiceState() const {
            SERVICE_STATUS ServiceStatus;
            if (!QueryServiceStatus(ServiceHandle, &ServiceStatus))
                throw Exception(String(u"NtService::getServiceState() QueryServiceStatus"));
            return ServiceStatus.dwCurrentState;
        }

        void removeDependency(const NtService &ServiceDependency) {
            if (!ServiceDependencySet.contains(ServiceDependency.ServiceName))
                throw Exception(String(u"NtService::removeDependency(const NtService&) ServiceDependency"));
            ServiceDependencySet.erase(ServiceDependency.ServiceName);
        }

        void setServiceErrorControl(const NtServiceErrorControl &ServiceErrorControlSource) {
            ServiceErrorControl = ServiceErrorControlSource;
            updateServiceConfiguration();
        }

        void setServicePath(const String &ServicePathSource) {
            ServicePath.doAssign(ServicePathSource);
            updateServiceConfiguration();
        }

        void setServiceStartType(const NtServiceStartType &ServiceStartTypeSource) {
            ServiceStartType = ServiceStartTypeSource;
            updateServiceConfiguration();
        }

        void setServiceType(const NtServiceType &ServiceTypeSource) {
            ServiceType = ServiceTypeSource;
            updateServiceConfiguration();
        }

        void updateServiceConfiguration() const {
            std::basic_stringstream<wchar_t> ServiceDependencyStream;
            for (const auto &ServiceDependency : ServiceDependencySet)
                ServiceDependencyStream << ServiceDependency.toWString() << L'\0';
            ServiceDependencyStream << L'\0';
            if (!ChangeServiceConfigW(ServiceHandle, (DWORD) ServiceType, (DWORD) ServiceStartType, (DWORD) ServiceErrorControl, ServicePath.toWString().c_str(), nullptr, nullptr, ServiceDependencyStream.str().c_str(), nullptr, nullptr, ServiceName.toWString().c_str()))
                throw Exception(String(u"NtServcie::updateServiceConfiguration() ChangeServiceConfigW"));
        }
    };

    class NtServiceManager final : public Object {
    private:
        SC_HANDLE ManagerHandle;
    public:
        NtServiceManager(const String &ManagerMachineName, const String &ManagerDatabaseName) {
            if (!(ManagerHandle = OpenSCManagerW(ManagerMachineName.isNull() ? nullptr : ManagerMachineName.toWString().c_str(), ManagerDatabaseName.isNull() ? nullptr : ManagerDatabaseName.toWString().c_str(), SC_MANAGER_ALL_ACCESS)))
                throw Exception(String(u"NtServiceManager::NtServiceManager(const String&, const String&) OpenSCManagerW"));
        }

        ~NtServiceManager() noexcept {
            if (ManagerHandle) {
                CloseServiceHandle(ManagerHandle);
                ManagerHandle = nullptr;
            }
        }

        NtService doCreateService(const String &ServiceName, const String &ServicePath, const NtService::NtServiceType &ServiceType, const NtService::NtServiceStartType &ServiceStartType, const NtService::NtServiceErrorControl &ServiceErrorControl) const {
            SC_HANDLE ServiceHandle = CreateServiceW(ManagerHandle, ServiceName.toWString().c_str(), ServiceName.toWString().c_str(), SERVICE_ALL_ACCESS, (DWORD) ServiceType, (DWORD) ServiceStartType, (DWORD) ServiceErrorControl, ServicePath.toWString().c_str(), nullptr, nullptr, nullptr, nullptr, nullptr);
            if (!ServiceHandle) throw Exception(String(u"NtServiceManager::doCreateService(const String&, const String&, const NtService::NtServiceType&, const NtService::NtServiceStartType&, const NtService::NtServiceErrorControl&) CreateServiceW"));
            return NtService(ServiceHandle, ServiceName, ServicePath, ServiceErrorControl, ServiceStartType, ServiceType);
        }

        NtService doOpenService(const String &ServiceName) const {
            SC_HANDLE ServiceHandle = OpenServiceW(ManagerHandle, ServiceName.toWString().c_str(), SERVICE_ALL_ACCESS);
            if (!ServiceHandle) throw Exception(String(u"NtServiceManager::doOpenService(const String&) OpenServiceW"));
            QUERY_SERVICE_CONFIGW ServiceConfiguration;
            if (!QueryServiceConfigW(ServiceHandle, &ServiceConfiguration, sizeof(QUERY_SERVICE_CONFIGW), nullptr))
                throw Exception(String(u"NtServiceManager::doOpenService(const String&) QueryServiceConfigW"));
            return NtService(ServiceHandle, ServiceName, String(ServiceConfiguration.lpBinaryPathName), (NtService::NtServiceErrorControl) ServiceConfiguration.dwErrorControl, (NtService::NtServiceStartType) ServiceConfiguration.dwStartType, (NtService::NtServiceType) ServiceConfiguration.dwServiceType);
        }
    };

    class NtDriver final : public Object {
    private:
        HANDLE DriverFileHandle;
        String DriverFilePath;
        NtService DriverService;
    public:
        NtDriver(const String &DriverFilePathSource, const NtService &DriverServiceSource) : DriverFileHandle(nullptr), DriverFilePath(DriverFilePathSource), DriverService(DriverServiceSource) {
            if (DriverServiceSource.getServiceType() != NtService::NtServiceType::TypeKernelDriver) throw Exception(String(u"NtDriver::NtDriver(const String&, const NtService&) DriverServiceSource"));
        }

        ~NtDriver() noexcept {
            if (DriverFileHandle) CloseHandle(DriverFileHandle);
        }

        void doClose() {
            if (!DriverFileHandle) throw Exception(String(u"NtDriver::doClose() DriverFileHandle"));
            CloseHandle(DriverFileHandle);
        }

        void doControl(DWORD DriverControlCode, void *DriverControlInputBuffer, DWORD DriverControlInputSize, void *DriverControlOutputBuffer, DWORD DriverControlOutputBufferSize, DWORD *DriverControlOutputSize) {
            if (!DeviceIoControl(DriverFileHandle, DriverControlCode, DriverControlInputBuffer, DriverControlInputSize, DriverControlOutputBuffer, DriverControlOutputBufferSize, DriverControlOutputSize, nullptr))
                throw Exception(String(u"NtDriver::doControl(DWORD, void*, DWORD, void*, DWORD, DWORD*) DeviceIoControl"));
        }

        void doLoadNt() {
            NtCreateKeyType NtCreateKey = (NtCreateKeyType) ModuleNtDll.getFunction(String(u"NtCreateKey"));
            NtLoadDriverType NtLoadDriver = (NtLoadDriverType) ModuleNtDll.getFunction(String(u"NtLoadDriver"));
            NtSetValueKeyType NtSetValueKey = (NtSetValueKeyType) ModuleNtDll.getFunction(String(u"NtSetValueKey"));
            RtlAdjustPrivilegeType RtlAdjustPrivilege = (RtlAdjustPrivilegeType) ModuleNtDll.getFunction(String(u"RtlAdjustPrivilege"));

            std::basic_stringstream<wchar_t> DriverImagePathStream;
            DriverImagePathStream << L"\\??\\" << DriverFilePath.toWString();

            std::basic_stringstream<wchar_t> DriverRegistrationPathStream;
            DriverRegistrationPathStream << L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" << DriverService.getServiceName().toWString();

            UNICODE_STRING DriverRegistrationPath;
            RtlInitUnicodeString(&DriverRegistrationPath, DriverRegistrationPathStream.str().c_str());

            OBJECT_ATTRIBUTES DriverRegistrationAttribute;
            InitializeObjectAttributes(&DriverRegistrationAttribute, &DriverRegistrationPath, OBJ_CASE_INSENSITIVE, nullptr, nullptr)

            DWORD DriverErrorControl = (DWORD) DriverService.getServiceErrorControl();
            DWORD DriverServiceType = SERVICE_KERNEL_DRIVER;
            DWORD DriverStartType = (DWORD) DriverService.getServiceStartType();

            UNICODE_STRING DriverErrorControlString, DriverImagePathString, DriverServiceTypeString, DriverStartTypeString;
            RtlInitUnicodeString(&DriverErrorControlString, L"ErrorControl");
            RtlInitUnicodeString(&DriverImagePathString, L"ImagePath");
            RtlInitUnicodeString(&DriverServiceTypeString, L"Type");
            RtlInitUnicodeString(&DriverStartTypeString, L"Start");

            HANDLE DriverRegistrationHandle;
            if (!NT_SUCCESS(NtCreateKey(&DriverRegistrationHandle, KEY_ALL_ACCESS, &DriverRegistrationAttribute, 0, nullptr, 0, nullptr)))
                throw Exception(String(u"NtDriver::doLoadNt() NtCreateKey"));
            NtSetValueKey(DriverRegistrationHandle, &DriverErrorControlString, 0, REG_DWORD, &DriverErrorControl, sizeof(DWORD));
            NtSetValueKey(DriverRegistrationHandle, &DriverImagePathString, 0, REG_EXPAND_SZ, (void*) DriverImagePathStream.str().c_str(), sizeof(wchar_t) * (DriverImagePathStream.str().size() + 1));
            NtSetValueKey(DriverRegistrationHandle, &DriverStartTypeString, 0, REG_DWORD, &DriverStartType, sizeof(DWORD));
            NtSetValueKey(DriverRegistrationHandle, &DriverServiceTypeString, 0, REG_DWORD, &DriverServiceType, sizeof(DWORD));
            NtClose(DriverRegistrationHandle);

            RtlAdjustPrivilege(SeLoadDriverPrivilege, TRUE, FALSE, nullptr);

            if (!NT_SUCCESS(NtLoadDriver(&DriverRegistrationPath)))
                throw Exception(String(u"NtDriver::doLoadNt() NtLoadDriver"));
        }

        void doOpen(const String &DriverControlPath) {
            DriverFileHandle = CreateFileW(DriverControlPath.toWString().c_str(), FILE_ALL_ACCESS, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (DriverFileHandle == INVALID_HANDLE_VALUE) throw Exception(String(u"NtDriver::doOpen(const String&) CreateFileW"));
        }

        void doRead(void *DriverControlBuffer, DWORD DriverControlBufferSize, DWORD *DriverControlSize) const {
            if (!ReadFile(DriverFileHandle, DriverControlBuffer, DriverControlBufferSize, DriverControlSize, nullptr))
                throw Exception(String(u"NtDriver::doRead(void*, DWORD, DWORD) ReadFile"));
        }

        void doUnloadNt() const {
            NtUnloadDriverType NtUnloadDriver = (NtUnloadDriverType) ModuleNtDll.getFunction(String(u"NtUnloadDriver"));
            RtlAdjustPrivilegeType RtlAdjustPrivilege = (RtlAdjustPrivilegeType) ModuleNtDll.getFunction(String(u"RtlAdjustPrivilege"));

            std::basic_stringstream<wchar_t> DriverRegistrationPathStream;
            DriverRegistrationPathStream << L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\" << DriverService.getServiceName().toWString();

            UNICODE_STRING DriverRegistrationPath;
            RtlInitUnicodeString(&DriverRegistrationPath, DriverRegistrationPathStream.str().c_str());

            RtlAdjustPrivilege(SeLoadDriverPrivilege, TRUE, FALSE, nullptr);

            if (!NT_SUCCESS(NtUnloadDriver(&DriverRegistrationPath)))
                throw Exception(String(u"NtDriver::doUnloadNt() NtUnloadDriver"));
            if (!NT_SUCCESS(SHDeleteKeyW(HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Services\\eLibraryDriver")))
                throw Exception(String(u"NtDriver::doUnloadNt() SHDeleteKeyW"));
        }

        void doUnloadSC() const {
            DriverService.doControl(SERVICE_CONTROL_STOP);
            DriverService.doDelete();
        }

        void doWrite(void *DriverControlBuffer, DWORD DriverControlBufferSize, DWORD *DriverControlSize) const {
            if (!WriteFile(DriverFileHandle, DriverControlBuffer, DriverControlBufferSize, DriverControlSize, nullptr))
                throw Exception(String(u"NtDriver::doWrite(void*, DWORD, DWORD) WriteFile"));
        }
    };
}

#endif

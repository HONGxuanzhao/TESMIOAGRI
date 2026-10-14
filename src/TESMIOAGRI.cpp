// TESMIOAGRI 0.1.0 - version-locked field growth and harvest quantities.
// No shared constants, executable files or saved inventories are modified.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "tesmio_api.h"

namespace Agri {
const char* const kHash = "296644a9f207d609031fc2ae73fed2dcb34619a1d55a35d1c7b51965ce6841b8";
const char* const kIni = "plugins\\TESMIOAGRI.ini";
const unsigned char kHarvest[] = {0xF3,0x45,0x0F,0x59,0xC6,0xF3,0x45,0x0F,0x59,0xC5,0x48,0x8B,0x8E,0x70,0x09,0,0};
const unsigned char kGrowth[] = {0xF3,0x0F,0x58,0xCF,0xF3,0x0F,0x11,0x8F,0x38,0x0E,0,0};
const unsigned char kRestore[] = {0xF3,0x0F,0x10,0x97,0x38,0x0E,0,0};
struct Patch { size_t rva; const unsigned char* original; size_t size; size_t stub; };
const Patch kPatches[] = {{0x16EB21,kHarvest,sizeof(kHarvest),0},
    {0x16EEE0,kHarvest,sizeof(kHarvest),0x40},
    {0x40EFD1,kGrowth,sizeof(kGrowth),0x80},
    {0x40F00B,kRestore,sizeof(kRestore),0xC0}};
static const TsmHost* H = nullptr;
static bool gInitialized = false;
static int gResult = 1;

bool Rel32(const void* next, const void* target, int32_t& result) {
    const int64_t d = static_cast<int64_t>(reinterpret_cast<uintptr_t>(target)) -
                      static_cast<int64_t>(reinterpret_cast<uintptr_t>(next));
    if (d < INT32_MIN || d > INT32_MAX) return false;
    result = static_cast<int32_t>(d); return true;
}
void Bytes(std::vector<unsigned char>& b, const unsigned char* p, size_t n) {
    b.insert(b.end(),p,p+n);
}
bool Rip(std::vector<unsigned char>& b, unsigned char* address,
         const unsigned char* opcode, size_t n, const void* target) {
    int32_t d = 0;
    if (!Rel32(address+b.size()+n+4,target,d)) return false;
    Bytes(b,opcode,n); Bytes(b,reinterpret_cast<unsigned char*>(&d),4); return true;
}
bool Jump(std::vector<unsigned char>& b, unsigned char* address, const void* target) {
    const unsigned char op[] = {0xE9}; return Rip(b,address,op,1,target);
}

// Stubs use no DLL code, no callbacks and no new stack frame. Native EFLAGS
// from COMISS(step,0) must survive the growth detour's scalar SSE arithmetic.
bool Emit(unsigned index, unsigned char* code, unsigned char* base,
          const float* growth, const float* yield, std::vector<unsigned char>& b) {
    b.clear();
    if (index < 2) {
        Bytes(b,kHarvest,sizeof(kHarvest));
        const unsigned char mul[] = {0xF3,0x44,0x0F,0x59,0x05}; // MULSS XMM8,[RIP+cell]
        if (!Rip(b,code,mul,sizeof(mul),yield)) return false;
    } else if (index == 2) {
        // Original batch frame has outgoing-argument scratch at RSP+20..2F.
        // The intervening 0x122210 call takes RCX,RDX,XMM2 only; its callee
        // can use the 32-byte home area, but cannot own these next 16 bytes.
        // Restore all 128 bits before the next field or any other branch.
        const unsigned char save[] = {0x0F,0x11,0x7C,0x24,0x20,0x76,0x08};
        Bytes(b,save,sizeof(save)); // MOVUPS [RSP+20],XMM7; JBE skip MULSS
        const unsigned char mul[] = {0xF3,0x0F,0x59,0x3D};
        if (!Rip(b,code,mul,sizeof(mul),growth)) return false;
        Bytes(b,kGrowth,sizeof(kGrowth));
    } else if (index == 3) {
        const unsigned char restore[] = {0x0F,0x10,0x7C,0x24,0x20};
        Bytes(b,restore,sizeof(restore));
        Bytes(b,kRestore,sizeof(kRestore));
    } else return false;
    return Jump(b,code,base+kPatches[index].rva+kPatches[index].size);
}

bool Number(const char* text, float& out) {
    if (!text || !*text) return false;
    char* end = nullptr; const double v = std::strtod(text,&end);
    while (end && (*end==' ' || *end=='\t' || *end=='\r' || *end=='\n')) ++end;
    if (!end || *end || !std::isfinite(v) || v < 0.1 || v > 10.0) return false;
    out = static_cast<float>(v); return true;
}
bool Setting(const char* key, float& value) {
    char text[96] = {};
    H->configString(kIni,"agriculture",key,text,sizeof(text),"1.0");
    if (Number(text,value)) return true;
    H->log("TESMIOAGRI refused: %s must be a finite number in [0.1,10]; no patches installed",key);
    return false;
}

bool GrowthContext(unsigned char* base) {
    // Confirm the native frame and the complete path between the paired
    // growth/restore detours. An overlapping mod must not silently replace
    // the flags, branch target or nutrition call we depend on.
    const unsigned char frame[] = {0x4C,0x8B,0xDC,0x56,0x41,0x54,0x41,0x57,0x48,0x81,0xEC,0x30,0x01,0,0};
    const unsigned char flags[] = {0x0F,0x2F,0xFE};
    const unsigned char between[] = {0x76,0x2C,0x41,0x0F,0x2F,0xC8,0x0F,0x28,0xD7,0x76,0x1B,
        0xF3,0x41,0x0F,0x5C,0xC8,0x0F,0x28,0xC7,0xF3,0x0F,0x5C,0xC1,0x0F,0x57,0xC9,
        0xF3,0x0F,0x5A,0xC8,0x41,0x0F,0x54,0xCC,0x66,0x0F,0x5A,0xD1,0x48,0x8B,0xD7,0xE8,0x05,0x32,0xD1,0xFF};
    const unsigned char after[] = {0x41,0x0F,0x2F,0xD0,0x76,0x0A};
    const unsigned char nutrition[] = {0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xDA,0xE8,0x72,0xFB,0xFF,0xFF};
    return !std::memcmp(base+0x40EEB0,frame,sizeof(frame)) &&
        !std::memcmp(base+0x40EFCE,flags,sizeof(flags)) &&
        !std::memcmp(base+0x40EFDD,between,sizeof(between)) &&
        !std::memcmp(base+0x40F013,after,sizeof(after)) &&
        !std::memcmp(base+0x122210,nutrition,sizeof(nutrition));
}

// Match the exact EXE on disk, not merely a version label. CNG is a Windows
// system API; no extra runtime dependency is required.
bool ExactExe() {
    wchar_t path[32768] = {};
    if (!GetModuleFileNameW(static_cast<HMODULE>(H->exeModule),path,32768)) return false;
    HANDLE file = CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                             nullptr,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    BCRYPT_ALG_HANDLE alg = nullptr; BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD objectSize=0,got=0; unsigned char digest[32] = {};
    bool ok = BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if (ok) ok = BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectSize),
                                  sizeof(objectSize),&got,0)>=0;
    std::vector<unsigned char> object(objectSize), buffer(65536);
    if (ok) ok = BCryptCreateHash(alg,&hash,object.data(),objectSize,nullptr,0,0)>=0;
    while (ok) {
        DWORD n=0;
        if (!ReadFile(file,buffer.data(),static_cast<DWORD>(buffer.size()),&n,nullptr)) {ok=false;break;}
        if (!n) break;
        ok = BCryptHashData(hash,buffer.data(),n,0)>=0;
    }
    if (ok) ok = BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if (hash) BCryptDestroyHash(hash);
    if (alg) BCryptCloseAlgorithmProvider(alg,0);
    CloseHandle(file);
    char hex[65]={};
    for (unsigned i=0;i<32;++i) std::sprintf(hex+2*i,"%02x",digest[i]);
    return ok && std::strcmp(hex,kHash)==0;
}

#ifdef TESMIOAGRI_TEST
static int gWriteCount=0, gFailWrite=0;
#endif
bool Write(void* address, const unsigned char* bytes, size_t size) {
#ifdef TESMIOAGRI_TEST
    if (++gWriteCount == gFailWrite) return false;
#endif
    DWORD old=0,discard=0;
    if (!VirtualProtect(address,size,PAGE_EXECUTE_READWRITE,&old)) return false;
    std::memcpy(address,bytes,size);
    const bool flushed = FlushInstructionCache(GetCurrentProcess(),address,size)!=FALSE;
    const bool protectedAgain = VirtualProtect(address,size,old,&discard)!=FALSE;
    return flushed && protectedAgain;
}

bool Install(unsigned char* base, float growth, float yield) {
    std::vector<unsigned> selected;
    if (yield != 1.0f) {selected.push_back(0);selected.push_back(1);}
    if (growth != 1.0f) {selected.push_back(2);selected.push_back(3);}
    if (selected.empty()) return true;
    // Verify the ENTIRE group before allocating or changing any instruction.
    for (unsigned i : selected) {
        const Patch& p = kPatches[i];
        if (p.rva+p.size > H->exeSize || std::memcmp(base+p.rva,p.original,p.size)!=0) {
            H->log("TESMIOAGRI refused: original bytes mismatch at RVA 0x%zX; no patches installed",p.rva);
            return false;
        }
    }
    unsigned char* page = H->allocNear(base+0x40EEB0,4096);
    if (!page) {H->log("TESMIOAGRI refused: near allocation failed");return false;}
    DWORD old=0,discard=0;
    if (!VirtualProtect(page,4096,PAGE_READWRITE,&old)) return false;
    auto* growthCell = reinterpret_cast<float*>(page+0x3F0);
    auto* yieldCell = reinterpret_cast<float*>(page+0x3F4);
    *growthCell=growth; *yieldCell=yield;
    std::vector<std::vector<unsigned char>> replacements;
    bool ready = true;
    for (unsigned i : selected) {
        const Patch& p = kPatches[i]; std::vector<unsigned char> stub, jump;
        if (!Emit(i,page+p.stub,base,growthCell,yieldCell,stub) || stub.size()>0x40 ||
            !Jump(jump,base+p.rva,page+p.stub)) {ready=false;break;}
        std::memcpy(page+p.stub,stub.data(),stub.size());
        jump.resize(p.size,0x90); replacements.push_back(jump);
    }
    if (ready) ready = VirtualProtect(page,4096,PAGE_EXECUTE_READ,&discard)!=FALSE &&
                       FlushInstructionCache(GetCurrentProcess(),page,4096)!=FALSE;
    if (!ready) {
        VirtualFree(page,0,MEM_RELEASE);
        H->log("TESMIOAGRI refused: stub construction, range or protection check failed"); return false;
    }
    size_t attempted=0;
    for (size_t n=0;n<selected.size();++n) {
        attempted=n+1; const Patch& p=kPatches[selected[n]];
        if (!Write(base+p.rva,replacements[n].data(),p.size)) {
            bool rolledBack=true;
            while (attempted) {
                const Patch& r=kPatches[selected[--attempted]];
                if (!Write(base+r.rva,r.original,r.size)) rolledBack=false;
            }
            if (rolledBack) VirtualFree(page,0,MEM_RELEASE);
            H->log(rolledBack ? "TESMIOAGRI refused: patch write failed; group rolled back" :
                "TESMIOAGRI CRITICAL: rollback incomplete; restart required; stub memory retained");
            return false;
        }
    }
    H->log("TESMIOAGRI installed %zu scoped patches; positive growth x%.6g, new harvest quantity x%.6g",
           selected.size(),static_cast<double>(growth),static_cast<double>(yield));
    return true;
}
int Init(const TsmHost* host,TsmPluginInfo* info) {
    if (!host || !info) return 1;
    info->name="TESMIOAGRI"; info->version="0.1.0";
    if (gInitialized) return gResult;
    gInitialized=true; H=host;
    if (H->apiVersion<TSM_API_VERSION_MIN || H->structSize < offsetof(TsmHost,configString)+sizeof(H->configString) ||
        !H->log || !H->configInt || !H->configString || !H->allocNear || !H->exeBase) return 1;
    const int enabled=H->configInt(kIni,"agriculture","enabled",0);
    if (!enabled) {H->log("TESMIOAGRI disabled; no code changed");return 0;}
    if (enabled!=1) {H->log("TESMIOAGRI refused: enabled must be 0 or 1");return 1;}
    float time=1,yield=1;
    if (!Setting("maturity_time_scale",time) || !Setting("yield_multiplier",yield)) return 1;
    if (time==1 && yield==1) {
        H->log("TESMIOAGRI identity settings: maturity_time_scale=1, yield_multiplier=1; no code changed");return 0;
    }
    if (H->exeSize<0xA9D000) return 1;
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(H->exeBase);
    if (dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<=0 || static_cast<size_t>(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS64)>H->exeSize) return 1;
    auto* pe=reinterpret_cast<IMAGE_NT_HEADERS64*>(H->exeBase+dos->e_lfanew);
    if (pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
        pe->FileHeader.TimeDateStamp!=0x6A3EB6AD || pe->OptionalHeader.SizeOfImage!=0xA9D000 || !ExactExe()) {
        H->log("TESMIOAGRI refused: exact supported SOVIET64.exe 1.1.1.9 required; no code changed");return 1;
    }
    H->log("TESMIOAGRI settings: maturity_time_scale=%.6g, yield_multiplier=%.6g; cold decay unscaled",
           static_cast<double>(time),static_cast<double>(yield));
    if (time!=1 && !GrowthContext(H->exeBase)) {
        H->log("TESMIOAGRI refused: growth frame/continuation context mismatch; no code changed");return 1;
    }
    return Install(H->exeBase,1.0f/time,yield) ? 0 : 1;
}
}
extern "C" __declspec(dllexport) unsigned TsmPluginApiVersion() {return TSM_API_VERSION;}
extern "C" __declspec(dllexport) int TsmPluginInit(const TsmHost* h,TsmPluginInfo* i) {
    Agri::gResult=Agri::Init(h,i); return Agri::gResult;
}
extern "C" __declspec(dllexport) int TsmPluginStart() {return 0;}

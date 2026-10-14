#define TESMIOAGRI_TEST
#include "../src/TESMIOAGRI.cpp"
#include <cstdarg>
#include <cassert>
#include <algorithm>

static unsigned char* exe=nullptr;
static void Log(const char* f,...) {
    va_list a; va_start(a,f); std::vprintf(f,a); va_end(a); std::puts("");
}
static unsigned char* Allocate(unsigned char*,size_t n) {
    return static_cast<unsigned char*>(VirtualAlloc(nullptr,n,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
}
static int Enabled(const char*,const char*,const char*,int) {return 1;}
static int Config(const char*,const char*,const char*,char* s,int n,const char* f) {
    strcpy_s(s,static_cast<size_t>(n),f);return 0;
}
static void Require(bool condition,const char* msg) {
    if (!condition) {std::printf("FAIL: %s\n",msg);std::exit(1);}
}
static void OriginalSites() {
    for (const auto& p:Agri::kPatches) std::memcpy(exe+p.rva,p.original,p.size);
}
static void Append(std::vector<unsigned char>& b,std::initializer_list<unsigned char> v) {
    b.insert(b.end(),v.begin(),v.end());
}
// Native Win64 adapters preserve the caller's nonvolatile registers. Synthetic
// executable memory lets the real generated detours and continuations execute.
static void YieldAdapter(unsigned index) {
    std::vector<unsigned char> b;
    Append(b,{0x56,0x48,0x83,0xEC,0x30,0x44,0x0F,0x11,0x34,0x24,
        0x44,0x0F,0x11,0x6C,0x24,0x10,0x44,0x0F,0x11,0x44,0x24,0x20,0x48,0x8B,0xF1,
        0x44,0x0F,0x28,0xC1,0x44,0x0F,0x28,0xF2,0x44,0x0F,0x28,0xEB});
    Require(Agri::Jump(b,exe+0x1000,exe+Agri::kPatches[index].rva),"yield adapter range");
    std::memcpy(exe+0x1000,b.data(),b.size());
    std::vector<unsigned char> tail;
    // RCX is the storage vector reloaded by the original stolen instruction.
    // Record the same delta as native inventory + statistics consumers.
    Append(tail,{0xF3,0x44,0x0F,0x11,0x41,0x08,0xF3,0x44,0x0F,0x11,0x41,0x0C,
        0x41,0x0F,0x28,0xC0,0x44,0x0F,0x10,0x34,0x24,
        0x44,0x0F,0x10,0x6C,0x24,0x10,0x44,0x0F,0x10,0x44,0x24,0x20,0x48,0x83,0xC4,0x30,0x5E,0xC3});
    const auto& p=Agri::kPatches[index]; std::memcpy(exe+p.rva+p.size,tail.data(),tail.size());
}
static void GrowthAdapter() {
    std::vector<unsigned char> b;
    // push RDI; allocate 0x60 (16-byte aligned) including a 32-byte home area
    // and separate saves. Args: RCX field, XMM1 initial, XMM2 signed step.
    Append(b,{0x57,0x48,0x83,0xEC,0x60,0x0F,0x11,0x74,0x24,0x30,
        0x0F,0x11,0x7C,0x24,0x40,0x44,0x0F,0x11,0x44,0x24,0x50,
        0x48,0x8B,0xF9,0x0F,0x28,0xFA,0x0F,0x57,0xF6,
        0xF3,0x44,0x0F,0x10,0x05});
    int32_t d=0; Require(Agri::Rel32(exe+0x2000+b.size()+4,exe+0x3000,d),"one range");
    Agri::Bytes(b,reinterpret_cast<unsigned char*>(&d),4);
    Append(b,{0x0F,0x2F,0xFE}); // COMISS XMM7,XMM6, original flags
    Require(Agri::Jump(b,exe+0x2000,exe+0x40EFD1),"growth adapter range");
    std::memcpy(exe+0x2000,b.data(),b.size());
    // Simplified native continuation: JBE skips nutrition on negative/zero;
    // native positive path passes effective delta capped at remaining growth.
    b.clear(); Append(b,{0x76,0x2C,0x41,0x0F,0x2F,0xC8,0x0F,0x28,0xD7,
        0x76,0x09,0xF3,0x41,0x0F,0x5C,0xC8,0xF3,0x0F,0x5C,0xD1});
    Append(b,{0xF3,0x0F,0x11,0x97,0x00,0x0F,0,0});
    // Clobber the entire allowed outgoing home area, as any real callee may.
    Append(b,{0x0F,0x57,0xC0,0x0F,0x11,0x04,0x24,0x0F,0x11,0x44,0x24,0x10});
    Require(Agri::Jump(b,exe+0x40EFDD,exe+0x40F00B),"growth continuation");
    b.resize(0x2E,0x90); std::memcpy(exe+0x40EFDD,b.data(),b.size());
    // After restore, cap progress and report XMM7 to detect step leakage.
    b.clear(); Append(b,{0x41,0x0F,0x2F,0xD0,0x76,0x0A,
        0xC7,0x87,0x38,0x0E,0,0,0,0,0x80,0x3F,
        0xF3,0x0F,0x11,0xBF,0x04,0x0F,0,0,
        0xF3,0x0F,0x10,0x87,0x38,0x0E,0,0,
        0x0F,0x10,0x74,0x24,0x30,0x0F,0x10,0x7C,0x24,0x40,
        0x44,0x0F,0x10,0x44,0x24,0x50,0x48,0x83,0xC4,0x60,0x5F,0xC3});
    std::memcpy(exe+0x40F013,b.data(),b.size());
    *reinterpret_cast<float*>(exe+0x3000)=1;
}
static bool Close(float a,float b) {return std::fabs(a-b)<0.000002f;}
int main() {
    float v=0;
    for (const char* text : {"", "nan", "inf", "0", "-1", "0.09", "10.1", "2junk"})
        Require(!Agri::Number(text,v),"invalid config rejected");
    for (const char* text : {"0.1","0.25","1", "10", "2.5  "})
        Require(Agri::Number(text,v),"valid config accepted");
    int32_t d=0;
    Require(!Agri::Rel32(reinterpret_cast<void*>(0x1000),reinterpret_cast<void*>(0x100001000),d),"far jump rejected");
    exe=Allocate(nullptr,0xA9D000);Require(exe!=nullptr,"synthetic EXE allocation");
    TsmHost host={};host.apiVersion=4;host.structSize=sizeof(host);host.exeBase=exe;
    host.exeSize=0xA9D000;host.log=Log;host.allocNear=Allocate;host.configInt=Enabled;host.configString=Config;
    Agri::H=&host;
    OriginalSites();
    Require(Agri::Install(exe,1,1),"identity accepted");
    for (const auto& p:Agri::kPatches) Require(!memcmp(exe+p.rva,p.original,p.size),"identity does not write");
    // Exact bytes mismatch must reject the group before any write.
    exe[0x16EEE0]=0xCC;
    Require(!Agri::Install(exe,4,2),"mismatched site rejected");
    Require(!memcmp(exe+0x16EB21,Agri::kHarvest,sizeof(Agri::kHarvest)),"mismatch leaves first site intact");
    OriginalSites(); Agri::gWriteCount=0; Agri::gFailWrite=3;
    Require(!Agri::Install(exe,4,2),"injected third write failure rejected");
    for (const auto& p:Agri::kPatches) Require(!memcmp(exe+p.rva,p.original,p.size),"failed group fully rolls back");
    Agri::gFailWrite=0;
    for (float mult : {0.5f,2.0f,4.0f}) {
        OriginalSites();Require(Agri::Install(exe,mult,mult),"nonidentity install");
        unsigned char field[0x1000]={}; float records[4]={};
        *reinterpret_cast<float**>(field+0x970)=records;
        for (unsigned i=0;i<2;++i) {
            YieldAdapter(i);
            auto f=reinterpret_cast<float(*)(void*,float,float,float)>(exe+0x1000);
            const float result=f(field,3.0f,0.8f,0.75f);
            Require(Close(result,1.8f*mult),"manual/mechanical delta scaled once");
            Require(Close(records[2],result) && Close(records[3],result),"inventory and statistics get identical delta");
        }
        GrowthAdapter();auto f=reinterpret_cast<float(*)(void*,float,float)>(exe+0x2000);
        for (float step : {0.019f,0.0f,-0.019f}) {
            std::memset(field,0,sizeof(field)); const float result=f(field,0.4f,step);
            const float actual = step>0 ? step*mult : step;
            Require(Close(result,0.4f+actual),"positive scaled, zero/cold unchanged");
            Require(Close(*reinterpret_cast<float*>(field+0xF04),step),"original step restored for next field");
            const float nutrient=*reinterpret_cast<float*>(field+0xF00);
            Require(Close(nutrient,step>0?actual:0),"nutrition receives effective positive growth");
        }
        std::memset(field,0,sizeof(field));
        Require(Close(f(field,0.999f,0.019f),1),"maturity capped at one");
        Require(Close(*reinterpret_cast<float*>(field+0xF00),0.001f),"nutrition capped to remaining growth");
    }
    std::puts("PASS: native detours, both harvest paths, cold/zero growth, step restore, cap/nutrition, config, range, identity, mismatch, rollback");
    VirtualFree(exe,0,MEM_RELEASE);return 0;
}

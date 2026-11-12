// Regression coverage for the ARM native-call register/stack boundary.
// Contributed by MrK45
// https://gamedev.net/forums/topic/720811-armv7-hard-float-argument-corruption-in-native-calls-across-the-registerstack-boundary/

#include "utils.h"
#include <cstdlib>
#include <cstring>

namespace TestArmNativeAbi
{
static const asQWORD g_qwFirst = 0x1122334455667788ULL;
static const asQWORD g_qwSecond = 0x123456789abcdef0ULL;
static const asQWORD g_qwThird = 0xfedcba9876543210ULL;
static const asINT64 g_llFirst = -0x112233445566778LL;
static const asINT64 g_llSecond = -0x123456789abcdefLL;

struct AbiResult
{
    asQWORD m_qwValue;
    asQWORD m_qwTag;
    asQWORD m_qwGuard;
};

class AbiHost
{
public:
    bool Check(bool gate, asQWORD first, asQWORD second);
    AbiResult Build(bool gate, asQWORD first, asQWORD second);
};

class AbiBase
{
public:
    virtual bool Check(bool gate, asQWORD first, asQWORD second);
    virtual AbiResult Build(bool gate, asQWORD first, asQWORD second);
};

class AbiDerived : public AbiBase
{
public:
    bool Check(bool gate, asQWORD first, asQWORD second) override;
    AbiResult Build(bool gate, asQWORD first, asQWORD second) override;
};

class AbiFunctor
{
public:
    bool First(AbiHost *owner, bool gate, asQWORD first, asQWORD second);
    bool Last(bool gate, asQWORD first, asQWORD second, AbiHost *owner);
};

static AbiHost g_host;
static AbiDerived g_virtualHost;
static AbiFunctor g_functor;

static bool Pair(bool gate, asQWORD first, asQWORD second)
{
    bool bValid = gate && first == g_qwFirst && second == g_qwSecond;

    if (!bValid)
        PRINTF("ABI args gate=%d first=0x%016llx second=0x%016llx\n", gate,
            (unsigned long long)first, (unsigned long long)second);
    return bValid;
}

static AbiResult Result(bool gate, asQWORD first, asQWORD second)
{
    AbiResult abiResult;

    abiResult.m_qwValue = Pair(gate, first, second) ? second : 0;
    abiResult.m_qwTag = g_qwFirst;
    abiResult.m_qwGuard = ~second;
    return abiResult;
}

bool AbiHost::Check(bool gate, asQWORD first, asQWORD second)
{
    return this == &g_host && Pair(gate, first, second);
}

AbiResult AbiHost::Build(bool gate, asQWORD first, asQWORD second)
{
    return Result(this == &g_host && gate, first, second);
}

bool AbiBase::Check(bool, asQWORD, asQWORD)
{
    return false;
}

AbiResult AbiBase::Build(bool, asQWORD, asQWORD)
{
    return Result(false, 0, 0);
}

bool AbiDerived::Check(bool gate, asQWORD first, asQWORD second)
{
    return this == &g_virtualHost && Pair(gate, first, second);
}

AbiResult AbiDerived::Build(bool gate, asQWORD first, asQWORD second)
{
    return Result(this == &g_virtualHost && gate, first, second);
}

bool AbiFunctor::First(AbiHost *owner, bool gate, asQWORD first, asQWORD second)
{
    return this == &g_functor && owner == &g_host && Pair(gate, first, second);
}

bool AbiFunctor::Last(bool gate, asQWORD first, asQWORD second, AbiHost *owner)
{
    return this == &g_functor && owner == &g_host && Pair(gate, first, second);
}

static bool One(asQWORD value)
{
    return value == g_qwFirst;
}

static bool Two(asQWORD first, asQWORD second)
{
    return Pair(true, first, second);
}

static bool Three(asQWORD first, asQWORD second, asQWORD third)
{
    return Pair(true, first, second) && third == g_qwThird;
}

static bool Signed(bool gate, asINT64 first, asINT64 second)
{
    return gate && first == g_llFirst && second == g_llSecond;
}

static bool AfterThree(asDWORD first, asDWORD second, asDWORD third, asQWORD value)
{
    return first == 11 && second == 22 && third == 33 && value == g_qwSecond;
}

static bool AfterFour(asDWORD first, asDWORD second, asDWORD third, asDWORD fourth, asQWORD value)
{
    return first == 11 && second == 22 && third == 33 && fourth == 44 && value == g_qwSecond;
}

static bool StackMany(asDWORD first, asDWORD second, asDWORD third, asDWORD fourth,
    asQWORD value, asDWORD middle, asQWORD next, asINT64 signedValue, asDWORD tail)
{
    return first == 11 && second == 22 && third == 33 && fourth == 44 &&
        value == g_qwFirst && middle == 55 && next == g_qwSecond && signedValue == g_llFirst && tail == 66;
}

static bool Mixed(float firstFloat, bool gate, asQWORD first, double firstDouble, asQWORD second,
    float lastFloat, asDWORD tail)
{
    return Pair(gate, first, second) && firstFloat == 1.25f && firstDouble == 2.5 && lastFloat == 3.75f && tail == 77;
}

static bool References(const asQWORD &first, const asQWORD &second)
{
    return Pair(true, first, second);
}

static bool ObjectFirst(AbiHost *owner, bool gate, asQWORD first, asQWORD second)
{
    return owner == &g_host && Pair(gate, first, second);
}

static bool ObjectLast(bool gate, asQWORD first, asQWORD second, AbiHost *owner)
{
    return owner == &g_host && Pair(gate, first, second);
}

static AbiResult ResultFirst(AbiHost *owner, bool gate, asQWORD first, asQWORD second)
{
    return Result(owner == &g_host && gate, first, second);
}

static AbiResult ResultLast(bool gate, asQWORD first, asQWORD second, AbiHost *owner)
{
    return Result(owner == &g_host && gate, first, second);
}

static void Generic(asIScriptGeneric *generic)
{
    generic->SetReturnByte(Pair(generic->GetArgByte(0) != 0, generic->GetArgQWord(1), generic->GetArgQWord(2)));
}

static bool SpillFloats(asDWORD first, asDWORD second, asDWORD third, asDWORD fourth, float value0, float value1, float value2, float value3, float value4, float value5, float value6, float value7, float value8, float value9, float value10, float value11, float value12, float value13, float value14, float value15, float value16, float value17, asQWORD last)
{
    return first == 11 && second == 22 && third == 33 &&
        fourth == 44 && value0 == 1.25f && value1 == 2.25f &&
        value2 == 3.25f && value3 == 4.25f && value4 == 5.25f &&
        value5 == 6.25f && value6 == 7.25f && value7 == 8.25f &&
        value8 == 9.25f && value9 == 10.25f && value10 == 11.25f &&
        value11 == 12.25f && value12 == 13.25f && value13 == 14.25f &&
        value14 == 15.25f && value15 == 16.25f && value16 == 17.25f &&
        value17 == 18.25f && last == g_qwSecond;
}

static bool SpillDoubles(asDWORD first, asDWORD second, asDWORD third, asDWORD fourth, double value0, double value1, double value2, double value3, double value4, double value5, double value6, double value7, double value8, double value9, asQWORD last)
{
    return first == 11 && second == 22 && third == 33 &&
        fourth == 44 && value0 == 1.25 && value1 == 2.25 &&
        value2 == 3.25 && value3 == 4.25 && value4 == 5.25 &&
        value5 == 6.25 && value6 == 7.25 && value7 == 8.25 &&
        value8 == 9.25 && value9 == 10.25 && last == g_qwSecond;
}

static bool Register(int code, const char *declaration)
{
    if (code >= 0)
        return true;
    PRINTF("ABI registration failed: %s (%d)\n", declaration, code);
    return false;
}

struct AbiCase
{
    const char *m_szName;
    const char *m_szBody;
};

bool Test()
{
    if (strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
    {
        PRINTF("Skipping native ABI matrix in AS_MAX_PORTABILITY\n");
        return false;
    }

    asIScriptEngine *pEngine = asCreateScriptEngine();
    COutStream outStream;
    bool bRegistered = true;
 	bool fail = false;
    unsigned int uiRun = 0;
    unsigned int uiFailures = 0;

    pEngine->SetMessageCallback(asMETHOD(COutStream, Callback), &outStream, asCALL_THISCALL);
#define ABI_GLOBAL(declaration, function, convention) \
    bRegistered = Register(pEngine->RegisterGlobalFunction(declaration, asFUNCTION(function), convention), declaration) && bRegistered
#define ABI_METHOD(declaration, function, convention) \
    bRegistered = Register(pEngine->RegisterObjectMethod("AbiHost", declaration, function, convention), declaration) && bRegistered
#define ABI_RESULT(expression) \
    "AbiResult r = " expression "; return r.value == uint64(0x123456789abcdef0) && r.tag == uint64(0x1122334455667788) && r.guard == uint64(0xedcba9876543210f);"
    ABI_GLOBAL("bool AbiPair(bool, uint64, uint64)", Pair, asCALL_CDECL);
    ABI_GLOBAL("bool AbiOne(uint64)", One, asCALL_CDECL);
    ABI_GLOBAL("bool AbiTwo(uint64, uint64)", Two, asCALL_CDECL);
    ABI_GLOBAL("bool AbiThree(uint64, uint64, uint64)", Three, asCALL_CDECL);
    ABI_GLOBAL("bool AbiSigned(bool, int64, int64)", Signed, asCALL_CDECL);
    ABI_GLOBAL("bool AbiAfterThree(uint, uint, uint, uint64)", AfterThree, asCALL_CDECL);
    ABI_GLOBAL("bool AbiAfterFour(uint, uint, uint, uint, uint64)", AfterFour, asCALL_CDECL);
    ABI_GLOBAL("bool AbiStackMany(uint, uint, uint, uint, uint64, uint, uint64, int64, uint)", StackMany, asCALL_CDECL);
    ABI_GLOBAL("bool AbiMixed(float, bool, uint64, double, uint64, float, uint)", Mixed, asCALL_CDECL);
    ABI_GLOBAL("bool AbiReferences(const uint64 &in, const uint64 &in)", References, asCALL_CDECL);
    ABI_GLOBAL("bool AbiGeneric(bool, uint64, uint64)", Generic, asCALL_GENERIC);
    ABI_GLOBAL("bool AbiSpillFloats(uint, uint, uint, uint, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, float, uint64)", SpillFloats, asCALL_CDECL);
    ABI_GLOBAL("bool AbiSpillDoubles(uint, uint, uint, uint, double, double, double, double, double, double, double, double, double, double, uint64)", SpillDoubles, asCALL_CDECL);

    bRegistered = Register(pEngine->RegisterObjectType("AbiHost", 0, asOBJ_REF | asOBJ_NOCOUNT), "AbiHost") && bRegistered;
    bRegistered = Register(pEngine->RegisterObjectType("AbiVirtual", 0, asOBJ_REF | asOBJ_NOCOUNT), "AbiVirtual") && bRegistered;
    bRegistered = Register(pEngine->RegisterGlobalProperty("AbiHost host", &g_host), "host") && bRegistered;
    bRegistered = Register(pEngine->RegisterGlobalProperty("AbiVirtual virtualHost", static_cast<AbiBase *>(&g_virtualHost)), "virtualHost") && bRegistered;
    ABI_METHOD("bool Check(bool, uint64, uint64)", asMETHOD(AbiHost, Check), asCALL_THISCALL);
    ABI_METHOD("bool ObjectFirst(bool, uint64, uint64)", asFUNCTION(ObjectFirst), asCALL_CDECL_OBJFIRST);
    ABI_METHOD("bool ObjectLast(bool, uint64, uint64)", asFUNCTION(ObjectLast), asCALL_CDECL_OBJLAST);
    bRegistered = Register(pEngine->RegisterObjectMethod("AbiVirtual", "bool Check(bool, uint64, uint64)", asMETHOD(AbiBase, Check), asCALL_THISCALL), "virtual Check") && bRegistered;
    bRegistered = Register(pEngine->RegisterObjectMethod("AbiHost", "bool FunctorFirst(bool, uint64, uint64)", asMETHOD(AbiFunctor, First), asCALL_THISCALL_OBJFIRST, &g_functor), "FunctorFirst") && bRegistered;
    bRegistered = Register(pEngine->RegisterObjectMethod("AbiHost", "bool FunctorLast(bool, uint64, uint64)", asMETHOD(AbiFunctor, Last), asCALL_THISCALL_OBJLAST, &g_functor), "FunctorLast") && bRegistered;
    bRegistered = Register(pEngine->RegisterObjectType("AbiResult", sizeof(AbiResult), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS | asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_ALIGN8), "AbiResult") && bRegistered;
    bRegistered = Register(pEngine->RegisterObjectProperty("AbiResult", "uint64 value", asOFFSET(AbiResult, m_qwValue)), "value") && bRegistered;
    bRegistered = Register(pEngine->RegisterObjectProperty("AbiResult", "uint64 tag", asOFFSET(AbiResult, m_qwTag)), "tag") && bRegistered;
    bRegistered = Register(pEngine->RegisterObjectProperty("AbiResult", "uint64 guard", asOFFSET(AbiResult, m_qwGuard)), "guard") && bRegistered;
    ABI_GLOBAL("AbiResult AbiResultCall(bool, uint64, uint64)", Result, asCALL_CDECL);
    ABI_METHOD("AbiResult Build(bool, uint64, uint64)", asMETHOD(AbiHost, Build), asCALL_THISCALL);
    ABI_METHOD("AbiResult ResultFirst(bool, uint64, uint64)", asFUNCTION(ResultFirst), asCALL_CDECL_OBJFIRST);
    ABI_METHOD("AbiResult ResultLast(bool, uint64, uint64)", asFUNCTION(ResultLast), asCALL_CDECL_OBJLAST);
    bRegistered = Register(pEngine->RegisterObjectMethod("AbiVirtual", "AbiResult Build(bool, uint64, uint64)", asMETHOD(AbiBase, Build), asCALL_THISCALL), "virtual Build") && bRegistered;

    const AbiCase arrCases[] =
    {
        {"cdecl_one", "return AbiOne(uint64(0x1122334455667788));"},
        {"cdecl_two", "return AbiTwo(uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"cdecl_three", "return AbiThree(uint64(0x1122334455667788),uint64(0x123456789abcdef0),uint64(0xfedcba9876543210));"},
        {"cdecl_bool_u64_u64", "return AbiPair(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"cdecl_signed", "return AbiSigned(true,int64(-0x112233445566778),int64(-0x123456789abcdef));"},
        {"cdecl_three_u32_u64", "return AbiAfterThree(11,22,33,uint64(0x123456789abcdef0));"},
        {"cdecl_four_u32_u64", "return AbiAfterFour(11,22,33,44,uint64(0x123456789abcdef0));"},
        {"cdecl_stack_many", "return AbiStackMany(11,22,33,44,uint64(0x1122334455667788),55,uint64(0x123456789abcdef0),int64(-0x112233445566778),66);"},
        {"cdecl_mixed_vfp", "return AbiMixed(1.25f,true,uint64(0x1122334455667788),2.5,uint64(0x123456789abcdef0),3.75f,77);"},
        {"references_control", "return AbiReferences(uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"generic_control", "return AbiGeneric(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"cdecl_spill_floats", "return AbiSpillFloats(11,22,33,44,1.25f,2.25f,3.25f,4.25f,5.25f,6.25f,7.25f,8.25f,9.25f,10.25f,11.25f,12.25f,13.25f,14.25f,15.25f,16.25f,17.25f,18.25f,uint64(0x123456789abcdef0));"},
        {"cdecl_spill_doubles", "return AbiSpillDoubles(11,22,33,44,1.25,2.25,3.25,4.25,5.25,6.25,7.25,8.25,9.25,10.25,uint64(0x123456789abcdef0));"},
        {"thiscall", "return host.Check(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"virtual_thiscall", "return virtualHost.Check(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"cdecl_objfirst", "return host.ObjectFirst(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"cdecl_objlast", "return host.ObjectLast(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"thiscall_objfirst", "return host.FunctorFirst(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"thiscall_objlast", "return host.FunctorLast(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0));"},
        {"sret_cdecl", ABI_RESULT("AbiResultCall(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0))")},
        {"sret_thiscall", ABI_RESULT("host.Build(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0))")},
        {"sret_virtual", ABI_RESULT("virtualHost.Build(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0))")},
        {"sret_objfirst", ABI_RESULT("host.ResultFirst(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0))")},
        {"sret_objlast", ABI_RESULT("host.ResultLast(true,uint64(0x1122334455667788),uint64(0x123456789abcdef0))")},
    };

    if (!bRegistered)
    {
        pEngine->ShutDownAndRelease();
        return true;
    }
    std::stringstream output;
    for (unsigned int uiIndex = 0; uiIndex < sizeof(arrCases) / sizeof(arrCases[0]); ++uiIndex)
    {
        asIScriptModule *pModule = pEngine->GetModule("abi", asGM_ALWAYS_CREATE);
        asIScriptContext *pContext = pEngine->CreateContext();
        std::string strCode = "bool Run() { ";
        int nBuild;
        int nExecution = -1;
        bool bPassed = false;

        strCode += arrCases[uiIndex].m_szBody;
        strCode += " }";
        pModule->AddScriptSection(arrCases[uiIndex].m_szName, strCode.c_str());
        nBuild = pModule->Build();
        if (nBuild >= 0 && pContext->Prepare(pModule->GetFunctionByDecl("bool Run()")) >= 0)
        {
            nExecution = pContext->Execute();
            bPassed = nExecution == asEXECUTION_FINISHED && pContext->GetReturnByte() != 0;
        }
        ++uiRun;
        if (!bPassed)
        {
            ++uiFailures;
            TEST_FAILED;
        }
        output << "ABI_CASE " << arrCases[uiIndex].m_szName << " " << (bPassed ? "PASS" : "FAIL") << " build=" << nBuild << " execution=" << nExecution << "\n";
        pContext->Release();
    }
    output << "ABI_SUMMARY " << uiRun << " cases, " << uiFailures << " failed\n";
    if( output.str() != 
        R"(ABI_CASE cdecl_one PASS build=0 execution=0
ABI_CASE cdecl_two PASS build=0 execution=0
ABI_CASE cdecl_three PASS build=0 execution=0
ABI_CASE cdecl_bool_u64_u64 PASS build=0 execution=0
ABI_CASE cdecl_signed PASS build=0 execution=0
ABI_CASE cdecl_three_u32_u64 PASS build=0 execution=0
ABI_CASE cdecl_four_u32_u64 PASS build=0 execution=0
ABI_CASE cdecl_stack_many PASS build=0 execution=0
ABI_CASE cdecl_mixed_vfp PASS build=0 execution=0
ABI_CASE references_control PASS build=0 execution=0
ABI_CASE generic_control PASS build=0 execution=0
ABI_CASE cdecl_spill_floats PASS build=0 execution=0
ABI_CASE cdecl_spill_doubles PASS build=0 execution=0
ABI_CASE thiscall PASS build=0 execution=0
ABI_CASE virtual_thiscall PASS build=0 execution=0
ABI_CASE cdecl_objfirst PASS build=0 execution=0
ABI_CASE cdecl_objlast PASS build=0 execution=0
ABI_CASE thiscall_objfirst PASS build=0 execution=0
ABI_CASE thiscall_objlast PASS build=0 execution=0
ABI_CASE sret_cdecl PASS build=0 execution=0
ABI_CASE sret_thiscall PASS build=0 execution=0
ABI_CASE sret_virtual PASS build=0 execution=0
ABI_CASE sret_objfirst PASS build=0 execution=0
ABI_CASE sret_objlast PASS build=0 execution=0
ABI_SUMMARY 24 cases, 0 failed
)" )
    {
        PRINTF(output.str().c_str());
        TEST_FAILED;
    }

    pEngine->ShutDownAndRelease();
    return fail;
#undef ABI_RESULT
#undef ABI_METHOD
#undef ABI_GLOBAL
}
}

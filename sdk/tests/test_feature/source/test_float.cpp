#include "utils.h"

#if _HAS_CXX17
#include <charconv>
#endif
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <string>
#include <vector>
#include <locale>
#include <iostream>
#include <math.h>

BEGIN_AS_NAMESPACE
double asStringScanDouble(const char *string, size_t *numScanned);
END_AS_NAMESPACE

namespace TestFloat
{

static const char *script =
"void TestFloat()                               \n"
"{                                              \n"
"  float a = 2, b = 3, c = 1;                   \n"
"  c = a + b;                                   \n"
"  a = b + 23;                                  \n"
"  b = 12 + c;                                  \n"
"  c = a - b;                                   \n"
"  a = b - 23;                                  \n"
"  b = 12 - c;                                  \n"
"  c = a * b;                                   \n"
"  a = b * 23;                                  \n"
"  b = 12 * c;                                  \n"
"  c = a / b;                                   \n"
"  a = b / 23;                                  \n"
"  b = 12 / c;                                  \n"
"  c = a % b;                                   \n"
"  a = b % 23;                                  \n"
"  b = 12 % c;                                  \n"
"  a++;                                         \n"
"  ++a;                                         \n"
"  a += b;                                      \n"
"  a += 3;                                      \n"
"  a /= c;                                      \n"
"  a /= 5;                                      \n"
"  a = b = c;                                   \n"
"  func( a-1, b, c );                           \n"
"  a = -b;                                      \n"
"  a = func2();                                 \n"
"}                                              \n"
"void func(float a, float &in b, float &out c)  \n"
"{                                              \n"
"  c = a + b;                                   \n"
"  b = c;                                       \n"
"  g = g;                                       \n"
"}                                              \n"
"float g = 0;                                   \n"
"float func2()                                  \n"
"{                                              \n"
"  return g + 1;                                \n"
"}                                              \n";


static const char *script2 =
"void start()           \n"
"{                      \n"
"float test = 1.9f;     \n"
"print(test);           \n"
"print(test + test);    \n"
"}                      \n";

void print_gen(asIScriptGeneric *gen)
{
	float val = *(float*)gen->GetAddressOfArg(0);
	UNUSED_VAR(val);
}

static unsigned long long bitsOf(double d) { unsigned long long u; std::memcpy(&u, &d, 8); return u; }

static double reference(const std::string &s, size_t len)
{
    std::string t;
    for (size_t i = 0; i < len; ++i) if (s[i] != '\'') t += s[i];
    double d = 0;
#if _HAS_CXX17
    std::from_chars(t.data(), t.data() + t.size(), d);
#else
    std::string tmp(t.data(), t.data() + t.size());
    char *end;
    d = strtod(tmp.c_str(), &end);
#endif
    return d;
}

// Does the literal fall in the fast path domain, judged independently of the code under test?
static bool inFastDomain(const std::string &s, size_t len)
{
    std::string digits; int fractionDigits = 0; bool inFraction = false; long exponent = 0; bool negExp = false;
    size_t i = 0;
    for (; i < len && s[i] != 'e' && s[i] != 'E'; ++i)
    {
        if (s[i] == '.') inFraction = true;
        else if (s[i] >= '0' && s[i] <= '9') { digits += s[i]; if (inFraction) ++fractionDigits; }
    }
    if (i < len) { ++i; if (s[i] == '-') { negExp = true; ++i; } else if (s[i] == '+') ++i;
        for (; i < len; ++i) if (s[i] >= '0' && s[i] <= '9') exponent = exponent * 10 + (s[i] - '0'); }
    size_t nz = digits.find_first_not_of('0');
    std::string sig = nz == std::string::npos ? "0" : digits.substr(nz);
    if (sig.size() > 15) return false;              // conservative: < 2^53 guaranteed
    long e10 = (negExp ? -exponent : exponent) - fractionDigits;
    return e10 >= -22 && e10 <= 22;
}

struct Stats { long n = 0, scriptWrong = 0, scriptClose = 0, domainWrong = 0, floatScriptWrong = 0; };

static std::string reportString;

static void check(const std::string &s, Stats &st, bool verbose)
{
    size_t nP = 0;
    double p = asStringScanDouble(s.c_str(), &nP);

    ++st.n;
    double r = reference(s, nP);
    bool pOk = bitsOf(p) == bitsOf(r);
    bool pClose = false;
    if( !pOk )
    {
        // Every platform / compiler has its own way implementation, so it is not required to be bitwise compatible, but the result must be close enough
        double rMantissa, pMantissa;
        int rExp, pExp;
        rMantissa = frexp(r, &rExp);
        pMantissa = frexp(p, &pExp);

        if( (rExp - pExp == 0) && abs(rMantissa - pMantissa) < 1.12e-15 ) // TODO: Can the margin for error be reduced even further? Probably
        {
            pClose = true;
            ++st.scriptClose;
        }
    }

    if (!pOk && !pClose) ++st.scriptWrong;
    if (!pOk && inFastDomain(s, nP)) { ++st.domainWrong; std::printf("FAST DOMAIN WRONG %s\n", s.c_str()); }
    // float literals: compiler does float(asStringScanDouble(...))
    std::string t; for (size_t i = 0; i < nP; ++i) if (s[i] != '\'') t += s[i];
    float rf = 0; 
#if _HAS_CXX17
    std::from_chars(t.data(), t.data() + t.size(), rf);
#else
    std::string tmp(t.data(), t.data() + t.size());
    char *end;
    rf = strtof(tmp.c_str(), &end);
#endif
    if (float(p) != rf) ++st.floatScriptWrong;
    if( verbose )
    {
        char buf[1024] = {0};
        std::sprintf(buf, "  %-28s reference %-24.17g script %-24.17g %s\n", s.c_str(), r, p, pOk ? "ok" : (pClose ? "close" : "WRONG"));
        reportString += buf;
    }
}

static void report(const char *title, const Stats &st)
{
    char buf[1024] = {0};
    std::sprintf(buf, "%s: %ld literals | script wrong %ld | script close %ld | script wrong in fast domain %ld"
                " | script wrong %ld\n",
                title, st.n, st.scriptWrong, st.scriptClose, st.domainWrong, st.floatScriptWrong);
    reportString += buf;
}

bool Test()
{
	bool fail = false;
	COutStream out;
 	asIScriptEngine *engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
	engine->SetMessageCallback(asMETHOD(COutStream,Callback), &out, asCALL_THISCALL);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

	int r = ExecuteString(engine, "float a = 1e5; float b = 1.0e5; assert( a == b ); \n");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	r = ExecuteString(engine, "double a = 3; double b = 7; assert( a / 10 == 0.3 ); assert( b / 10 == 0.7 ); \n");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;


	asIScriptModule *mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script, strlen(script));
 	r = mod->Build();
	if( r < 0 ) TEST_FAILED;


	engine->RegisterGlobalFunction("void print(float)", asFUNCTION(print_gen), asCALL_GENERIC);

	mod = engine->GetModule(0, asGM_ALWAYS_CREATE);
	mod->AddScriptSection("script", script2, strlen(script2));
	r = mod->Build();
	if( r < 0 ) TEST_FAILED;

	r = ExecuteString(engine, "start()", mod);
	if (r != asEXECUTION_FINISHED) TEST_FAILED;

#if !defined(__psp2__)
	// The locale affects the way the compiler reads float values
	setlocale(LC_NUMERIC, "");
#endif

	float f;
	engine->RegisterGlobalProperty("float f", &f);
	r = ExecuteString(engine, "f = 3.14f;");
	if( r != asEXECUTION_FINISHED )
		TEST_FAILED;

	if( f < 3.139999f || f > 3.140001f )
		TEST_FAILED;

#if !defined(__psp2__)
	setlocale(LC_NUMERIC, "C");
#endif

	engine->Release();

	// Test that compiler properly scans the digits
	// http://www.gamedev.net/topic/662758-formatfloat-fails-for-documented-dbl-max/
	{
		engine = asCreateScriptEngine(ANGELSCRIPT_VERSION);
		engine->SetMessageCallback(asMETHOD(COutStream, Callback), &out, asCALL_THISCALL);

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterStdStringUtils(engine);

		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		r = ExecuteString(engine, "double d = 1.79769313486231e+308; \n"
								  "string s = formatFloat(d, 'e'); \n"
								  "assert( s == '2e+308' ); \n"
								  "d = 2.22507385850720e-308; \n"
								  "s = formatFloat(d, 'e'); \n"
								  "assert( s == '2e-308' ); \n"
								  "float f = 3.402823466e+38f; \n"
								  "s = formatFloat(f, 'e'); \n"
								  "assert( s == '3e+38' || s == '3e+038' ); \n" // The formatting varies from compiler to compiler, and version too
								  "f = 1.175494351e-38f; \n"
								  "s = formatFloat(f, 'e'); \n"
								  "assert( s == '1e-38' || s == '1e-038' ); \n" // The formatting varies from compiler to compiler, and version too
						);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		engine->ShutDownAndRelease();
	}

    // Test internal asStringScanDouble directly
    // Test harness provided by Rémy STIVANI
	{
        const char *edge[] = { "0.001", "0.123456789", "1.75", "0.05", "0.01", "0.005", "0.1", "2.5",
                               "0", "0.0", "1'000.5", "0.000'001", "1.5e3", "1.5E+3", "25e-3", "1e22", "1e23",
                               "9007199254740992.0", "9007199254740993.0", "123456789012345678901234567890.0",
                               "0.1e-30", "1e308", "1e-320", "0.00000000000000000000000001", "3.14159f" };
        Stats e;
        for (const char *s : edge) check(s, e, true);
        report("edge", e);

        std::mt19937_64 rng(411);
        auto digit = [&] { return char('0' + rng() % 10); };
        Stats rs;
        for (long k = 0; k < 10000; ++k)
        {
            std::string s;
            int nInt = int(rng() % 18), nFrac = int(rng() % 21);
            if (nInt == 0) s = "0"; else for (int i = 0; i < nInt; ++i) { if (i && rng() % 50 == 0) s += '\''; s += digit(); }
            if (nFrac || rng() % 2) { s += '.'; for (int i = 0; i < nFrac; ++i) { if (i && rng() % 50 == 0) s += '\''; s += digit(); } if (!nFrac) s += '0'; }
            if (rng() % 5 == 0) { s += (rng() % 2 ? 'e' : 'E'); int rnd = int(rng() % 3); if (rnd == 1) s += '-'; else if (rnd == 2) s += '+'; s += std::to_string(rng() % 40); }
            check(s, rs, false);
        }
        report("random", rs);
        if( e.scriptWrong > 0 ||
           e.scriptClose > 5 ||
           e.domainWrong || e.floatScriptWrong ||
           rs.scriptWrong > 0 ||
           rs.scriptClose > 2866 ||
           rs.domainWrong || rs.floatScriptWrong )
        {
            PRINTF("%s", reportString.c_str());
            TEST_FAILED;
        }
	}

	// Success
 	return fail;
}

} // namespace


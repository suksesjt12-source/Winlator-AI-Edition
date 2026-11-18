#include <jni.h>

#include "aie/pe_image.hpp"
#include "aie/x64_interpreter.hpp"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

jstring to_java_string(JNIEnv* env, const std::string& value) {
    return env->NewStringUTF(value.c_str());
}

const char* architecture_name(aie::engine::PeArchitecture architecture) {
    switch (architecture) {
        case aie::engine::PeArchitecture::X86: return "x86";
        case aie::engine::PeArchitecture::X86_64: return "x86-64";
        default: return "unsupported";
    }
}

} // namespace

extern "C" JNIEXPORT jstring JNICALL
Java_com_winlator_NativeExecutionEngine_nativeGetStatus(JNIEnv* env, jclass) {
    return to_java_string(
        env,
        "AI Edition native core loaded. PE32/PE32+ inspection and a limited x86-64 "
        "instruction and bounded guest-memory subsets are available; full Windows execution is not implemented."
    );
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_winlator_NativeExecutionEngine_nativeInspectExecutable(
    JNIEnv* env, jclass, jbyteArray file_bytes) {
    if (file_bytes == nullptr) {
        return to_java_string(env, "No executable data supplied.");
    }

    const jsize length = env->GetArrayLength(file_bytes);
    if (length <= 0) {
        return to_java_string(env, "Executable file is empty.");
    }

    jbyte* bytes = env->GetByteArrayElements(file_bytes, nullptr);
    if (bytes == nullptr) {
        return to_java_string(env, "Could not access executable bytes.");
    }

    const auto parsed = aie::engine::inspect_pe_image(
        reinterpret_cast<const std::uint8_t*>(bytes),
        static_cast<std::size_t>(length)
    );
    env->ReleaseByteArrayElements(file_bytes, bytes, JNI_ABORT);

    if (!parsed.ok) {
        return to_java_string(env, "Invalid or unsupported PE file: " + parsed.error);
    }

    std::ostringstream output;
    output << (parsed.image.format == aie::engine::PeFormat::PE32 ? "PE32" : "PE32+")
           << " / " << architecture_name(parsed.image.architecture)
           << " | sections: " << parsed.image.section_count
           << " | entry RVA: 0x" << std::hex << parsed.image.entry_point_rva
           << " | image size: 0x" << parsed.image.size_of_image;
    return to_java_string(env, output.str());
}


extern "C" JNIEXPORT jstring JNICALL
Java_com_winlator_NativeExecutionEngine_nativeRunInterpreterSelfTest(
    JNIEnv* env, jclass) {
    const auto result = aie::engine::run_x64_self_test();
    if (!result.returned()) {
        return to_java_string(env, "FAIL | interpreter stopped: " + result.error);
    }

    const bool passed = result.state.registers[0] == 45 &&
                        result.state.registers[1] == 40 &&
                        result.state.registers[2] == 40 &&
                        result.state.registers[4] == aie::engine::X64_STACK_SIZE;
    if (!passed) {
        std::ostringstream failure;
        failure << "FAIL | RAX=" << result.state.registers[0]
                << " | RCX=" << result.state.registers[1]
                << " | RDX=" << result.state.registers[2]
                << " | RSP=" << result.state.registers[4];
        return to_java_string(env, failure.str());
    }

    std::ostringstream output;
    output << "PASS | x86-64 interpreter executed its built-in test program"
           << " | RAX=" << result.state.registers[0]
           << " | RCX=" << result.state.registers[1]
           << " | RDX=" << result.state.registers[2]
           << " | guest memory read/write + arithmetic=pass"
           << " | stack restored=" << (result.state.registers[4] == aie::engine::X64_STACK_SIZE ? "yes" : "no")
           << " | instructions=" << result.instructions_executed;
    return to_java_string(env, output.str());
}


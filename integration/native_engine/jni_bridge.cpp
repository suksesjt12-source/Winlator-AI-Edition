#include <jni.h>

#include "aie/pe_image.hpp"

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
        "instruction subset are available; full Windows execution is not implemented."
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

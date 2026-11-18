package com.winlator;

/**
 * JNI entry point for the independent AI Edition native engine.
 * The bridge exposes PE metadata inspection and a fixed interpreter smoke test. It does not run Windows programs.
 */
public final class NativeExecutionEngine {
    private static final String LOAD_ERROR;

    static {
        String error = null;
        try {
            System.loadLibrary("aie_execution_engine");
        } catch (UnsatisfiedLinkError exception) {
            error = exception.getMessage();
        }
        LOAD_ERROR = error;
    }

    private NativeExecutionEngine() {}

    public static String getStatus() {
        if (LOAD_ERROR != null) {
            return "Native library unavailable: " + LOAD_ERROR;
        }
        return nativeGetStatus();
    }

    public static String inspectExecutable(byte[] fileBytes) {
        if (LOAD_ERROR != null) {
            return "Native library unavailable: " + LOAD_ERROR;
        }
        if (fileBytes == null || fileBytes.length == 0) {
            return "No executable data supplied.";
        }
        return nativeInspectExecutable(fileBytes);
    }

    public static String runInterpreterSelfTest() {
        if (LOAD_ERROR != null) {
            return "Native library unavailable: " + LOAD_ERROR;
        }
        return nativeRunInterpreterSelfTest();
    }

    private static native String nativeGetStatus();
    private static native String nativeInspectExecutable(byte[] fileBytes);
    private static native String nativeRunInterpreterSelfTest();
}

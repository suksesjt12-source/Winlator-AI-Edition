#!/usr/bin/env python3
"""Apply the Winlator AI Edition launcher, artwork and assistant to upstream Winlator."""

from pathlib import Path
import re
import shutil
import sys


def replace_once(path: Path, before: str, after: str, label: str) -> None:
    content = path.read_text(encoding="utf-8")
    if after.strip() in content:
        return
    if before not in content:
        raise RuntimeError(f"Could not locate insertion point for {label}: {path}")
    path.write_text(content.replace(before, after, 1), encoding="utf-8")


def ensure_cmake_subdirectory(cmake_file: Path, directory: str) -> None:
    content = cmake_file.read_text(encoding="utf-8")
    directive = f"add_subdirectory({directory})"
    if directive not in {line.strip() for line in content.splitlines()}:
        cmake_file.write_text(content.rstrip() + "\n\n" + directive + "\n",
                              encoding="utf-8")


def install_launcher_manifest(manifest: Path) -> None:
    content = manifest.read_text(encoding="utf-8")
    launcher_name = 'android:name="com.winlator.LauncherActivity"'
    if launcher_name not in content:
        pattern = re.compile(
            r'<activity android:name="com\.winlator\.MainActivity"[^>]*>[\s\S]*?</activity>'
        )
        match = pattern.search(content)
        if not match:
            raise RuntimeError("Could not locate MainActivity declaration in upstream manifest.")

        main_block = match.group(0)
        main_block = re.sub(
            r'\s*<intent-filter>\s*<action android:name="android.intent.action.MAIN"/>'
            r'\s*<category android:name="android.intent.category.LAUNCHER"/>'
            r'\s*</intent-filter>',
            "",
            main_block,
        )
        main_block = main_block.replace('android:exported="true"', 'android:exported="false"', 1)

        launcher_block = (
            '<activity android:name="com.winlator.LauncherActivity"\n'
            '            android:theme="@style/AppThemeDark"\n'
            '            android:exported="true"\n'
            '            android:screenOrientation="unspecified">\n'
            '            <intent-filter>\n'
            '                <action android:name="android.intent.action.MAIN"/>\n'
            '                <category android:name="android.intent.category.LAUNCHER"/>\n'
            '            </intent-filter>\n'
            '        </activity>\n\n'
        )
        content = content.replace(match.group(0), launcher_block + main_block, 1)

    if 'android:roundIcon="@mipmap/ic_launcher_round"' not in content:
        content = content.replace(
            'android:icon="@mipmap/ic_launcher"',
            'android:icon="@mipmap/ic_launcher"\n        android:roundIcon="@mipmap/ic_launcher_round"',
            1,
        )
    manifest.write_text(content, encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: apply_upstream_ai_extension.py <upstream-winlator-root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1]).resolve()
    app = root / "app"
    java_dir = app / "app/src/main/java/com/winlator"
    res_dir = app / "app/src/main/res"
    source_root = Path(__file__).resolve().parents[1] / "integration"
    assistant_source = source_root / "AIAssistantActivity.java"
    launcher_source = source_root / "LauncherActivity.java"
    main_activity = java_dir / "MainActivity.java"
    menu_xml = res_dir / "menu/main_menu.xml"
    strings_xml = res_dir / "values/strings.xml"
    manifest_xml = app / "app/src/main/AndroidManifest.xml"
    build_gradle = app / "app/build.gradle"
    file_utils = java_dir / "core/FileUtils.java"
    proguard_rules = app / "app/proguard-rules.pro"
    cpp_root = app / "app/src/main/cpp"
    native_bridge_source = source_root / "NativeExecutionEngine.java"
    native_cmake_source = source_root / "native_engine/CMakeLists.txt"
    native_jni_source = source_root / "native_engine/jni_bridge.cpp"
    engine_root = Path(__file__).resolve().parents[1] / "engine"

    required = [
        assistant_source, launcher_source, native_bridge_source,
        native_cmake_source, native_jni_source, main_activity, menu_xml, strings_xml,
        manifest_xml, build_gradle, file_utils, proguard_rules,
        cpp_root / "CMakeLists.txt",
        engine_root / "include/aie/pe_image.hpp",
        engine_root / "include/aie/pe_loader.hpp",
        engine_root / "include/aie/x64_interpreter.hpp",
        engine_root / "include/aie/guest_memory.hpp",
        engine_root / "src/pe_image.cpp",
        engine_root / "src/pe_loader.cpp",
        engine_root / "src/x64_interpreter.cpp",
        engine_root / "src/guest_memory.cpp",
        source_root / "assets/ai_cover.webp",
        source_root / "assets/ic_launcher.xml",
        source_root / "assets/ic_launcher_round.xml",
        source_root / "assets/ai_brand_colors.xml",
    ]
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise FileNotFoundError("Missing upstream integration files: " + ", ".join(missing))

    java_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(assistant_source, java_dir / "AIAssistantActivity.java")
    shutil.copy2(launcher_source, java_dir / "LauncherActivity.java")
    shutil.copy2(native_bridge_source, java_dir / "NativeExecutionEngine.java")

    # Add the independent native core as a separate JNI shared library in the APK.
    native_target = cpp_root / "aie_engine"
    (native_target / "include/aie").mkdir(parents=True, exist_ok=True)
    (native_target / "src").mkdir(parents=True, exist_ok=True)
    for relative in [
        "include/aie/pe_image.hpp",
        "include/aie/pe_loader.hpp",
        "include/aie/x64_interpreter.hpp",
        "include/aie/guest_memory.hpp",
        "src/pe_image.cpp",
        "src/pe_loader.cpp",
        "src/x64_interpreter.cpp",
        "src/guest_memory.cpp",
    ]:
        shutil.copy2(engine_root / relative, native_target / relative)
    shutil.copy2(native_cmake_source, native_target / "CMakeLists.txt")
    shutil.copy2(native_jni_source, native_target / "jni_bridge.cpp")

    drawable_dir = res_dir / "drawable-nodpi"
    icon_dir = res_dir / "mipmap-anydpi-v26"
    values_dir = res_dir / "values"
    drawable_dir.mkdir(parents=True, exist_ok=True)
    icon_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source_root / "assets/ai_cover.webp", drawable_dir / "ai_cover.webp")
    shutil.copy2(source_root / "assets/ic_launcher.xml", icon_dir / "ic_launcher.xml")
    shutil.copy2(source_root / "assets/ic_launcher_round.xml", icon_dir / "ic_launcher_round.xml")
    shutil.copy2(source_root / "assets/ai_brand_colors.xml", values_dir / "ai_brand_colors.xml")

    # Keep the upstream engine and app internals, but isolate the installed AI Edition.
    replace_once(
        build_gradle,
        "applicationId 'com.winlator'",
        "applicationId 'com.winlator.aiedition'",
        "AI Edition application ID",
    )
    replace_once(
        manifest_xml,
        'android:authorities="com.winlator.FileProvider"',
        'android:authorities="com.winlator.aiedition.FileProvider"',
        "unique FileProvider authority",
    )
    replace_once(
        file_utils,
        '"com.winlator.FileProvider"',
        '"com.winlator.aiedition.FileProvider"',
        "FileProvider authority usage",
    )

    ensure_cmake_subdirectory(cpp_root / "CMakeLists.txt", "aie_engine")
    keep_rule = "-keep class com.winlator.NativeExecutionEngine { *; }"
    proguard_content = proguard_rules.read_text(encoding="utf-8")
    if keep_rule not in proguard_content:
        proguard_rules.write_text(
            proguard_content.rstrip() + "\n\n" + keep_rule + "\n",
            encoding="utf-8",
        )

    # Keep the assistant reachable from the retained runtime navigation drawer.
    replace_once(
        main_activity,
        "            case R.id.menu_item_settings:\n",
        "            case R.id.menu_item_ai_assistant:\n"
        "                startActivity(new Intent(this, AIAssistantActivity.class));\n"
        "                break;\n"
        "            case R.id.menu_item_settings:\n",
        "NavigationView AI menu handler",
    )
    replace_once(
        menu_xml,
        '        <item android:icon="@drawable/icon_settings" android:id="@+id/menu_item_settings"',
        '        <item android:icon="@drawable/icon_about" android:id="@+id/menu_item_ai_assistant" android:title="@string/ai_assistant" />\n'
        '        <item android:icon="@drawable/icon_settings" android:id="@+id/menu_item_settings"',
        "navigation menu item",
    )
    replace_once(
        strings_xml,
        '    <string name="settings">Settings</string>',
        '    <string name="ai_assistant">AI Assistant</string>\n'
        '    <string name="settings">Settings</string>',
        "AI Assistant string resource",
    )
    replace_once(
        strings_xml,
        '    <string name="app_name">Winlator</string>',
        '    <string name="app_name">Winlator AI Edition</string>',
        "application label",
    )

    manifest_content = manifest_xml.read_text(encoding="utf-8")
    assistant_declaration = (
        '        <activity android:name="com.winlator.AIAssistantActivity"\n'
        '            android:exported="false"\n'
        '            android:label="@string/ai_assistant"\n'
        '            android:theme="@style/AppThemeDark" />\n\n'
    )
    if 'android:name="com.winlator.AIAssistantActivity"' not in manifest_content:
        replace_once(
            manifest_xml,
            '        <activity android:name="com.winlator.XServerDisplayActivity"',
            assistant_declaration + '        <activity android:name="com.winlator.XServerDisplayActivity"',
            "assistant activity declaration",
        )

    install_launcher_manifest(manifest_xml)
    print("Applied AI Edition launcher, custom cover/icon and assistant extension.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

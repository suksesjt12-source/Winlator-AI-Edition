#!/usr/bin/env python3
"""Apply the Winlator AI Edition assistant entry to a checked-out upstream Winlator tree."""

from pathlib import Path
import shutil
import sys


def replace_once(path: Path, before: str, after: str, label: str) -> None:
    text = path.read_text(encoding="utf-8")
    if after.strip() in text:
        return
    if before not in text:
        raise RuntimeError(f"Could not locate insertion point for {label}: {path}")
    path.write_text(text.replace(before, after, 1), encoding="utf-8")


def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: apply_upstream_ai_extension.py <upstream-winlator-root>", file=sys.stderr)
        return 2

    root = Path(sys.argv[1]).resolve()
    app = root / "app"
    java_dir = app / "app/src/main/java/com/winlator"
    source = Path(__file__).resolve().parents[1] / "integration/AIAssistantActivity.java"
    main_activity = java_dir / "MainActivity.java"
    menu_xml = app / "app/src/main/res/menu/main_menu.xml"
    strings_xml = app / "app/src/main/res/values/strings.xml"
    manifest_xml = app / "app/src/main/AndroidManifest.xml"

    required = [source, main_activity, menu_xml, strings_xml, manifest_xml]
    missing = [str(path) for path in required if not path.is_file()]
    if missing:
        raise FileNotFoundError("Missing upstream integration files: " + ", ".join(missing))

    destination = java_dir / "AIAssistantActivity.java"
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)

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
    replace_once(
        manifest_xml,
        '        <activity android:name="com.winlator.XServerDisplayActivity"',
        '        <activity android:name="com.winlator.AIAssistantActivity"\n'
        '            android:exported="false"\n'
        '            android:label="@string/ai_assistant"\n'
        '            android:theme="@style/AppThemeDark" />\n\n'
        '        <activity android:name="com.winlator.XServerDisplayActivity"',
        "assistant activity declaration",
    )

    print("Applied Winlator AI Edition assistant extension.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

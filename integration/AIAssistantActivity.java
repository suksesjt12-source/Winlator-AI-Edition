package com.winlator;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class AIAssistantActivity extends Activity {
    private static final int REQUEST_INSPECT_EXECUTABLE = 4301;
    private static final long MAX_INSPECT_BYTES = 96L * 1024L * 1024L;

    private final int background = Color.rgb(14, 18, 28);
    private final int panel = Color.rgb(29, 37, 52);
    private final int accent = Color.rgb(130, 190, 255);
    private LinearLayout content;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ScrollView scrollView = new ScrollView(this);
        scrollView.setBackgroundColor(background);

        content = new LinearLayout(this);
        content.setOrientation(LinearLayout.VERTICAL);
        int padding = dp(20);
        content.setPadding(padding, padding, padding, padding);
        scrollView.addView(content);

        addText(content, "WINLATOR AI ASSISTANT", 23, Color.WHITE, true);
        addText(content, "Offline device advisor • No API key required", 13, Color.LTGRAY, false);

        ActivityManager manager = (ActivityManager)getSystemService(ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memory = new ActivityManager.MemoryInfo();
        if (manager != null) manager.getMemoryInfo(memory);

        long totalMb = memory.totalMem / (1024L * 1024L);
        long availableMb = memory.availMem / (1024L * 1024L);
        String soc = Build.VERSION.SDK_INT >= 31 ? Build.SOC_MODEL : "Unavailable on this Android API";
        String abi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "Unknown";

        addPanel(content, "DEVICE", Build.MANUFACTURER + " " + Build.MODEL);
        addPanel(content, "SOC", soc);
        addPanel(content, "ANDROID", "Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")");
        addPanel(content, "CPU ABI", abi);
        addPanel(content, "MEMORY", "Total RAM: " + totalMb + " MB\nAvailable now: " + availableMb + " MB");
        addPanel(content, "NATIVE ENGINE", NativeExecutionEngine.getStatus());

        Button inspect = new Button(this);
        inspect.setText("Inspect Windows .exe (native PE parser)");
        inspect.setAllCaps(false);
        inspect.setOnClickListener(v -> chooseExecutable());
        content.addView(inspect, new LinearLayout.LayoutParams(-1, -2));

        String profile;
        String resolution;
        if (availableMb < 1500L) {
            profile = "Cool & Stable";
            resolution = "Start with 640x360 or 800x450 and a 30 FPS cap. Close unnecessary background apps before testing.";
        } else if (availableMb < 2800L) {
            profile = "Balanced";
            resolution = "Start with 800x450 or 960x540 and a 30 FPS cap. Increase resolution only after checking frame pacing.";
        } else {
            profile = "Balanced";
            resolution = "Start with 960x540 and a 30 FPS cap. Test higher resolutions per game rather than assuming they will be faster.";
        }

        addPanel(content, "SUGGESTED PROFILE", profile);
        addPanel(content, "STARTING POINT", resolution);
        addPanel(content, "GPU COMPATIBILITY", "Select only graphics drivers supported by the device GPU. Turnip is an Adreno-oriented driver and is not a universal Mali driver. Validate Vortek or VirGL support on the target device.");
        addPanel(content, "IMPORTANT", "The native tool below only reads PE executable metadata. It does not launch or execute the selected file. This assistant gives local rule-based recommendations and does not change CPU clocks, GPU clocks or Android system settings. Actual game launching still depends on the installed Wine, Box64 and graphics runtime.");

        Button close = new Button(this);
        close.setText("Back to Launcher");
        close.setAllCaps(false);
        close.setOnClickListener(v -> finish());
        content.addView(close, new LinearLayout.LayoutParams(-1, -2));

        setContentView(scrollView);
    }

    private void chooseExecutable() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
        startActivityForResult(intent, REQUEST_INSPECT_EXECUTABLE);
    }

    @Override
    @SuppressWarnings("deprecation")
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQUEST_INSPECT_EXECUTABLE || resultCode != RESULT_OK || data == null) return;

        Uri uri = data.getData();
        if (uri == null) return;
        String fileName = displayName(uri);
        if (!fileName.toLowerCase(java.util.Locale.ROOT).endsWith(".exe")) {
            addPanel(content, "INSPECTION RESULT", "Selected file is not an .exe: " + fileName);
            return;
        }

        addPanel(content, "INSPECTION", "Reading " + fileName + " (maximum " + (MAX_INSPECT_BYTES / (1024L * 1024L)) + " MB)...");
        new Thread(() -> {
            String result = inspectExecutable(uri);
            runOnUiThread(() -> addPanel(content, "NATIVE PE INSPECTION — " + fileName, result));
        }, "aie-pe-inspection").start();
    }

    private String inspectExecutable(Uri uri) {
        try {
            byte[] bytes = readBounded(uri);
            return NativeExecutionEngine.inspectExecutable(bytes);
        } catch (Exception exception) {
            String detail = exception.getMessage();
            return "Inspection failed: " + (detail == null ? exception.getClass().getSimpleName() : detail);
        }
    }

    private byte[] readBounded(Uri uri) throws IOException {
        try (InputStream input = getContentResolver().openInputStream(uri);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            if (input == null) throw new IOException("Android could not open this document.");
            byte[] buffer = new byte[64 * 1024];
            long total = 0;
            int count;
            while ((count = input.read(buffer)) != -1) {
                total += count;
                if (total > MAX_INSPECT_BYTES) {
                    throw new IOException("File exceeds the 96 MB inspection limit.");
                }
                output.write(buffer, 0, count);
            }
            if (total == 0) throw new IOException("The selected file is empty.");
            return output.toByteArray();
        }
    }

    private String displayName(Uri uri) {
        try (Cursor cursor = getContentResolver().query(
                uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (index >= 0) {
                    String value = cursor.getString(index);
                    if (value != null && !value.isEmpty()) return value;
                }
            }
        } catch (Exception ignored) {
            // Fall back to the document URI when metadata is unavailable.
        }
        String segment = uri.getLastPathSegment();
        return segment == null ? "Unknown.exe" : segment;
    }

    private void addPanel(LinearLayout parent, String title, String value) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(14), dp(12), dp(14), dp(12));
        card.setBackgroundColor(panel);

        addText(card, title, 12, accent, true);
        addText(card, value, 14, Color.WHITE, false);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, -2);
        params.bottomMargin = dp(10);
        parent.addView(card, params);
    }

    private void addText(LinearLayout parent, String value, int size, int color, boolean bold) {
        TextView textView = new TextView(this);
        textView.setText(value);
        textView.setTextSize(size);
        textView.setTextColor(color);
        if (bold) textView.setTypeface(textView.getTypeface(), android.graphics.Typeface.BOLD);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, -2);
        params.bottomMargin = dp(8);
        parent.addView(textView, params);
    }

    private int dp(int value) {
        return (int)(value * getResources().getDisplayMetrics().density + 0.5f);
    }
}

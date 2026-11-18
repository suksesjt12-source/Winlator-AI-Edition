package com.winlator;

import android.app.Activity;
import android.app.ActivityManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.util.Locale;

public class AIAssistantActivity extends Activity {
    private final int background = Color.rgb(14, 18, 28);
    private final int panel = Color.rgb(29, 37, 52);
    private final int accent = Color.rgb(130, 190, 255);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ScrollView scrollView = new ScrollView(this);
        scrollView.setBackgroundColor(background);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        int padding = dp(20);
        layout.setPadding(padding, padding, padding, padding);
        scrollView.addView(layout);

        addText(layout, "WINLATOR AI ASSISTANT", 23, Color.WHITE, true);
        addText(layout, "Offline device advisor • No API key required", 13, Color.LTGRAY, false);

        ActivityManager manager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memory = new ActivityManager.MemoryInfo();
        manager.getMemoryInfo(memory);

        long totalMb = memory.totalMem / (1024L * 1024L);
        long availableMb = memory.availMem / (1024L * 1024L);
        String soc = Build.VERSION.SDK_INT >= 31 ? Build.SOC_MODEL : "Unavailable on this Android API";
        String abi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "Unknown";

        addPanel(layout, "DEVICE", Build.MANUFACTURER + " " + Build.MODEL);
        addPanel(layout, "SOC", soc);
        addPanel(layout, "ANDROID", "Android " + Build.VERSION.RELEASE + " (API " + Build.VERSION.SDK_INT + ")");
        addPanel(layout, "CPU ABI", abi);
        addPanel(layout, "MEMORY", "Total RAM: " + totalMb + " MB\nAvailable now: " + availableMb + " MB");

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

        addPanel(layout, "SUGGESTED PROFILE", profile);
        addPanel(layout, "STARTING POINT", resolution);
        addPanel(layout, "GPU COMPATIBILITY", "Select only graphics drivers supported by the device GPU. Turnip is an Adreno-oriented driver and is not a universal Mali driver. Validate Vortek or VirGL support on the target device.");
        addPanel(layout, "IMPORTANT", "This assistant gives local rule-based recommendations. It does not change CPU clocks, GPU clocks, Android system settings or guarantee higher FPS. Actual game launching still depends on the installed Wine, Box64 and graphics runtime.");

        Button close = new Button(this);
        close.setText("Back to Winlator");
        close.setAllCaps(false);
        close.setOnClickListener(v -> finish());
        layout.addView(close, new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        ));

        setContentView(scrollView);
    }

    private void addPanel(LinearLayout parent, String title, String value) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(14), dp(12), dp(14), dp(12));
        card.setBackgroundColor(panel);

        addText(card, title, 12, accent, true);
        addText(card, value, 14, Color.WHITE, false);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        params.bottomMargin = dp(10);
        parent.addView(card, params);
    }

    private void addText(LinearLayout parent, String value, int size, int color, boolean bold) {
        TextView textView = new TextView(this);
        textView.setText(value);
        textView.setTextSize(size);
        textView.setTextColor(color);
        if (bold) textView.setTypeface(textView.getTypeface(), android.graphics.Typeface.BOLD);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT,
            LinearLayout.LayoutParams.WRAP_CONTENT
        );
        params.bottomMargin = dp(8);
        parent.addView(textView, params);
    }

    private int dp(int value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }
}

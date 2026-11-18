package com.winlator;

import android.app.Activity;
import android.app.ActivityManager;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

public class LauncherActivity extends Activity {
    private static final int BG = Color.rgb(7, 11, 22);
    private static final int PANEL = Color.rgb(17, 25, 43);
    private static final int BORDER = Color.rgb(37, 55, 83);
    private static final int WHITE = Color.rgb(242, 247, 255);
    private static final int MUTED = Color.rgb(151, 166, 190);
    private static final int CYAN = Color.rgb(66, 211, 255);
    private static final int PURPLE = Color.rgb(171, 111, 255);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Window window = getWindow();
        window.setStatusBarColor(BG);
        window.setNavigationBarColor(BG);

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setBackgroundColor(BG);

        LinearLayout page = new LinearLayout(this);
        page.setOrientation(LinearLayout.VERTICAL);
        page.setPadding(dp(18), dp(18), dp(18), dp(26));
        scroll.addView(page);

        addHeader(page);
        addHero(page);
        addDeviceStatus(page);
        addText(page, "WORKSPACE", 12, CYAN, true, dp(22), dp(10));

        LinearLayout rowOne = new LinearLayout(this);
        rowOne.setOrientation(LinearLayout.HORIZONTAL);
        rowOne.addView(actionCard("▤", "GAME LIBRARY", "Your game shortcuts", () ->
            openRuntimeSection(R.id.menu_item_shortcuts)),
            new LinearLayout.LayoutParams(0, dp(138), 1f));
        rowOne.addView(actionCard("▣", "CONTAINERS", "Windows environments", () ->
            openRuntimeSection(R.id.menu_item_containers)),
            actionParams());
        page.addView(rowOne);

        LinearLayout rowTwo = new LinearLayout(this);
        rowTwo.setOrientation(LinearLayout.HORIZONTAL);
        rowTwo.addView(actionCard("✦", "AI ASSISTANT", "Device-aware guidance", () ->
            startActivity(new Intent(this, AIAssistantActivity.class))),
            new LinearLayout.LayoutParams(0, dp(138), 1f));
        rowTwo.addView(actionCard("⚙", "COMPONENTS & SETTINGS", "Wine, Box64 and graphics", () ->
            openRuntimeSection(R.id.menu_item_settings)),
            actionParams());
        LinearLayout.LayoutParams rowTwoParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        rowTwoParams.topMargin = dp(10);
        page.addView(rowTwo, rowTwoParams);

        addRuntimeNote(page);
        setContentView(scroll);
    }

    private void addHeader(LinearLayout page) {
        LinearLayout header = new LinearLayout(this);
        header.setOrientation(LinearLayout.HORIZONTAL);
        header.setGravity(Gravity.CENTER_VERTICAL);

        LinearLayout brand = new LinearLayout(this);
        brand.setOrientation(LinearLayout.VERTICAL);
        TextView name = label("WINLATOR AI EDITION", 16, WHITE, true);
        TextView sub = label("PC GAME LAUNCHER", 10, MUTED, true);
        brand.addView(name);
        brand.addView(sub);
        header.addView(brand, new LinearLayout.LayoutParams(0, -2, 1f));

        TextView badge = label("ARM64  •  ANDROID", 10, CYAN, true);
        badge.setPadding(dp(11), dp(8), dp(11), dp(8));
        badge.setBackground(pill(BORDER));
        header.addView(badge);
        page.addView(header);
        addSpace(page, 16);
    }

    private void addHero(LinearLayout page) {
        FrameLayout hero = new FrameLayout(this);
        GradientDrawable frame = new GradientDrawable();
        frame.setColor(PANEL);
        frame.setCornerRadius(dp(22));
        frame.setStroke(dp(1), BORDER);
        hero.setBackground(frame);
        hero.setClipToOutline(true);

        ImageView artwork = new ImageView(this);
        artwork.setImageResource(R.drawable.ai_cover);
        artwork.setScaleType(ImageView.ScaleType.CENTER_CROP);
        artwork.setContentDescription("Winlator AI Edition blue and violet technology emblem");
        hero.addView(artwork, new FrameLayout.LayoutParams(-1, -1));

        View shade = new View(this);
        shade.setBackground(new GradientDrawable(
            GradientDrawable.Orientation.BOTTOM_TOP,
            new int[]{0xF5070B16, 0x94070B16, 0x18070B16}));
        hero.addView(shade, new FrameLayout.LayoutParams(-1, -1));

        LinearLayout copy = new LinearLayout(this);
        copy.setOrientation(LinearLayout.VERTICAL);
        copy.setPadding(dp(18), dp(18), dp(18), dp(18));
        copy.setGravity(Gravity.BOTTOM);

        TextView tag = label("AI-ASSISTED WINDOWS GAMING", 10, CYAN, true);
        copy.addView(tag);
        TextView title = label("Your PC game hub\non Android.", 26, WHITE, true);
        title.setLineSpacing(dp(1), 1f);
        LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(-1, -2);
        titleParams.topMargin = dp(7);
        copy.addView(title, titleParams);
        TextView caption = label("One clean home for your library, containers and runtime tools.", 12, MUTED, false);
        LinearLayout.LayoutParams captionParams = new LinearLayout.LayoutParams(-1, -2);
        captionParams.topMargin = dp(6);
        copy.addView(caption, captionParams);

        FrameLayout.LayoutParams copyParams = new FrameLayout.LayoutParams(-1, -1, Gravity.BOTTOM);
        hero.addView(copy, copyParams);

        LinearLayout.LayoutParams heroParams = new LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.MATCH_PARENT, dp(236));
        page.addView(hero, heroParams);
    }

    private void addDeviceStatus(LinearLayout page) {
        ActivityManager manager = (ActivityManager)getSystemService(ACTIVITY_SERVICE);
        ActivityManager.MemoryInfo memory = new ActivityManager.MemoryInfo();
        if (manager != null) manager.getMemoryInfo(memory);

        String abi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "Unknown ABI";
        String device = Build.MANUFACTURER + " " + Build.MODEL;
        String android = "Android " + Build.VERSION.RELEASE + "  •  " + abi;
        long freeMb = memory.availMem / (1024L * 1024L);

        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(14), dp(12), dp(14), dp(12));
        card.setBackground(panelBackground());

        addText(card, "DEVICE PROFILE", 10, CYAN, true, 0, 0);
        addText(card, device, 14, WHITE, true, dp(4), 0);
        addText(card, android + "  •  " + freeMb + " MB available RAM", 11, MUTED, false, dp(4), 0);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, -2);
        params.topMargin = dp(12);
        page.addView(card, params);
    }

    private View actionCard(String symbol, String title, String description, Runnable action) {
        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(14), dp(13), dp(12), dp(12));
        card.setBackground(panelBackground());
        card.setClickable(true);
        card.setFocusable(true);
        card.setElevation(dp(2));
        card.setOnClickListener(v -> action.run());

        TextView icon = label(symbol, 23, title.startsWith("AI") ? PURPLE : CYAN, true);
        card.addView(icon);
        TextView heading = label(title, 12, WHITE, true);
        LinearLayout.LayoutParams headingParams = new LinearLayout.LayoutParams(-1, -2);
        headingParams.topMargin = dp(7);
        card.addView(heading, headingParams);
        TextView detail = label(description, 10, MUTED, false);
        LinearLayout.LayoutParams detailParams = new LinearLayout.LayoutParams(-1, -2);
        detailParams.topMargin = dp(4);
        card.addView(detail, detailParams);
        return card;
    }

    private LinearLayout.LayoutParams actionParams() {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, dp(138), 1f);
        params.leftMargin = dp(10);
        return params;
    }

    private void addRuntimeNote(LinearLayout page) {
        LinearLayout note = new LinearLayout(this);
        note.setOrientation(LinearLayout.VERTICAL);
        note.setPadding(dp(14), dp(13), dp(14), dp(13));
        note.setBackground(panelBackground());
        addText(note, "WINDOWS RUNTIME", 10, PURPLE, true, 0, 0);
        addText(note, "Powered by the Winlator compatibility layer", 13, WHITE, true, dp(5), 0);
        addText(note, "Wine, Box64, graphics drivers and translation components remain configurable inside the runtime. Game compatibility depends on the device and title.", 11, MUTED, false, dp(5), 0);

        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, -2);
        params.topMargin = dp(18);
        page.addView(note, params);

        TextView footer = label("WINLATOR AI EDITION  •  BUILT FOR ANDROID", 9, MUTED, true);
        footer.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams footerParams = new LinearLayout.LayoutParams(-1, -2);
        footerParams.topMargin = dp(18);
        page.addView(footer, footerParams);
    }

    private void openRuntimeSection(int menuItemId) {
        Intent intent = new Intent(this, MainActivity.class);
        intent.putExtra("selected_menu_item_id", menuItemId);
        startActivity(intent);
    }

    private GradientDrawable panelBackground() {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(PANEL);
        drawable.setCornerRadius(dp(17));
        drawable.setStroke(dp(1), BORDER);
        return drawable;
    }

    private GradientDrawable pill(int color) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(color);
        drawable.setCornerRadius(dp(30));
        return drawable;
    }

    private TextView label(String value, int size, int color, boolean bold) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(size);
        view.setTextColor(color);
        view.setGravity(Gravity.CENTER_VERTICAL);
        view.setTypeface(Typeface.DEFAULT, bold ? Typeface.BOLD : Typeface.NORMAL);
        view.setIncludeFontPadding(true);
        return view;
    }

    private void addText(LinearLayout parent, String value, int size, int color,
                         boolean bold, int topMargin, int bottomMargin) {
        TextView view = label(value, size, color, bold);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, -2);
        params.topMargin = topMargin;
        params.bottomMargin = bottomMargin;
        parent.addView(view, params);
    }

    private void addSpace(LinearLayout parent, int dp) {
        View spacer = new View(this);
        parent.addView(spacer, new LinearLayout.LayoutParams(1, dp(dp)));
    }

    private int dp(int value) {
        return (int)(value * getResources().getDisplayMetrics().density + 0.5f);
    }
}

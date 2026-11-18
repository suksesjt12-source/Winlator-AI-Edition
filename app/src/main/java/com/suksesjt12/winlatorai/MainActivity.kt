package com.suksesjt12.winlatorai

import android.app.Activity
import android.app.ActivityManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

class MainActivity : Activity() {
    private lateinit var content: LinearLayout
    private var selectedProfile = "Balanced"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        buildScreen()
        showHome()
    }

    private fun buildScreen() {
        val scroll = ScrollView(this).apply {
            setBackgroundColor(Color.rgb(14, 18, 28))
            isFillViewport = true
        }
        content = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(20), dp(24), dp(20), dp(28))
        }
        scroll.addView(content)
        setContentView(scroll)
    }

    private fun showHome() {
        content.removeAllViews()
        addText("WINLATOR AI EDITION", 24, Color.WHITE, true)
        addText("Android ARM64 • AI-assisted configuration", 13, Color.LTGRAY)
        addGap(20)
        addPanel("STATUS", "Prototype launcher aktif. Komponen runtime Windows belum disertakan pada tahap ini.")
        addButton("Deteksi perangkat") { showDeviceInfo() }
        addButton("AI Assistant") { showAssistant() }
        addButton("Optimasi otomatis") { showOptimization() }
        addButton("Profil game") { showGameProfiles() }
        addGap(14)
        addText("Rekomendasi hanya mengubah profil di aplikasi ini. Tidak melakukan root, overclock, atau mengubah pengaturan sistem.", 12, Color.LTGRAY)
    }

    private fun showDeviceInfo() {
        val am = getSystemService(ACTIVITY_SERVICE) as ActivityManager
        val memory = ActivityManager.MemoryInfo()
        am.getMemoryInfo(memory)
        val totalRam = memory.totalMem / (1024L * 1024L)
        val freeRam = memory.availMem / (1024L * 1024L)
        content.removeAllViews()
        addText("INFO PERANGKAT", 22, Color.WHITE, true)
        addPanel("MODEL", "${Build.MANUFACTURER} ${Build.MODEL}")
        addPanel("ANDROID", "Android ${Build.VERSION.RELEASE} (API ${Build.VERSION.SDK_INT})")
        addPanel("ABI", Build.SUPPORTED_ABIS.joinToString())
        addPanel("MEMORI", "Total sistem: ${totalRam} MB\nTersedia saat ini: ${freeRam} MB")
        addPanel("DEVICE", Build.DEVICE)
        addButton("Kembali") { showHome() }
    }

    private fun showAssistant() {
        content.removeAllViews()
        addText("AI ASSISTANT", 22, Color.WHITE, true)
        addPanel("CARA KERJA", "Asisten lokal berbasis aturan. Belum terhubung ke layanan AI cloud dan tidak memerlukan API key.")
        addPanel("Mengapa game Windows tidak langsung berjalan?", "Android tidak menjalankan file .exe secara native. Dibutuhkan runtime kompatibilitas seperti Wine dan lapisan CPU seperti Box64, ditambah konfigurasi grafis yang sesuai.")
        addPanel("Apakah AI bisa menaikkan FPS?", "AI dapat membantu memilih konfigurasi yang masuk akal, tetapi tidak menambah kemampuan fisik CPU atau GPU. Hasil harus diuji per game dan dipantau suhunya.")
        addPanel("SARAN AWAL", "Mulai dari resolusi rendah-menengah, gunakan target FPS realistis, dan beri jeda jika perangkat terasa terlalu panas.")
        addButton("Kembali") { showHome() }
    }

    private fun showOptimization() {
        val am = getSystemService(ACTIVITY_SERVICE) as ActivityManager
        val memory = ActivityManager.MemoryInfo()
        am.getMemoryInfo(memory)
        val freeMb = memory.availMem / (1024L * 1024L)
        val recommendation = when {
            freeMb < 1800 -> "Cool & Stable: tutup aplikasi latar belakang yang tidak diperlukan. Mulai pada 640x360 atau 800x450 dan target 30 FPS."
            freeMb < 3000 -> "Balanced: mulai pada 800x450 atau 960x540, target 30 FPS. Naikkan kualitas hanya jika frame pacing stabil."
            else -> "Balanced: mulai pada 960x540, target 30 FPS. Uji 720p hanya jika game dan runtime mampu mempertahankan frame pacing."
        }
        content.removeAllViews()
        addText("OPTIMASI OTOMATIS", 22, Color.WHITE, true)
        addPanel("PROFIL TERPILIH", selectedProfile)
        addPanel("MEMORI TERSEDIA", "$freeMb MB")
        addPanel("REKOMENDASI", recommendation)
        addPanel("BATASAN", "Aplikasi belum dapat membaca suhu baterai/GPU secara konsisten di semua perangkat dan tidak mengubah clock, driver, atau pengaturan sistem.")
        addButton("Gunakan Balanced") { selectedProfile = "Balanced"; showOptimization() }
        addButton("Gunakan Performance") { selectedProfile = "Performance"; showOptimization() }
        addButton("Gunakan Cool & Stable") { selectedProfile = "Cool & Stable"; showOptimization() }
        addButton("Kembali") { showHome() }
    }

    private fun showGameProfiles() {
        content.removeAllViews()
        addText("PROFIL GAME", 22, Color.WHITE, true)
        addPanel("BELUM ADA GAME", "Tahap berikutnya akan menambahkan pustaka game, profil per game, dan pemilihan file .exe. Memilih .exe saja belum dapat menjalankannya tanpa runtime Windows.")
        addButton("Kembali") { showHome() }
    }

    private fun addText(value: String, size: Int, color: Int, bold: Boolean = false) {
        val view = TextView(this).apply {
            text = value
            textSize = size.toFloat()
            setTextColor(color)
            if (bold) setTypeface(typeface, android.graphics.Typeface.BOLD)
        }
        content.addView(view, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(8) })
    }

    private fun addPanel(title: String, body: String) {
        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(14), dp(12), dp(14), dp(12))
            setBackgroundColor(Color.rgb(29, 37, 52))
        }
        val heading = TextView(this).apply {
            text = title
            textSize = 12f
            setTextColor(Color.rgb(130, 190, 255))
            setTypeface(typeface, android.graphics.Typeface.BOLD)
        }
        val detail = TextView(this).apply {
            text = body
            textSize = 14f
            setTextColor(Color.WHITE)
            setPadding(0, dp(6), 0, 0)
        }
        panel.addView(heading)
        panel.addView(detail)
        content.addView(panel, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(10) })
    }

    private fun addButton(label: String, action: () -> Unit) {
        val button = Button(this).apply {
            text = label
            isAllCaps = false
            setOnClickListener { action() }
        }
        content.addView(button, LinearLayout.LayoutParams(-1, -2).apply { bottomMargin = dp(8) })
    }

    private fun addGap(value: Int) {
        content.addView(TextView(this), LinearLayout.LayoutParams(1, dp(value)))
    }

    private fun dp(value: Int): Int = (value * resources.displayMetrics.density).toInt()
}

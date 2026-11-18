package com.suksesjt12.winlatorai

import android.app.Activity
import android.app.ActivityManager
import android.content.Intent
import android.graphics.Color
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.OpenableColumns
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView

class MainActivity : Activity() {
    private lateinit var content: LinearLayout
    private var selectedProfile = "Balanced"

    private val preferences by lazy { getSharedPreferences("winlator_ai", MODE_PRIVATE) }

    companion object {
        private const val REQUEST_PICK_EXE = 2101
        private const val KEY_GAME_URIS = "game_uris"
    }

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
        addPanel("STATUS", "Prototype launcher aktif. Runtime Windows asli belum terintegrasi pada tahap ini.")
        addButton("Deteksi perangkat") { showDeviceInfo() }
        addButton("AI Assistant") { showAssistant() }
        addButton("Optimasi otomatis") { showOptimization() }
        addButton("Pustaka game (.exe)") { showGameProfiles() }
        addGap(14)
        addText("Profil dan daftar file disimpan di aplikasi. File .exe belum bisa dijalankan sebelum runtime Winlator diintegrasikan.", 12, Color.LTGRAY)
    }

    private fun showDeviceInfo() {
        val am = getSystemService(ACTIVITY_SERVICE) as ActivityManager
        val memory = ActivityManager.MemoryInfo()
        am.getMemoryInfo(memory)
        val totalRam = memory.totalMem / (1024L * 1024L)
        val freeRam = memory.availMem / (1024L * 1024L)
        val soc = if (Build.VERSION.SDK_INT >= 31) Build.SOC_MODEL else "Tidak tersedia melalui API Android ini"
        content.removeAllViews()
        addText("INFO PERANGKAT", 22, Color.WHITE, true)
        addPanel("MODEL", "${Build.MANUFACTURER} ${Build.MODEL}")
        addPanel("SOC", soc)
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
        addPanel("Saran untuk Helio G100 / Mali", "Jangan memilih driver Turnip khusus Adreno untuk GPU Mali. Pilihan renderer harus mengikuti dukungan runtime dan driver yang benar-benar tersedia.")
        addPanel("SARAN AWAL", "Mulai dari resolusi rendah-menengah, gunakan target FPS realistis, dan beri jeda jika perangkat terasa terlalu panas.")
        addButton("Kembali") { showHome() }
    }

    private fun showOptimization() {
        val am = getSystemService(ACTIVITY_SERVICE) as ActivityManager
        val memory = ActivityManager.MemoryInfo()
        am.getMemoryInfo(memory)
        val freeMb = memory.availMem / (1024L * 1024L)
        val recommendation = when (selectedProfile) {
            "Performance" -> if (freeMb < 1800) {
                "Memori tersedia rendah. Tutup aplikasi latar yang tidak diperlukan terlebih dahulu. Mulai 640x360 atau 800x450 dengan target 30 FPS."
            } else {
                "Mulai 800x450 atau 960x540. Gunakan target 30 FPS dan uji kestabilan frame pacing sebelum menaikkan resolusi."
            }
            "Cool & Stable" ->
                "Utamakan kestabilan: 640x360 atau 800x450, target 30 FPS, batasi sesi pengujian dan pantau suhu perangkat."
            else -> if (freeMb < 1800) {
                "Memori tersedia rendah. Mulai 640x360 atau 800x450, target 30 FPS, dan kurangi aplikasi latar."
            } else {
                "Mulai 960x540 dengan target 30 FPS. Uji 720p hanya jika game dan runtime mampu mempertahankan frame pacing."
            }
        }
        content.removeAllViews()
        addText("OPTIMASI OTOMATIS", 22, Color.WHITE, true)
        addPanel("PROFIL TERPILIH", selectedProfile)
        addPanel("MEMORI TERSEDIA", "$freeMb MB")
        addPanel("REKOMENDASI", recommendation)
        addPanel("BATASAN", "Rekomendasi ini hanya saran awal berdasarkan memori dan profil yang dipilih. Aplikasi belum membaca suhu GPU/CPU secara konsisten dan tidak mengubah clock, driver, atau pengaturan sistem.")
        addButton("Gunakan Balanced") { selectedProfile = "Balanced"; showOptimization() }
        addButton("Gunakan Performance") { selectedProfile = "Performance"; showOptimization() }
        addButton("Gunakan Cool & Stable") { selectedProfile = "Cool & Stable"; showOptimization() }
        addButton("Kembali") { showHome() }
    }

    private fun chooseExe() {
        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "*/*"
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
        }
        startActivityForResult(intent, REQUEST_PICK_EXE)
    }

    @Deprecated("Deprecated by Android; retained for compatibility with the current minimal Activity implementation.")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode != REQUEST_PICK_EXE || resultCode != RESULT_OK) return
        val uri = data?.data ?: return
        val name = displayName(uri)
        if (!name.lowercase().endsWith(".exe")) {
            showGameProfiles("File yang dipilih bukan file .exe.")
            return
        }

        try {
            val flags = data.flags and (Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            contentResolver.takePersistableUriPermission(uri, flags)
        } catch (_: SecurityException) {
            // Some document providers do not offer persistable grants.
        }

        val gameUris = preferences.getStringSet(KEY_GAME_URIS, emptySet())?.toMutableSet() ?: mutableSetOf()
        gameUris.add(uri.toString())
        preferences.edit().putStringSet(KEY_GAME_URIS, gameUris).apply()
        showGameProfiles("${name} added to the library.")
    }

    private fun showGameProfiles(message: String? = null) {
        content.removeAllViews()
        addText("PUSTAKA GAME", 22, Color.WHITE, true)
        addText("Pilih file .exe dari penyimpanan. Izin baca file disimpan melalui Android Storage Access Framework.", 13, Color.LTGRAY)
        if (message != null) addPanel("INFO", message)
        addButton("Pilih file .exe") { chooseExe() }

        val gameUris = preferences.getStringSet(KEY_GAME_URIS, emptySet())?.toList().orEmpty()
        if (gameUris.isEmpty()) {
            addPanel("PUSTAKA KOSONG", "Belum ada file .exe yang ditambahkan.")
        } else {
            gameUris.forEach { uriText ->
                val uri = Uri.parse(uriText)
                val name = displayName(uri)
                val profileKey = profileKey(uriText)
                val gameProfile = preferences.getString(profileKey, "Balanced") ?: "Balanced"
                addPanel(name, "Profil tersimpan: $gameProfile\nStatus: runtime belum terintegrasi; file belum dapat dijalankan dari aplikasi ini.")
                addButton("Atur profil: $name") { showGameProfileOptions(uriText, name) }
                addButton("Hapus dari pustaka: $name") {
                    val updated = preferences.getStringSet(KEY_GAME_URIS, emptySet())?.toMutableSet() ?: mutableSetOf()
                    updated.remove(uriText)
                    preferences.edit().putStringSet(KEY_GAME_URIS, updated).remove(profileKey).apply()
                    showGameProfiles("File dihapus dari daftar aplikasi. File asli tidak dihapus.")
                }
            }
        }
        addButton("Kembali") { showHome() }
    }

    private fun showGameProfileOptions(uriText: String, name: String) {
        content.removeAllViews()
        addText("PROFIL GAME", 22, Color.WHITE, true)
        addPanel(name, "Pilih profil konfigurasi yang akan disimpan untuk game ini.")
        listOf("Balanced", "Performance", "Cool & Stable").forEach { profile ->
            addButton("Gunakan $profile") {
                preferences.edit().putString(profileKey(uriText), profile).apply()
                showGameProfiles("Profil $profile disimpan untuk $name.")
            }
        }
        addButton("Kembali") { showGameProfiles() }
    }

    private fun profileKey(uriText: String): String = "profile_${uriText.hashCode()}"

    private fun displayName(uri: Uri): String {
        var result: String? = null
        try {
            contentResolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)?.use { cursor ->
                if (cursor.moveToFirst()) {
                    val index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    if (index >= 0) result = cursor.getString(index)
                }
            }
        } catch (_: Exception) {
            // Fall back to the URI's final segment.
        }
        return result ?: uri.lastPathSegment?.substringAfterLast('/') ?: "Unknown.exe"
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

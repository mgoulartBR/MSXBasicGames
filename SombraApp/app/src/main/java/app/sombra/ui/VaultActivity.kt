package app.sombra.ui

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.os.SystemClock
import android.provider.Settings
import android.view.WindowManager
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.lifecycle.lifecycleScope
import androidx.core.view.WindowCompat
import app.sombra.core.Freezer
import app.sombra.core.Session
import app.sombra.core.ShizukuState
import app.sombra.data.AppInfo
import app.sombra.data.AppRepo
import app.sombra.data.HiddenApp
import app.sombra.data.HideMode
import app.sombra.data.Store
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

interface VaultActions {
    fun close()
    fun open(app: HiddenApp)
    /** mode == null remove o app do cofre. Retorna mensagem de erro, ou null se deu certo. */
    suspend fun apply(app: AppInfo, mode: HideMode?): String?
    fun openAccessibilitySettings()
    fun requestShizuku()
    fun openShizuku()
    fun openHomeSettings()
    fun openAppDetails()
}

class VaultActivity : ComponentActivity(), VaultActions {
    private var stoppedAt = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (Store.blockScreenshots) window.setFlags(WindowManager.LayoutParams.FLAG_SECURE, WindowManager.LayoutParams.FLAG_SECURE)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        lifecycleScope.launch { Session.vaultOpen.collect { if (!it) finish() } }
        setContent { SombraTheme { VaultScreen(this) } }
    }

    override fun onStop() {
        super.onStop()
        if (!isChangingConfigurations) stoppedAt = SystemClock.elapsedRealtime()
    }

    override fun onStart() {
        super.onStart()
        if (stoppedAt > 0 && SystemClock.elapsedRealtime() - stoppedAt > Store.autoLockSec * 1000L) {
            Session.lockAll()
        }
        stoppedAt = 0
    }

    override fun close() { Session.lockAll() }

    override fun open(app: HiddenApp) {
        lifecycleScope.launch {
            if (app.mode == HideMode.FREEZE) {
                val ok = withContext(Dispatchers.IO) { Freezer.setFrozen(this@VaultActivity, app.pkg, false) }
                if (!ok) { toast("Não foi possível descongelar. O Shizuku está ativo?"); return@launch }
            }
            if (app.mode == HideMode.LOCK) Session.allow(app.pkg)
            if (!AppRepo.launch(this@VaultActivity, app.pkg)) toast("Não foi possível abrir ${app.label}")
        }
    }

    override suspend fun apply(app: AppInfo, mode: HideMode?): String? = withContext(Dispatchers.IO) {
        val previous = Store.modeOf(app.pkg)
        if (previous == HideMode.FREEZE && mode != HideMode.FREEZE) {
            if (!Freezer.setFrozen(this@VaultActivity, app.pkg, false)) return@withContext "Não foi possível descongelar o app. Verifique o Shizuku."
        }
        when (mode) {
            null -> Store.remove(app.pkg)
            HideMode.FREEZE -> {
                if (Freezer.state(this@VaultActivity) != ShizukuState.READY) return@withContext "Shizuku não está pronto. Veja Ajustes › Métodos de proteção."
                if (!Freezer.setFrozen(this@VaultActivity, app.pkg, true)) return@withContext "Falha ao congelar o app."
                Store.put(HiddenApp(app.pkg, app.label, mode))
            }
            else -> Store.put(HiddenApp(app.pkg, app.label, mode))
        }
        null
    }

    override fun openAccessibilitySettings() = startActivity(Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS))
    override fun requestShizuku() = Freezer.requestPermission()
    override fun openShizuku() {
        val i = packageManager.getLaunchIntentForPackage(Freezer.SHIZUKU_PKG)
            ?: Intent(Intent.ACTION_VIEW, Uri.parse("https://shizuku.rikka.app/download/"))
        startActivity(i)
    }
    override fun openHomeSettings() = startActivity(Intent(Settings.ACTION_HOME_SETTINGS))
    override fun openAppDetails() = startActivity(
        Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.parse("package:$packageName"))
    )

    private fun toast(s: String) = Toast.makeText(this, s, Toast.LENGTH_LONG).show()
}

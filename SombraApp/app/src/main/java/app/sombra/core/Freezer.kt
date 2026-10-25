package app.sombra.core

import android.content.Context
import android.content.pm.PackageManager
import rikka.shizuku.Shizuku

enum class ShizukuState { NOT_INSTALLED, NOT_RUNNING, NO_PERMISSION, READY }

/**
 * "Congelar" = `pm disable-user` executado com privilégios de shell via Shizuku.
 * O app desaparece da gaveta, de qualquer lançador, dos atalhos e da lista de apps das Configurações,
 * e não consegue ser aberto até ser reativado.
 */
object Freezer {
    const val SHIZUKU_PKG = "moe.shizuku.privileged.api"
    private val PKG_RE = Regex("^[A-Za-z0-9._]+$")

    fun state(ctx: Context): ShizukuState {
        val installed = try {
            ctx.packageManager.getPackageInfo(SHIZUKU_PKG, 0); true
        } catch (_: Exception) { false }
        val running = try { Shizuku.pingBinder() } catch (_: Throwable) { false }
        return when {
            !running && !installed -> ShizukuState.NOT_INSTALLED
            !running -> ShizukuState.NOT_RUNNING
            !granted() -> ShizukuState.NO_PERMISSION
            else -> ShizukuState.READY
        }
    }

    private fun granted(): Boolean = try {
        !Shizuku.isPreV11() && Shizuku.checkSelfPermission() == PackageManager.PERMISSION_GRANTED
    } catch (_: Throwable) { false }

    fun requestPermission() {
        try { Shizuku.requestPermission(4242) } catch (_: Throwable) { }
    }

    fun isFrozen(ctx: Context, pkg: String): Boolean = try {
        val s = ctx.packageManager.getApplicationEnabledSetting(pkg)
        s == PackageManager.COMPONENT_ENABLED_STATE_DISABLED_USER ||
            s == PackageManager.COMPONENT_ENABLED_STATE_DISABLED
    } catch (_: Exception) { false }

    /** Bloqueante: chame fora da thread principal. */
    fun setFrozen(ctx: Context, pkg: String, frozen: Boolean): Boolean {
        if (!PKG_RE.matches(pkg)) return false
        if (isFrozen(ctx, pkg) == frozen) return true
        val cmd = if (frozen) "pm disable-user --user 0 $pkg" else "pm enable $pkg"
        return try {
            exec(cmd)
            isFrozen(ctx, pkg) == frozen
        } catch (_: Throwable) { false }
    }

    private fun exec(cmd: String): String {
        // Em Shizuku 13+ newProcess é privado; a reflexão é o caminho documentado pela comunidade.
        val m = Shizuku::class.java.getDeclaredMethod(
            "newProcess", Array<String>::class.java, Array<String>::class.java, String::class.java
        )
        m.isAccessible = true
        val p = m.invoke(null, arrayOf("sh", "-c", cmd), null, null) as Process
        val out = p.inputStream.bufferedReader().readText() + p.errorStream.bufferedReader().readText()
        p.waitFor()
        return out
    }
}

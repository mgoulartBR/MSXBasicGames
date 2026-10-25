package app.sombra.core

import android.os.SystemClock
import app.sombra.data.Store
import kotlinx.coroutines.flow.MutableStateFlow
import java.util.concurrent.ConcurrentHashMap

/** Estado volátil (só em memória): cofre aberto e apps bloqueados já liberados. */
object Session {
    /** Verdadeiro enquanto o cofre estiver desbloqueado; a VaultActivity fecha quando vira falso. */
    val vaultOpen = MutableStateFlow(false)

    /** Notificações apagadas por app desde que o cofre foi bloqueado (só em memória, sem conteúdo). */
    val suppressed = MutableStateFlow<Map<String, Int>>(emptyMap())

    fun noteSuppressed(pkg: String) {
        suppressed.value = suppressed.value + (pkg to ((suppressed.value[pkg] ?: 0) + 1))
    }

    fun clearSuppressed(pkg: String) {
        suppressed.value = suppressed.value - pkg
    }

    private val allowedUntil = ConcurrentHashMap<String, Long>()

    fun allow(pkg: String) {
        allowedUntil[pkg] = SystemClock.elapsedRealtime() + Store.graceSec * 1000L
    }

    /** Janela deslizante: cada uso dentro do prazo estende a liberação. */
    fun isAllowed(pkg: String): Boolean {
        val until = allowedUntil[pkg] ?: return false
        val now = SystemClock.elapsedRealtime()
        if (now > until) {
            allowedUntil.remove(pkg)
            return false
        }
        allowedUntil[pkg] = now + Store.graceSec * 1000L
        return true
    }

    fun lockAll() {
        allowedUntil.clear()
        vaultOpen.value = false
    }
}

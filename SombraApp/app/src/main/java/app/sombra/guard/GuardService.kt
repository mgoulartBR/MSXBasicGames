package app.sombra.guard

import android.accessibilityservice.AccessibilityService
import android.os.SystemClock
import android.view.accessibility.AccessibilityEvent
import app.sombra.core.Session
import app.sombra.data.Store
import app.sombra.ui.LockActivity

/**
 * Observa apenas o PACOTE da janela que veio para a frente (não lê conteúdo de tela).
 * Se for um app protegido no modo "falso erro" e ainda não liberado, cobre-o com a LockActivity.
 */
class GuardService : AccessibilityService() {
    private var lastShown = 0L

    override fun onAccessibilityEvent(event: AccessibilityEvent) {
        if (event.eventType != AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED) return
        val pkg = event.packageName?.toString() ?: return
        if (pkg == packageName) return
        if (pkg !in Store.lockedPackages()) return
        if (Session.isAllowed(pkg)) return
        val now = SystemClock.elapsedRealtime()
        if (now - lastShown < 500) return
        lastShown = now
        startActivity(LockActivity.intent(this, pkg))
    }

    override fun onInterrupt() = Unit
}

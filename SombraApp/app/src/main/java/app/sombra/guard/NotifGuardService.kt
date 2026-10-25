package app.sombra.guard

import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification
import app.sombra.core.Session
import app.sombra.data.Store

/**
 * Apaga as notificações dos apps protegidos assim que elas chegam. O texto nunca é lido nem guardado:
 * só contamos quantas foram apagadas para mostrar um selo no cofre.
 * Limitação do Android: notificações "em andamento" (ex.: chamadas, mídia) não podem ser apagadas por apps.
 */
class NotifGuardService : NotificationListenerService() {

    override fun onListenerConnected() {
        // varre o que já estava na barra quando o acesso foi concedido
        try { activeNotifications?.forEach { suppress(it) } } catch (_: Exception) { }
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) = suppress(sbn)

    private fun suppress(sbn: StatusBarNotification) {
        if (sbn.packageName !in Store.mutedPackages()) return
        if (sbn.isOngoing) return
        cancelNotification(sbn.key)
        Session.noteSuppressed(sbn.packageName)
    }
}

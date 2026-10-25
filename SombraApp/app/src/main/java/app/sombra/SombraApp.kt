package app.sombra

import android.app.Application
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import app.sombra.core.Freezer
import app.sombra.core.Session
import app.sombra.data.HideMode
import app.sombra.data.Store
import kotlin.concurrent.thread

class SombraApp : Application() {
    override fun onCreate() {
        super.onCreate()
        Store.init(this)
        // Tela apagou → fecha o cofre, revoga liberações e re-congela apps descongelados.
        registerReceiver(object : BroadcastReceiver() {
            override fun onReceive(c: Context, i: Intent) {
                Session.lockAll()
                if (Store.refreezeOnScreenOff) {
                    val frozen = Store.hidden.value.filter { it.mode == HideMode.FREEZE }
                    if (frozen.isNotEmpty()) thread { frozen.forEach { Freezer.setFrozen(c, it.pkg, true) } }
                }
            }
        }, IntentFilter(Intent.ACTION_SCREEN_OFF))
    }
}

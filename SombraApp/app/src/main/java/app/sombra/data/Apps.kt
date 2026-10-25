package app.sombra.data

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.drawable.AdaptiveIconDrawable
import android.graphics.drawable.Drawable
import android.util.LruCache
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.asImageBitmap
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

data class AppInfo(val pkg: String, val label: String)

object AppRepo {
    /** Último resultado em memória: a Home pinta instantaneamente e atualiza em segundo plano. */
    @Volatile var cache: List<AppInfo> = emptyList()
        private set

    suspend fun loadLaunchable(ctx: Context, includeSelf: Boolean = false): List<AppInfo> = withContext(Dispatchers.Default) {
        val pm = ctx.packageManager
        val intent = Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_LAUNCHER)
        val list = pm.queryIntentActivities(intent, 0)
            .map { AppInfo(it.activityInfo.packageName, it.loadLabel(pm).toString()) }
            .distinctBy { it.pkg }
            .filter { includeSelf || it.pkg != ctx.packageName }
            .sortedBy { it.label.lowercase() }
        cache = list
        list
    }

    fun launch(ctx: Context, pkg: String): Boolean {
        val i = ctx.packageManager.getLaunchIntentForPackage(pkg) ?: return false
        i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
        return try { ctx.startActivity(i); true } catch (_: Exception) { false }
    }

    fun label(ctx: Context, pkg: String): String = try {
        val pm = ctx.packageManager
        pm.getApplicationLabel(pm.getApplicationInfo(pkg, 0)).toString()
    } catch (_: PackageManager.NameNotFoundException) { pkg }
}

/** Carrega ícones fora da thread principal e os desenha todos na mesma forma (recorte feito na UI). */
object AppIcons {
    private val cache = object : LruCache<String, ImageBitmap>(256) {}

    fun peek(pkg: String, px: Int): ImageBitmap? = cache.get("$pkg@$px")

    suspend fun load(ctx: Context, pkg: String, px: Int): ImageBitmap? = withContext(Dispatchers.Default) {
        cache.get("$pkg@$px") ?: try {
            val d = ctx.packageManager.getApplicationIcon(pkg)
            render(d, px).asImageBitmap().also { cache.put("$pkg@$px", it) }
        } catch (_: Exception) { null }
    }

    private fun render(d: Drawable, size: Int): Bitmap {
        val bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888)
        val c = Canvas(bmp)
        if (d is AdaptiveIconDrawable) {
            // camadas têm 108dp; a área visível é 72dp → ampliamos 1,5x para preencher o ladrilho
            val o = (size * 0.25f).toInt()
            d.background?.also { it.setBounds(-o, -o, size + o, size + o); it.draw(c) }
            d.foreground?.also { it.setBounds(-o, -o, size + o, size + o); it.draw(c) }
        } else {
            val pad = (size * 0.08f).toInt()
            d.setBounds(pad, pad, size - pad, size - pad)
            d.draw(c)
        }
        return bmp
    }
}

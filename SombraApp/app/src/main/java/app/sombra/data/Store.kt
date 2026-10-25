package app.sombra.data

import android.content.Context
import android.content.SharedPreferences
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import java.security.MessageDigest
import java.security.SecureRandom
import java.util.Base64
import javax.crypto.SecretKeyFactory
import javax.crypto.spec.PBEKeySpec

enum class HideMode { LOCK, FREEZE, HOME }

data class HiddenApp(val pkg: String, val label: String, val mode: HideMode)

/** Preferências + lista de apps protegidos. Tudo em SharedPreferences privado do app. */
object Store {
    private lateinit var sp: SharedPreferences

    private val _hidden = MutableStateFlow<List<HiddenApp>>(emptyList())
    val hidden: StateFlow<List<HiddenApp>> = _hidden

    var autoLockSec by mutableStateOf(30); private set
    var graceSec by mutableStateOf(60); private set
    var refreezeOnScreenOff by mutableStateOf(true); private set
    var blockScreenshots by mutableStateOf(true); private set

    @Synchronized
    fun init(ctx: Context) {
        if (::sp.isInitialized) return
        sp = ctx.applicationContext.getSharedPreferences("sombra", Context.MODE_PRIVATE)
        autoLockSec = sp.getInt("auto_lock", 30)
        graceSec = sp.getInt("grace", 60)
        refreezeOnScreenOff = sp.getBoolean("refreeze", true)
        blockScreenshots = sp.getBoolean("block_shots", true)
        _hidden.value = readHidden()
    }

    // ---- apps protegidos ----
    private fun readHidden(): List<HiddenApp> =
        (sp.getStringSet("hidden", emptySet()) ?: emptySet()).mapNotNull {
            val p = it.split('\u0001')
            if (p.size < 3) null else runCatching { HiddenApp(p[1], p[2], HideMode.valueOf(p[0])) }.getOrNull()
        }.sortedBy { it.label.lowercase() }

    private fun writeHidden(list: List<HiddenApp>) {
        sp.edit().putStringSet("hidden", list.map { "${it.mode.name}\u0001${it.pkg}\u0001${it.label}" }.toSet()).apply()
        _hidden.value = list.sortedBy { it.label.lowercase() }
    }

    fun modeOf(pkg: String): HideMode? = _hidden.value.firstOrNull { it.pkg == pkg }?.mode

    fun put(app: HiddenApp) = writeHidden(_hidden.value.filter { it.pkg != app.pkg } + app)

    fun remove(pkg: String) = writeHidden(_hidden.value.filter { it.pkg != pkg })

    fun lockedPackages(): Set<String> =
        _hidden.value.filter { it.mode == HideMode.LOCK }.map { it.pkg }.toSet()

    // ---- ajustes ----
    fun setAutoLock(v: Int) { autoLockSec = v; sp.edit().putInt("auto_lock", v).apply() }
    fun setGrace(v: Int) { graceSec = v; sp.edit().putInt("grace", v).apply() }
    fun setRefreeze(v: Boolean) { refreezeOnScreenOff = v; sp.edit().putBoolean("refreeze", v).apply() }
    fun saveBlockScreenshots(v: Boolean) { blockScreenshots = v; sp.edit().putBoolean("block_shots", v).apply() }

    // ---- senha ----
    fun hasPin(): Boolean = sp.contains("pin_hash")

    fun setPin(pin: String) {
        val salt = ByteArray(16).also { SecureRandom().nextBytes(it) }
        sp.edit()
            .putString("pin_salt", Base64.getEncoder().encodeToString(salt))
            .putString("pin_hash", Base64.getEncoder().encodeToString(hash(pin, salt)))
            .apply()
    }

    fun checkPin(pin: String): Boolean {
        val salt = sp.getString("pin_salt", null) ?: return false
        val want = sp.getString("pin_hash", null) ?: return false
        return MessageDigest.isEqual(
            Base64.getDecoder().decode(want),
            hash(pin, Base64.getDecoder().decode(salt))
        )
    }

    /**
     * Resultado da calculadora → senha? Como só guardamos o hash, testamos o resultado inteiro
     * (se for inteiro não negativo com até PIN_MAX dígitos) contra o hash.
     */
    fun pinMatches(result: java.math.BigDecimal): Boolean {
        val r = result.stripTrailingZeros()
        if (r.signum() < 0 || r.scale() > 0) return false
        val digits = r.toBigInteger().toString()
        if (digits.length !in PIN_MIN..PIN_MAX) return false
        return checkPin(digits)
    }

    private fun hash(pin: String, salt: ByteArray): ByteArray =
        SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256")
            .generateSecret(PBEKeySpec(pin.toCharArray(), salt, 60_000, 256)).encoded

    const val PIN_MIN = 4
    const val PIN_MAX = 9
    fun validPin(p: String) = p.length in PIN_MIN..PIN_MAX && p.all { it.isDigit() } && p[0] != '0'
}

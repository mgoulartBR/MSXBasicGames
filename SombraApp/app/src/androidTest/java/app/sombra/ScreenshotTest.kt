package app.sombra

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.By
import androidx.test.uiautomator.UiDevice
import androidx.test.uiautomator.Until
import app.sombra.core.Session
import app.sombra.data.HiddenApp
import app.sombra.data.HideMode
import app.sombra.data.Store
import app.sombra.ui.CalculatorActivity
import app.sombra.ui.HomeActivity
import app.sombra.ui.LockActivity
import app.sombra.ui.SetupActivity
import org.junit.Assert.assertNotNull
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith

/** Percorre o app de verdade no emulador e salva screenshots em /sdcard/Download/sombra. */
@RunWith(AndroidJUnit4::class)
class ScreenshotTest {
    private val inst = InstrumentationRegistry.getInstrumentation()
    private val ctx: Context = inst.targetContext
    private val dev = UiDevice.getInstance(inst)
    private val pkg = "app.sombra"

    @Before fun setUp() {
        Store.init(ctx)
        Store.setPin("1234")
        Store.saveBlockScreenshots(false)
        dev.executeShellCommand("mkdir -p /sdcard/Download/sombra")
        dev.pressHome()
        // semeia o cofre com apps que existirem no emulador
        val seeds = listOf(
            "com.android.chrome" to HideMode.LOCK, "com.google.android.apps.maps" to HideMode.LOCK,
            "com.google.android.gm" to HideMode.FREEZE, "com.google.android.youtube" to HideMode.FREEZE,
            "com.google.android.apps.photos" to HideMode.HOME, "com.google.android.calendar" to HideMode.LOCK,
            "com.android.settings" to HideMode.HOME, "com.google.android.contacts" to HideMode.FREEZE,
        )
        Store.hidden.value.forEach { Store.remove(it.pkg) }
        seeds.forEach { (p, m) ->
            val label = try {
                ctx.packageManager.getApplicationLabel(ctx.packageManager.getApplicationInfo(p, 0)).toString()
            } catch (_: PackageManager.NameNotFoundException) { null }
            if (label != null) Store.put(HiddenApp(p, label, m))
        }
    }

    private fun shot(name: String) {
        Thread.sleep(900)
        dev.executeShellCommand("screencap -p /sdcard/Download/sombra/$name.png")
    }

    private fun tap(res: String) {
        val o = dev.wait(Until.findObject(By.res(pkg, res)), 8000)
        assertNotNull("não achei $res", o)
        o.click()
    }

    private fun start(cls: Class<*>) {
        ctx.startActivity(Intent(ctx, cls).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK))
        dev.wait(Until.hasObject(By.pkg(pkg)), 8000)
    }

    @Test fun walkthrough() {
        // 1. configuração inicial
        start(SetupActivity::class.java)
        shot("01_boas_vindas")
        tap("setup_start")
        listOf("1", "2", "3", "4").forEach { tap("pin_$it") }
        shot("02_criar_senha")

        // 2. calculadora → conta → cofre
        start(CalculatorActivity::class.java)
        listOf("key_1", "key_0", "key_0", "key_0", "key_+", "key_2", "key_3", "key_4").forEach { tap(it) }
        shot("03_calculadora")
        tap("key_=")
        dev.wait(Until.hasObject(By.res(pkg, "tab_add")), 8000)
        shot("04_cofre")

        tap("tab_add")
        shot("05_adicionar")
        dev.wait(Until.findObject(By.res(pkg, "app_row")), 8000).click()
        dev.wait(Until.hasObject(By.res(pkg, "btn_apply")), 8000)
        shot("06_escolher_metodo")
        dev.pressBack()

        dev.wait(Until.findObject(By.res(pkg, "tab_settings")), 8000).click()
        shot("07_ajustes")

        // 3. falso erro
        ctx.startActivity(LockActivity.intent(ctx, "com.android.chrome"))
        dev.wait(Until.hasObject(By.res(pkg, "fake_close")), 8000)
        shot("08_falso_erro")
        dev.wait(Until.findObject(By.res(pkg, "fake_close")), 8000).longClick()
        dev.wait(Until.hasObject(By.res(pkg, "pin_1")), 8000)
        listOf("1", "2", "3").forEach { tap("pin_$it") }
        shot("09_falso_erro_senha")

        // 4. Sombra Home
        start(HomeActivity::class.java)
        shot("10_home")
    }
}

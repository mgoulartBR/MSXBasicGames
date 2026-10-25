package app.sombra.ui

import android.content.Context
import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.BackHandler
import androidx.activity.compose.setContent
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.view.WindowCompat
import app.sombra.core.Session
import app.sombra.data.AppRepo
import app.sombra.data.Store

/**
 * Falso "O app parou". Segure "Fechar app" por ~1s para revelar o teclado de senha.
 * Tocar normalmente em qualquer botão apenas devolve o usuário à tela inicial.
 */
class LockActivity : ComponentActivity() {
    private var target = ""

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        target = intent.getStringExtra(EXTRA_PKG).orEmpty()
        val label = AppRepo.label(this, target)
        setContent {
            SombraTheme {
                BackHandler { goHome() }
                FakeCrashScreen(
                    pkg = target, label = label,
                    onDismiss = ::goHome,
                    onSubmitPin = { pin ->
                        if (Store.checkPin(pin)) {
                            Session.allow(target)
                            finish()
                            true
                        } else false
                    },
                )
            }
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        recreate()
    }

    private fun goHome() {
        startActivity(Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
        finish()
    }

    companion object {
        const val EXTRA_PKG = "pkg"
        fun intent(ctx: Context, pkg: String) = Intent(ctx, LockActivity::class.java)
            .putExtra(EXTRA_PKG, pkg)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_NO_ANIMATION)
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun FakeCrashScreen(
    pkg: String,
    label: String,
    onDismiss: () -> Unit,
    onSubmitPin: (String) -> Boolean,
    startWithPad: Boolean = false,
) {
    var pad by remember { mutableStateOf(startWithPad) }
    var pin by remember { mutableStateOf("") }
    var wrong by remember { mutableStateOf(false) }

    Box(Modifier.fillMaxSize().background(Color(0xFF000000)), contentAlignment = Alignment.Center) {
        Column(
            Modifier
                .statusBarsPadding()
                .navigationBarsPadding()
                .padding(horizontal = 28.dp)
                .clip(RoundedCornerShape(28.dp))
                .background(Color(0xFF2B2D31))
                .padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
        ) {
            if (!pad) {
                AppIcon(pkg, 44.dp)
                Spacer(Modifier.height(16.dp))
                Text(
                    "O app $label parou", color = Color(0xFFE6E6E9), fontSize = 20.sp,
                    fontWeight = FontWeight.Medium, lineHeight = 26.sp,
                )
                Spacer(Modifier.height(24.dp))
                Column(Modifier.fillMaxWidth(), horizontalAlignment = Alignment.Start) {
                    DialogRow("Informações do app", onClick = onDismiss)
                    DialogRow("Fechar app", onClick = onDismiss, onLongClick = { pad = true }, tag = "fake_close")
                    DialogRow("Enviar feedback", onClick = onDismiss)
                }
            } else {
                Text("Digite a senha", color = Color(0xFFE6E6E9), fontSize = 18.sp, fontWeight = FontWeight.Medium)
                Spacer(Modifier.height(16.dp))
                PinPad(pin, { pin = it; wrong = false }, error = wrong)
                Spacer(Modifier.height(14.dp))
                Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    Box(Modifier.weight(1f)) { PrimaryButton("Cancelar", modifier = Modifier.testTag("lock_cancel")) { onDismiss() } }
                    Box(Modifier.weight(1f)) {
                        PrimaryButton("Abrir", enabled = pin.length >= 4, modifier = Modifier.testTag("lock_open")) {
                            if (!onSubmitPin(pin)) { wrong = true; pin = "" }
                        }
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun DialogRow(text: String, onClick: () -> Unit, onLongClick: (() -> Unit)? = null, tag: String = "") {
    Box(
        Modifier
            .fillMaxWidth()
            .padding(vertical = 4.dp)
            .clip(RoundedCornerShape(50))
            .background(Color(0xFF383A40))
            .testTag(tag)
            .combinedClickable(onClick = onClick, onLongClick = onLongClick)
            .padding(horizontal = 20.dp, vertical = 14.dp),
    ) {
        Text(text, color = Color(0xFFA8C7FA), fontSize = 15.sp, fontWeight = FontWeight.Medium)
    }
}

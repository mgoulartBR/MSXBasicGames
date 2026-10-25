package app.sombra.ui

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.view.WindowCompat
import app.sombra.data.Store

/** Primeira execução: explica o funcionamento e cria a senha. */
class SetupActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        setContent {
            SombraTheme {
                SetupScreen(onDone = { pin ->
                    Store.setPin(pin)
                    startActivity(Intent(this, CalculatorActivity::class.java))
                    finish()
                })
            }
        }
    }
}

@Composable
fun SetupScreen(startStep: Int = 0, onDone: (String) -> Unit) {
    var step by remember { mutableIntStateOf(startStep) }
    Column(
        Modifier
            .fillMaxSize()
            .background(Pal.Bg)
            .statusBarsPadding()
            .navigationBarsPadding()
            .verticalScroll(rememberScrollState())
            .padding(horizontal = 28.dp, vertical = 24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Spacer(Modifier.height(12.dp))
        Box(
            Modifier.size(84.dp).clip(CircleShape)
                .background(Brush.linearGradient(listOf(Pal.Accent, Pal.Accent2))),
            contentAlignment = Alignment.Center,
        ) {
            Icon(SIcons.Shield, null, tint = Pal.Bg, modifier = Modifier.size(44.dp))
        }
        Spacer(Modifier.height(20.dp))
        if (step == 0) {
            Text("Sombra", color = Pal.Text, fontSize = 30.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.height(8.dp))
            Text(
                "Esconda e proteja apps sensíveis, como os do banco, atrás de uma calculadora comum.",
                color = Pal.Muted, fontSize = 15.sp, textAlign = TextAlign.Center, lineHeight = 22.sp,
            )
            Spacer(Modifier.height(28.dp))
            Step("1", "Você define uma senha numérica.")
            Step("2", "Na calculadora, digite a senha (ou qualquer conta cujo resultado seja ela, como 1000+234) e toque em  =")
            Step("3", "O cofre abre. Escolha como cada app será protegido.")
            Spacer(Modifier.height(28.dp))
            PrimaryButton("Começar", modifier = Modifier.testTag("setup_start")) { step = 1 }
        } else {
            PinCreateFlow(onDone = onDone)
        }
    }
}

@Composable
private fun Step(n: String, text: String) {
    androidx.compose.foundation.layout.Row(
        Modifier.padding(vertical = 7.dp),
        horizontalArrangement = Arrangement.spacedBy(14.dp),
        verticalAlignment = Alignment.Top,
    ) {
        Box(Modifier.size(28.dp).clip(CircleShape).background(Pal.SurfaceHi), contentAlignment = Alignment.Center) {
            Text(n, color = Pal.Accent, fontWeight = FontWeight.Bold, fontSize = 14.sp)
        }
        Text(text, color = Pal.Text, fontSize = 14.sp, lineHeight = 20.sp, modifier = Modifier.padding(top = 3.dp))
    }
}

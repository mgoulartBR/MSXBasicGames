package app.sombra.ui

import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
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
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.testTagsAsResourceId
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.view.WindowCompat
import app.sombra.core.Calc
import app.sombra.core.Session
import app.sombra.data.Store

/**
 * Calculadora funcional de fachada. Se o resultado da conta for igual à senha, abre o cofre.
 * Sem senha definida, redireciona para a configuração inicial.
 */
class CalculatorActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        if (!Store.hasPin()) {
            startActivity(Intent(this, SetupActivity::class.java))
            finish()
            return
        }
        setContent { CalculatorScreen(onUnlock = ::openVault) }
    }

    private fun openVault() {
        Session.vaultOpen.value = true
        startActivity(Intent(this, VaultActivity::class.java))
    }
}

private val Ops = setOf('+', '-', '×', '÷', '%')
private val CalcBg = Color(0xFF000000)
private val KeyNum = Color(0xFF2A2D33)
private val KeyFn = Color(0xFF4A4F57)
private val KeyOp = Color(0xFFFF9F0A)

@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun CalculatorScreen(onUnlock: () -> Unit) {
    var expr by remember { mutableStateOf("") }
    var fresh by remember { mutableStateOf(false) } // última ação foi "="

    val preview = remember(expr) {
        if (expr.any { it in Ops || it == '(' } && expr.last() !in Ops) Calc.eval(expr)?.let(Calc::format) else null
    }

    fun press(k: String) {
        when (k) {
            "AC" -> { expr = ""; fresh = false }
            "⌫" -> { expr = expr.dropLast(1); fresh = false }
            "=" -> {
                if (expr.isEmpty()) return
                val r = Calc.eval(expr)
                if (r == null) { expr = "Erro"; fresh = true; return }
                if (Store.hasPin() && Store.pinMatches(r)) {
                    expr = ""; fresh = false
                    onUnlock()
                } else {
                    expr = Calc.format(r); fresh = true
                }
            }
            "+", "-", "×", "÷", "%" -> {
                if (expr == "Erro") return
                fresh = false
                if (expr.isEmpty()) { if (k == "-") expr = "-"; return }
                expr = if (expr.last() in Ops && k != "%" && expr.last() != '%') expr.dropLast(1) + k else expr + k
            }
            else -> { // dígitos e ponto
                if (fresh || expr == "Erro") { expr = ""; fresh = false }
                if (k == "." && currentNumber(expr).contains('.')) return
                if (currentNumber(expr).length >= 12) return
                expr += if (k == "." && (expr.isEmpty() || expr.last() in Ops)) "0." else k
            }
        }
    }

    Column(
        Modifier
            .fillMaxSize()
            .background(CalcBg)
            .semantics { testTagsAsResourceId = true }
            .statusBarsPadding()
            .navigationBarsPadding()
            .padding(horizontal = 14.dp, vertical = 10.dp)
    ) {
        Column(
            Modifier.weight(1f).fillMaxWidth().padding(horizontal = 10.dp),
            verticalArrangement = Arrangement.Bottom,
            horizontalAlignment = Alignment.End,
        ) {
            Text(
                preview.orEmpty().let { if (it.isEmpty()) " " else "= $it" },
                color = Color(0xFF8E9299), fontSize = 26.sp, maxLines = 1,
            )
            Text(
                expr.ifEmpty { "0" },
                color = Color.White, fontSize = if (expr.length > 9) 44.sp else 68.sp,
                fontWeight = FontWeight.Light, maxLines = 1, overflow = TextOverflow.Clip,
                textAlign = TextAlign.End, modifier = Modifier.fillMaxWidth().testTag("display"),
            )
        }
        val rows = listOf(
            listOf("AC" to KeyFn, "⌫" to KeyFn, "%" to KeyFn, "÷" to KeyOp),
            listOf("7" to KeyNum, "8" to KeyNum, "9" to KeyNum, "×" to KeyOp),
            listOf("4" to KeyNum, "5" to KeyNum, "6" to KeyNum, "-" to KeyOp),
            listOf("1" to KeyNum, "2" to KeyNum, "3" to KeyNum, "+" to KeyOp),
        )
        rows.forEach { r ->
            Row(Modifier.fillMaxWidth().padding(vertical = 5.dp), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                r.forEach { (k, c) -> CalcKey(k, c, Modifier.weight(1f)) { press(k) } }
            }
        }
        Row(Modifier.fillMaxWidth().padding(vertical = 5.dp), horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            CalcKey("0", KeyNum, Modifier.weight(2.1f)) { press("0") }
            CalcKey(".", KeyNum, Modifier.weight(1f)) { press(".") }
            CalcKey("=", KeyOp, Modifier.weight(1f)) { press("=") }
        }
    }
}

private fun currentNumber(e: String): String {
    val i = e.indexOfLast { it in Ops }
    return if (i < 0) e else e.substring(i + 1)
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun CalcKey(label: String, bg: Color, modifier: Modifier, onClick: () -> Unit) {
    Box(
        modifier
            .height(74.dp)
            .clip(RoundedCornerShape(37.dp))
            .background(bg)
            .testTag("key_$label")
            .clickable(onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Text(label, color = Color.White, fontSize = if (label == "⌫") 24.sp else 30.sp, fontWeight = FontWeight.Normal)
    }
}

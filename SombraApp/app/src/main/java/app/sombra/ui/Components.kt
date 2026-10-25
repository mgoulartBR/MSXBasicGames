package app.sombra.ui

import androidx.compose.animation.animateColorAsState
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.Image
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.combinedClickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Home
import androidx.compose.material.icons.filled.Lock
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.ImageBitmap
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import app.sombra.data.AppIcons
import app.sombra.data.HideMode

fun modeIcon(m: HideMode): ImageVector = when (m) {
    HideMode.LOCK -> Icons.Filled.Lock
    HideMode.FREEZE -> SIcons.Snow
    HideMode.HOME -> Icons.Filled.Home
}

fun modeName(m: HideMode) = when (m) {
    HideMode.LOCK -> "Falso erro"
    HideMode.FREEZE -> "Congelado"
    HideMode.HOME -> "Sombra Home"
}

/** Ícone do app recortado numa forma única (squircle), para a grade ficar uniforme. */
@Composable
fun AppIcon(pkg: String, size: Dp, modifier: Modifier = Modifier) {
    val ctx = LocalContext.current
    val px = with(LocalDensity.current) { size.roundToPx() }
    val bmp by produceState<ImageBitmap?>(AppIcons.peek(pkg, px), pkg, px) {
        if (value == null) value = AppIcons.load(ctx, pkg, px)
    }
    Box(
        modifier
            .size(size)
            .clip(RoundedCornerShape(size * 0.28f))
            .background(Pal.SurfaceHi)
    ) {
        bmp?.let { Image(it, contentDescription = null, modifier = Modifier.fillMaxSize()) }
    }
}

@Composable
fun ModeBadge(mode: HideMode, modifier: Modifier = Modifier, size: Dp = 20.dp) {
    Box(
        modifier
            .size(size)
            .clip(CircleShape)
            .background(Pal.of(mode))
            .border(2.dp, Pal.Surface, CircleShape),
        contentAlignment = Alignment.Center,
    ) {
        Icon(modeIcon(mode), null, tint = Color(0xFF10131A), modifier = Modifier.size(size * 0.58f))
    }
}

/** Ladrilho da grade: cartão suave, ícone grande e nome em até 2 linhas. */
@OptIn(ExperimentalFoundationApi::class)
@Composable
fun AppTile(
    pkg: String,
    label: String,
    mode: HideMode?,
    onClick: () -> Unit,
    onLongClick: (() -> Unit)? = null,
    modifier: Modifier = Modifier,
    badgeCount: Int = 0,
) {
    Column(
        modifier
            .clip(RoundedCornerShape(20.dp))
            .background(Pal.Surface)
            .combinedClickable(onClick = onClick, onLongClick = onLongClick)
            .padding(horizontal = 4.dp, vertical = 12.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Box {
            AppIcon(pkg, 56.dp)
            if (mode != null) ModeBadge(mode, Modifier.align(Alignment.BottomEnd).padding(end = 0.dp), 20.dp)
            if (badgeCount > 0) {
                Box(
                    Modifier.align(Alignment.TopEnd).clip(CircleShape).background(Pal.Danger)
                        .border(2.dp, Pal.Surface, CircleShape).padding(horizontal = 6.dp, vertical = 1.dp),
                    contentAlignment = Alignment.Center,
                ) { Text(if (badgeCount > 9) "9+" else "$badgeCount", color = Color.White, fontSize = 10.sp, fontWeight = FontWeight.Bold) }
            }
        }
        Spacer(Modifier.height(8.dp))
        Text(
            label, color = Pal.Text, fontSize = 12.sp, lineHeight = 14.sp, maxLines = 2,
            overflow = TextOverflow.Ellipsis, textAlign = TextAlign.Center,
            modifier = Modifier.padding(horizontal = 4.dp).height(28.dp),
        )
    }
}

/** Teclado numérico + indicadores de dígitos. Usado em: criação de senha, troca e desbloqueio do falso erro. */
@Composable
fun PinPad(
    value: String,
    onChange: (String) -> Unit,
    maxLen: Int = 9,
    error: Boolean = false,
    modifier: Modifier = Modifier,
) {
    Column(modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
        Row(Modifier.height(28.dp), horizontalArrangement = Arrangement.spacedBy(12.dp), verticalAlignment = Alignment.CenterVertically) {
            val shown = maxOf(value.length, 4)
            repeat(shown) { i ->
                val filled = i < value.length
                val c by animateColorAsState(
                    when {
                        error -> Pal.Danger
                        filled -> Pal.Accent
                        else -> Pal.Outline
                    }, label = "dot"
                )
                Box(Modifier.size(if (filled) 14.dp else 12.dp).clip(CircleShape).background(c))
            }
        }
        Spacer(Modifier.height(24.dp))
        val keys = listOf("1", "2", "3", "4", "5", "6", "7", "8", "9", "", "0", "⌫")
        keys.chunked(3).forEach { row ->
            Row(Modifier.padding(vertical = 5.dp), horizontalArrangement = Arrangement.spacedBy(18.dp)) {
                row.forEach { k ->
                    if (k.isEmpty()) {
                        Spacer(Modifier.size(72.dp))
                    } else {
                        Box(
                            Modifier
                                .size(72.dp)
                                .clip(CircleShape)
                                .background(Pal.SurfaceHi)
                                .testTag("pin_$k")
                                .clickable {
                                    if (k == "⌫") onChange(value.dropLast(1))
                                    else if (value.length < maxLen) onChange(value + k)
                                },
                            contentAlignment = Alignment.Center,
                        ) {
                            Text(k, color = Pal.Text, fontSize = if (k == "⌫") 22.sp else 28.sp, fontWeight = FontWeight.Medium)
                        }
                    }
                }
            }
        }
    }
}

@Composable
fun PrimaryButton(text: String, enabled: Boolean = true, modifier: Modifier = Modifier, onClick: () -> Unit) {
    Box(
        modifier
            .fillMaxWidth()
            .height(52.dp)
            .clip(RoundedCornerShape(16.dp))
            .background(if (enabled) Pal.Accent else Pal.SurfaceHi)
            .clickable(enabled = enabled, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        Text(text, color = if (enabled) Color(0xFF0A1330) else Pal.Muted, fontWeight = FontWeight.SemiBold, fontSize = 16.sp)
    }
}

/** Fluxo "digite a nova senha → confirme". */
@Composable
fun PinCreateFlow(onDone: (String) -> Unit, modifier: Modifier = Modifier) {
    var first by remember { mutableStateOf<String?>(null) }
    var cur by remember { mutableStateOf("") }
    var err by remember { mutableStateOf<String?>(null) }
    Column(modifier.fillMaxWidth(), horizontalAlignment = Alignment.CenterHorizontally) {
        Text(
            if (first == null) "Crie sua senha" else "Confirme a senha",
            color = Pal.Text, fontSize = 20.sp, fontWeight = FontWeight.SemiBold,
        )
        Spacer(Modifier.height(4.dp))
        Text(
            err ?: "De 4 a 9 dígitos, sem começar por zero",
            color = if (err != null) Pal.Danger else Pal.Muted, fontSize = 13.sp,
        )
        Spacer(Modifier.height(20.dp))
        PinPad(cur, { cur = it; err = null }, error = err != null)
        Spacer(Modifier.height(16.dp))
        PrimaryButton(
            if (first == null) "Continuar" else "Salvar senha",
            enabled = cur.length >= 4,
            modifier = Modifier.testTag("pin_ok"),
        ) {
            val f = first
            if (f == null) {
                if (app.sombra.data.Store.validPin(cur)) { first = cur; cur = "" }
                else err = "Use 4 a 9 dígitos, sem começar por 0"
            } else if (f == cur) {
                onDone(cur)
            } else {
                err = "As senhas não conferem"; first = null; cur = ""
            }
        }
    }
}

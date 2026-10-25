package app.sombra.ui

import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.darkColorScheme
import androidx.compose.runtime.Composable
import androidx.compose.ui.graphics.Color
import app.sombra.data.HideMode

object Pal {
    val Bg = Color(0xFF0A0C11)
    val Surface = Color(0xFF141821)
    val SurfaceHi = Color(0xFF1D2330)
    val Outline = Color(0xFF2A3142)
    val Text = Color(0xFFE9EDF5)
    val Muted = Color(0xFF8A93A8)
    val Accent = Color(0xFF7C9BFF)
    val Accent2 = Color(0xFF3DDBB8)
    val Danger = Color(0xFFFF6B6B)

    fun of(m: HideMode) = when (m) {
        HideMode.LOCK -> Color(0xFFFFB454)
        HideMode.FREEZE -> Color(0xFF5CC8FF)
        HideMode.HOME -> Color(0xFFB794FF)
    }
}

@Composable
fun SombraTheme(content: @Composable () -> Unit) {
    MaterialTheme(
        colorScheme = darkColorScheme(
            primary = Pal.Accent, onPrimary = Color(0xFF0A1330),
            background = Pal.Bg, onBackground = Pal.Text,
            surface = Pal.Surface, onSurface = Pal.Text,
            surfaceVariant = Pal.SurfaceHi, onSurfaceVariant = Pal.Muted,
            outline = Pal.Outline, error = Pal.Danger,
            surfaceContainer = Pal.Surface, surfaceContainerHigh = Pal.SurfaceHi,
        ),
        content = content,
    )
}

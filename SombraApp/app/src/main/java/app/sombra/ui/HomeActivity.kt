package app.sombra.ui

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
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
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Search
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.runtime.collectAsState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.view.WindowCompat
import app.sombra.data.AppInfo
import app.sombra.data.AppRepo
import app.sombra.data.Store

/**
 * Lançador opcional e propositalmente leve: sem serviços, sem leitura de disco na partida.
 * Pinta na hora com o último resultado em memória e atualiza a lista em segundo plano.
 */
class HomeActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        WindowCompat.setDecorFitsSystemWindows(window, false)
        setContent { SombraTheme { HomeScreen() } }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent) // botão Home: volta ao topo
    }
}

@OptIn(ExperimentalFoundationApi::class)
@Composable
fun HomeScreen(preload: List<AppInfo>? = null) {
    val ctx = LocalContext.current
    var apps by remember { mutableStateOf(preload ?: AppRepo.cache) }
    var tick by remember { mutableIntStateOf(0) }
    var query by remember { mutableStateOf("") }
    val hidden by Store.hidden.collectAsState()
    BackHandler { }

    LaunchedEffect(tick) {
        if (preload == null) runCatching { apps = AppRepo.loadLaunchable(ctx, includeSelf = true) }
    }
    DisposableEffect(Unit) {
        val r = object : BroadcastReceiver() {
            override fun onReceive(c: Context, i: Intent) { tick++ }
        }
        val f = IntentFilter().apply {
            addAction(Intent.ACTION_PACKAGE_ADDED); addAction(Intent.ACTION_PACKAGE_REMOVED); addDataScheme("package")
        }
        ctx.registerReceiver(r, f)
        onDispose { ctx.unregisterReceiver(r) }
    }

    val hiddenPk = remember(hidden) { hidden.map { it.pkg }.toSet() }
    val visible = remember(apps, hiddenPk, query) {
        apps.filter { it.pkg !in hiddenPk && (query.isBlank() || it.label.contains(query, ignoreCase = true)) }
    }

    Column(
        Modifier
            .fillMaxSize()
            .background(Brush.verticalGradient(listOf(Color(0xCC0A0C11), Color(0x990A0C11), Color(0xE60A0C11))))
            .statusBarsPadding()
            .navigationBarsPadding()
            .padding(horizontal = 14.dp)
    ) {
        Spacer(Modifier.height(14.dp))
        HomeSearch(query) { query = it }
        Spacer(Modifier.height(14.dp))
        LazyVerticalGrid(
            columns = GridCells.Fixed(4),
            verticalArrangement = Arrangement.spacedBy(18.dp),
            horizontalArrangement = Arrangement.spacedBy(4.dp),
            modifier = Modifier.fillMaxSize(),
            contentPadding = androidx.compose.foundation.layout.PaddingValues(bottom = 24.dp),
        ) {
            items(visible, key = { it.pkg }) { a ->
                Column(
                    Modifier
                        .clip(RoundedCornerShape(16.dp))
                        .combinedClickable(onClick = { AppRepo.launch(ctx, a.pkg) })
                        .padding(vertical = 4.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                ) {
                    AppIcon(a.pkg, 60.dp)
                    Spacer(Modifier.height(6.dp))
                    Text(
                        a.label, color = Color.White, fontSize = 12.sp, maxLines = 1, overflow = TextOverflow.Ellipsis,
                        textAlign = TextAlign.Center, modifier = Modifier.padding(horizontal = 2.dp),
                    )
                }
            }
        }
    }
}

@Composable
private fun HomeSearch(query: String, onChange: (String) -> Unit) {
    Box(
        Modifier
            .fillMaxWidth()
            .height(48.dp)
            .clip(RoundedCornerShape(24.dp))
            .background(Color(0x66FFFFFF).copy(alpha = 0.12f))
            .padding(horizontal = 16.dp),
        contentAlignment = Alignment.CenterStart,
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Icon(Icons.Filled.Search, null, tint = Color(0xCCFFFFFF))
            Spacer(Modifier.width(10.dp))
            Box(Modifier.weight(1f)) {
                if (query.isEmpty()) Text("Buscar apps", color = Color(0x99FFFFFF), fontSize = 15.sp)
                BasicTextField(
                    value = query, onValueChange = onChange, singleLine = true,
                    textStyle = TextStyle(color = Color.White, fontSize = 15.sp),
                    cursorBrush = SolidColor(Color.White), modifier = Modifier.fillMaxWidth(),
                )
            }
        }
    }
}

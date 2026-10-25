package app.sombra.ui

import android.content.Context
import android.content.Intent
import android.provider.Settings
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.grid.GridCells
import androidx.compose.foundation.lazy.grid.GridItemSpan
import androidx.compose.foundation.lazy.grid.LazyVerticalGrid
import androidx.compose.foundation.lazy.grid.items
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.Add
import androidx.compose.material.icons.filled.Lock
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.Settings
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.ModalBottomSheet
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Switch
import androidx.compose.material3.SwitchDefaults
import androidx.compose.material3.Text
import androidx.compose.material3.rememberModalBottomSheetState
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.derivedStateOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.produceState
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.testTag
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.testTagsAsResourceId
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import app.sombra.core.Freezer
import app.sombra.core.ShizukuState
import app.sombra.data.AppInfo
import app.sombra.data.AppRepo
import app.sombra.data.HiddenApp
import app.sombra.data.HideMode
import app.sombra.data.Store
import kotlinx.coroutines.launch

/** Estado do ambiente que muda fora do app (permissões/serviços); reavaliado a cada onResume. */
data class Env(val guardOn: Boolean, val shizuku: ShizukuState, val homeDefault: Boolean)

@Composable
fun rememberEnv(): Env {
    val ctx = LocalContext.current
    val owner = LocalLifecycleOwner.current
    var tick by remember { mutableIntStateOf(0) }
    DisposableEffect(owner) {
        val o = LifecycleEventObserver { _, e -> if (e == Lifecycle.Event.ON_RESUME) tick++ }
        owner.lifecycle.addObserver(o)
        onDispose { owner.lifecycle.removeObserver(o) }
    }
    return remember(tick) { readEnv(ctx) }
}

private fun readEnv(ctx: Context): Env {
    val enabled = Settings.Secure.getString(ctx.contentResolver, Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES).orEmpty()
    val home = ctx.packageManager.resolveActivity(
        Intent(Intent.ACTION_MAIN).addCategory(Intent.CATEGORY_HOME), android.content.pm.PackageManager.MATCH_DEFAULT_ONLY
    )?.activityInfo?.packageName
    return Env(
        guardOn = enabled.contains(ctx.packageName) && enabled.contains("GuardService"),
        shizuku = Freezer.state(ctx),
        homeDefault = home == ctx.packageName,
    )
}

@OptIn(ExperimentalMaterial3Api::class, ExperimentalComposeUiApi::class)
@Composable
fun VaultScreen(actions: VaultActions, startTab: Int = 0) {
    var tab by rememberSaveable { mutableIntStateOf(startTab) }
    var editing by remember { mutableStateOf<AppInfo?>(null) }
    val env = rememberEnv()
    BackHandler { actions.close() }

    Column(
        Modifier
            .fillMaxSize()
            .background(Pal.Bg)
            .semantics { testTagsAsResourceId = true }
    ) {
        Box(Modifier.weight(1f)) {
            when (tab) {
                0 -> HiddenTab(actions, onAdd = { tab = 1 }, onEdit = { editing = it })
                1 -> AddTab(onPick = { editing = it })
                else -> SettingsTab(actions, env)
            }
        }
        NavigationBar(containerColor = Pal.Surface, modifier = Modifier.navigationBarsPadding()) {
            val colors = NavigationBarItemDefaults.colors(
                selectedIconColor = Pal.Bg, selectedTextColor = Pal.Accent, indicatorColor = Pal.Accent,
                unselectedIconColor = Pal.Muted, unselectedTextColor = Pal.Muted,
            )
            NavigationBarItem(tab == 0, { tab = 0 }, { Icon(SIcons.Shield, null) }, label = { Text("Cofre") },
                colors = colors, modifier = Modifier.testTag("tab_vault"))
            NavigationBarItem(tab == 1, { tab = 1 }, { Icon(Icons.Filled.Add, null) }, label = { Text("Adicionar") },
                colors = colors, modifier = Modifier.testTag("tab_add"))
            NavigationBarItem(tab == 2, { tab = 2 }, { Icon(Icons.Filled.Settings, null) }, label = { Text("Ajustes") },
                colors = colors, modifier = Modifier.testTag("tab_settings"))
        }
    }

    editing?.let { app ->
        ModeSheet(app, env, actions, onDismiss = { editing = null }, onOpenSettings = { editing = null; tab = 2 })
    }
}

// ───────────────────────── aba Cofre ─────────────────────────

@OptIn(ExperimentalFoundationApi::class)
@Composable
private fun HiddenTab(actions: VaultActions, onAdd: () -> Unit, onEdit: (AppInfo) -> Unit) {
    val hidden by Store.hidden.collectAsState()
    LazyVerticalGrid(
        columns = GridCells.Fixed(4),
        contentPadding = PaddingValues(start = 14.dp, end = 14.dp, top = 8.dp, bottom = 20.dp),
        horizontalArrangement = Arrangement.spacedBy(10.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
        modifier = Modifier.fillMaxSize().statusBarsPadding(),
    ) {
        item(span = { GridItemSpan(maxLineSpan) }) {
            Column {
                Row(Modifier.padding(top = 12.dp, bottom = 4.dp), verticalAlignment = Alignment.CenterVertically) {
                    Box(
                        Modifier.size(46.dp).clip(CircleShape)
                            .background(Brush.linearGradient(listOf(Pal.Accent, Pal.Accent2))),
                        contentAlignment = Alignment.Center,
                    ) { Icon(SIcons.Shield, null, tint = Pal.Bg, modifier = Modifier.size(26.dp)) }
                    Spacer(Modifier.width(14.dp))
                    Column(Modifier.weight(1f)) {
                        Text("Cofre", color = Pal.Text, fontSize = 26.sp, fontWeight = FontWeight.Bold)
                        Text(
                            if (hidden.isEmpty()) "Nenhum app protegido" else "${hidden.size} ${if (hidden.size == 1) "app protegido" else "apps protegidos"}",
                            color = Pal.Muted, fontSize = 13.sp,
                        )
                    }
                    Row(
                        Modifier.clip(RoundedCornerShape(50)).background(Pal.SurfaceHi).testTag("btn_lock")
                            .clickable { actions.close() }.padding(horizontal = 14.dp, vertical = 9.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Icon(Icons.Filled.Lock, null, tint = Pal.Text, modifier = Modifier.size(16.dp))
                        Spacer(Modifier.width(6.dp))
                        Text("Bloquear", color = Pal.Text, fontSize = 13.sp, fontWeight = FontWeight.Medium)
                    }
                }
                if (hidden.isNotEmpty()) {
                    Spacer(Modifier.height(10.dp))
                    Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                        HideMode.values().forEach { m ->
                            val n = hidden.count { it.mode == m }
                            if (n > 0) Chip("${modeName(m)} · $n", Pal.of(m))
                        }
                    }
                    Spacer(Modifier.height(6.dp))
                    Text("Toque para abrir · segure para editar", color = Pal.Muted, fontSize = 12.sp)
                    Spacer(Modifier.height(6.dp))
                }
            }
        }
        if (hidden.isEmpty()) {
            item(span = { GridItemSpan(maxLineSpan) }) {
                Column(
                    Modifier.fillMaxWidth().padding(top = 40.dp).clip(RoundedCornerShape(24.dp)).background(Pal.Surface).padding(28.dp),
                    horizontalAlignment = Alignment.CenterHorizontally,
                ) {
                    Icon(SIcons.EyeOff, null, tint = Pal.Accent, modifier = Modifier.size(40.dp))
                    Spacer(Modifier.height(14.dp))
                    Text("Seu cofre está vazio", color = Pal.Text, fontSize = 17.sp, fontWeight = FontWeight.SemiBold)
                    Spacer(Modifier.height(6.dp))
                    Text(
                        "Adicione apps de banco e outros que você quer manter longe de olhares curiosos.",
                        color = Pal.Muted, fontSize = 13.sp, textAlign = TextAlign.Center, lineHeight = 19.sp,
                    )
                    Spacer(Modifier.height(18.dp))
                    PrimaryButton("Adicionar apps", onClick = onAdd)
                }
            }
        } else {
            items(hidden, key = { it.pkg }) { a ->
                AppTile(
                    a.pkg, a.label, a.mode,
                    onClick = { actions.open(a) },
                    onLongClick = { onEdit(AppInfo(a.pkg, a.label)) },
                    modifier = Modifier.testTag("hidden_tile"),
                )
            }
        }
    }
}

@Composable
private fun Chip(text: String, color: Color) {
    Row(
        Modifier.clip(RoundedCornerShape(50)).background(color.copy(alpha = 0.14f)).padding(horizontal = 10.dp, vertical = 5.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(Modifier.size(7.dp).clip(CircleShape).background(color))
        Spacer(Modifier.width(6.dp))
        Text(text, color = color, fontSize = 12.sp, fontWeight = FontWeight.Medium)
    }
}

// ───────────────────────── aba Adicionar ─────────────────────────

@Composable
private fun AddTab(onPick: (AppInfo) -> Unit) {
    val ctx = LocalContext.current
    val hidden by Store.hidden.collectAsState()
    val all by produceState(AppRepo.cache) { value = AppRepo.loadLaunchable(ctx) }
    var q by remember { mutableStateOf("") }
    val list by remember(all, hidden) {
        derivedStateOf {
            // apps congelados somem do lançador, então juntamos os já protegidos para poder editá-los
            val extra = hidden.filter { h -> all.none { it.pkg == h.pkg } }.map { AppInfo(it.pkg, it.label) }
            (all + extra).sortedBy { it.label.lowercase() }
                .filter { q.isBlank() || it.label.contains(q, ignoreCase = true) }
        }
    }
    Column(Modifier.fillMaxSize().statusBarsPadding().padding(horizontal = 14.dp)) {
        Text("Adicionar apps", color = Pal.Text, fontSize = 26.sp, fontWeight = FontWeight.Bold, modifier = Modifier.padding(top = 20.dp))
        Text("Escolha o app e como ele será protegido", color = Pal.Muted, fontSize = 13.sp)
        Spacer(Modifier.height(14.dp))
        Row(
            Modifier.fillMaxWidth().height(48.dp).clip(RoundedCornerShape(24.dp)).background(Pal.Surface).padding(horizontal = 16.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            Icon(Icons.Filled.Search, null, tint = Pal.Muted)
            Spacer(Modifier.width(10.dp))
            Box(Modifier.weight(1f)) {
                if (q.isEmpty()) Text("Buscar", color = Pal.Muted, fontSize = 15.sp)
                BasicTextField(
                    q, { q = it }, singleLine = true, textStyle = TextStyle(color = Pal.Text, fontSize = 15.sp),
                    cursorBrush = SolidColor(Pal.Accent), modifier = Modifier.fillMaxWidth().testTag("search"),
                )
            }
        }
        Spacer(Modifier.height(10.dp))
        LazyColumn(verticalArrangement = Arrangement.spacedBy(8.dp), contentPadding = PaddingValues(bottom = 16.dp)) {
            items(list, key = { it.pkg }) { a ->
                val mode = hidden.firstOrNull { it.pkg == a.pkg }?.mode
                Row(
                    Modifier.fillMaxWidth().clip(RoundedCornerShape(18.dp)).background(Pal.Surface)
                        .testTag("app_row").clickable { onPick(a) }.padding(horizontal = 12.dp, vertical = 10.dp),
                    verticalAlignment = Alignment.CenterVertically,
                ) {
                    AppIcon(a.pkg, 44.dp)
                    Spacer(Modifier.width(14.dp))
                    Column(Modifier.weight(1f)) {
                        Text(a.label, color = Pal.Text, fontSize = 15.sp, fontWeight = FontWeight.Medium, maxLines = 1, overflow = TextOverflow.Ellipsis)
                        Text(a.pkg, color = Pal.Muted, fontSize = 11.sp, maxLines = 1, overflow = TextOverflow.Ellipsis)
                    }
                    if (mode != null) {
                        Chip(modeName(mode), Pal.of(mode))
                    } else {
                        Box(Modifier.size(32.dp).clip(CircleShape).background(Pal.SurfaceHi), contentAlignment = Alignment.Center) {
                            Icon(Icons.Filled.Add, null, tint = Pal.Accent, modifier = Modifier.size(18.dp))
                        }
                    }
                }
            }
        }
    }
}

// ───────────────────────── escolha do método ─────────────────────────

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun ModeSheet(app: AppInfo, env: Env, actions: VaultActions, onDismiss: () -> Unit, onOpenSettings: () -> Unit) {
    val current = Store.modeOf(app.pkg)
    var selected by remember { mutableStateOf(current ?: HideMode.LOCK) }
    var busy by remember { mutableStateOf(false) }
    var error by remember { mutableStateOf<String?>(null) }
    val scope = rememberCoroutineScope()
    val state = rememberModalBottomSheetState(skipPartiallyExpanded = true)

    ModalBottomSheet(onDismissRequest = onDismiss, sheetState = state, containerColor = Pal.Surface) {
        Column(
            Modifier.navigationBarsPadding().verticalScroll(rememberScrollState()).padding(horizontal = 20.dp).padding(bottom = 16.dp)
        ) {
            Row(verticalAlignment = Alignment.CenterVertically) {
                AppIcon(app.pkg, 52.dp)
                Spacer(Modifier.width(14.dp))
                Column {
                    Text(app.label, color = Pal.Text, fontSize = 19.sp, fontWeight = FontWeight.SemiBold)
                    Text("Como proteger este app?", color = Pal.Muted, fontSize = 13.sp)
                }
            }
            Spacer(Modifier.height(16.dp))
            ModeOption(
                HideMode.LOCK, selected == HideMode.LOCK, { selected = HideMode.LOCK },
                "Falso erro",
                "O app continua instalado, mas ao abrir aparece “O app parou”. Segure “Fechar app” para digitar a senha.",
                if (env.guardOn) null else "Ative o serviço de bloqueio em Ajustes",
            )
            ModeOption(
                HideMode.FREEZE, selected == HideMode.FREEZE, { selected = HideMode.FREEZE },
                "Congelar (invisível)",
                "Some de todos os lançadores e das Configurações; não abre nem recebe notificações. Só reaparece pelo cofre.",
                when (env.shizuku) {
                    ShizukuState.READY -> null
                    ShizukuState.NOT_INSTALLED -> "Requer o app Shizuku (veja Ajustes)"
                    ShizukuState.NOT_RUNNING -> "Inicie o Shizuku (veja Ajustes)"
                    ShizukuState.NO_PERMISSION -> "Permita o acesso ao Shizuku em Ajustes"
                },
            )
            ModeOption(
                HideMode.HOME, selected == HideMode.HOME, { selected = HideMode.HOME },
                "Só na Sombra Home",
                "Não aparece no lançador Sombra Home. Outros lançadores continuam mostrando o app.",
                if (env.homeDefault) null else "A Sombra Home não é o lançador padrão",
            )
            error?.let {
                Spacer(Modifier.height(10.dp))
                Text(it, color = Pal.Danger, fontSize = 13.sp)
            }
            Spacer(Modifier.height(16.dp))
            PrimaryButton(
                if (busy) "Aplicando…" else if (current == null) "Proteger app" else "Salvar",
                enabled = !busy, modifier = Modifier.testTag("btn_apply"),
            ) {
                busy = true; error = null
                scope.launch {
                    val r = actions.apply(app, selected)
                    busy = false
                    if (r == null) onDismiss() else error = r
                }
            }
            if (current != null) {
                Spacer(Modifier.height(8.dp))
                Box(
                    Modifier.fillMaxWidth().height(48.dp).clip(RoundedCornerShape(16.dp)).clickable(enabled = !busy) {
                        busy = true; error = null
                        scope.launch {
                            val r = actions.apply(app, null)
                            busy = false
                            if (r == null) onDismiss() else error = r
                        }
                    },
                    contentAlignment = Alignment.Center,
                ) { Text("Remover do cofre", color = Pal.Danger, fontWeight = FontWeight.Medium) }
            }
        }
    }
}

@Composable
private fun ModeOption(mode: HideMode, selected: Boolean, onClick: () -> Unit, title: String, body: String, warn: String?) {
    val c = Pal.of(mode)
    Row(
        Modifier
            .fillMaxWidth()
            .padding(vertical = 5.dp)
            .clip(RoundedCornerShape(18.dp))
            .background(if (selected) c.copy(alpha = 0.10f) else Pal.SurfaceHi.copy(alpha = 0.5f))
            .border(1.5.dp, if (selected) c else Color.Transparent, RoundedCornerShape(18.dp))
            .testTag("mode_${mode.name}")
            .clickable(onClick = onClick)
            .padding(14.dp),
        verticalAlignment = Alignment.Top,
    ) {
        Box(Modifier.size(40.dp).clip(CircleShape).background(c.copy(alpha = 0.18f)), contentAlignment = Alignment.Center) {
            Icon(modeIcon(mode), null, tint = c, modifier = Modifier.size(22.dp))
        }
        Spacer(Modifier.width(14.dp))
        Column(Modifier.weight(1f)) {
            Text(title, color = Pal.Text, fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
            Spacer(Modifier.height(3.dp))
            Text(body, color = Pal.Muted, fontSize = 12.5.sp, lineHeight = 17.sp)
            if (warn != null) {
                Spacer(Modifier.height(6.dp))
                Text("⚠ $warn", color = Color(0xFFFFB454), fontSize = 12.sp, fontWeight = FontWeight.Medium)
            }
        }
    }
}

// ───────────────────────── aba Ajustes ─────────────────────────

@Composable
private fun SettingsTab(actions: VaultActions, env: Env) {
    var changePin by remember { mutableStateOf(false) }
    Column(
        Modifier.fillMaxSize().statusBarsPadding().verticalScroll(rememberScrollState()).padding(horizontal = 14.dp).padding(bottom = 24.dp)
    ) {
        Text("Ajustes", color = Pal.Text, fontSize = 26.sp, fontWeight = FontWeight.Bold, modifier = Modifier.padding(top = 20.dp, bottom = 6.dp))

        Section("Métodos de proteção")
        MethodCard(
            HideMode.LOCK, "Falso erro",
            if (env.guardOn) "Serviço ativo" else "Serviço desligado",
            env.guardOn, if (env.guardOn) "Gerenciar" else "Ativar", actions::openAccessibilitySettings,
        )
        MethodCard(
            HideMode.FREEZE, "Congelar com Shizuku",
            when (env.shizuku) {
                ShizukuState.READY -> "Pronto para uso"
                ShizukuState.NOT_INSTALLED -> "Shizuku não encontrado"
                ShizukuState.NOT_RUNNING -> "Shizuku instalado, mas parado"
                ShizukuState.NO_PERMISSION -> "Falta permitir o acesso"
            },
            env.shizuku == ShizukuState.READY,
            when (env.shizuku) {
                ShizukuState.READY -> "Abrir Shizuku"
                ShizukuState.NOT_INSTALLED -> "Instalar"
                ShizukuState.NOT_RUNNING -> "Iniciar"
                ShizukuState.NO_PERMISSION -> "Permitir"
            },
        ) { if (env.shizuku == ShizukuState.NO_PERMISSION) actions.requestShizuku() else actions.openShizuku() }
        MethodCard(
            HideMode.HOME, "Sombra Home",
            if (env.homeDefault) "É o lançador padrão" else "Não é o lançador padrão",
            env.homeDefault, "Definir", actions::openHomeSettings,
        )

        Section("Segurança")
        ChoiceRow("Fechar o cofre após", listOf(15, 30, 60, 300), Store.autoLockSec, Store::setAutoLock)
        ChoiceRow("Pedir senha de novo (falso erro) após", listOf(30, 60, 300, 900), Store.graceSec, Store::setGrace)
        SwitchRow("Congelar de novo ao apagar a tela", "Apps descongelados pelo cofre voltam a ficar invisíveis", Store.refreezeOnScreenOff, Store::setRefreeze)
        SwitchRow("Bloquear capturas de tela no cofre", "Também esconde o cofre na tela de apps recentes", Store.blockScreenshots, Store::setBlockScreenshots)
        ActionRow("Alterar senha") { changePin = true }

        Section("Dicas para Xiaomi / Poco (HyperOS)")
        Column(Modifier.fillMaxWidth().clip(RoundedCornerShape(18.dp)).background(Pal.Surface).padding(16.dp)) {
            Tip("Segurança › Permissões › Início automático: permita para este app.")
            Tip("Informações do app › Outras permissões: ative “Exibir pop-ups durante execução em segundo plano”.")
            Tip("Economia de bateria: escolha “Sem restrições” para o serviço de bloqueio não ser encerrado.")
            Spacer(Modifier.height(10.dp))
            Box(
                Modifier.clip(RoundedCornerShape(50)).background(Pal.SurfaceHi).clickable { actions.openAppDetails() }
                    .padding(horizontal = 16.dp, vertical = 10.dp)
            ) { Text("Abrir informações do app", color = Pal.Accent, fontSize = 13.sp, fontWeight = FontWeight.Medium) }
        }
        Spacer(Modifier.height(10.dp))
        Text(
            "Sombra 1.0 · o serviço de acessibilidade só lê o nome do app que abriu; nada sai do aparelho.",
            color = Pal.Muted, fontSize = 11.5.sp, lineHeight = 16.sp, modifier = Modifier.padding(horizontal = 4.dp),
        )
    }
    if (changePin) {
        AlertDialog(
            onDismissRequest = { changePin = false },
            confirmButton = {},
            containerColor = Pal.Surface,
            text = { PinCreateFlow(onDone = { Store.setPin(it); changePin = false }) },
        )
    }
}

@Composable
private fun Section(t: String) =
    Text(t.uppercase(), color = Pal.Muted, fontSize = 12.sp, fontWeight = FontWeight.SemiBold, letterSpacing = 0.8.sp,
        modifier = Modifier.padding(start = 4.dp, top = 22.dp, bottom = 8.dp))

@Composable
private fun MethodCard(mode: HideMode, title: String, status: String, ok: Boolean, action: String, onClick: () -> Unit) {
    val c = Pal.of(mode)
    Row(
        Modifier.fillMaxWidth().padding(vertical = 4.dp).clip(RoundedCornerShape(18.dp)).background(Pal.Surface).padding(14.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Box(Modifier.size(40.dp).clip(CircleShape).background(c.copy(alpha = 0.18f)), contentAlignment = Alignment.Center) {
            Icon(modeIcon(mode), null, tint = c, modifier = Modifier.size(22.dp))
        }
        Spacer(Modifier.width(14.dp))
        Column(Modifier.weight(1f)) {
            Text(title, color = Pal.Text, fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
            Row(verticalAlignment = Alignment.CenterVertically) {
                Box(Modifier.size(7.dp).clip(CircleShape).background(if (ok) Pal.Accent2 else Color(0xFFFFB454)))
                Spacer(Modifier.width(6.dp))
                Text(status, color = Pal.Muted, fontSize = 12.5.sp)
            }
        }
        Box(
            Modifier.clip(RoundedCornerShape(50)).background(if (ok) Pal.SurfaceHi else Pal.Accent).clickable(onClick = onClick)
                .padding(horizontal = 14.dp, vertical = 8.dp)
        ) { Text(action, color = if (ok) Pal.Text else Pal.Bg, fontSize = 13.sp, fontWeight = FontWeight.SemiBold) }
    }
}

@Composable
private fun ChoiceRow(title: String, options: List<Int>, value: Int, onSelect: (Int) -> Unit) {
    Column(Modifier.fillMaxWidth().padding(vertical = 4.dp).clip(RoundedCornerShape(18.dp)).background(Pal.Surface).padding(14.dp)) {
        Text(title, color = Pal.Text, fontSize = 14.sp, fontWeight = FontWeight.Medium)
        Spacer(Modifier.height(10.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            options.forEach { o ->
                val sel = o == value
                Box(
                    Modifier.clip(RoundedCornerShape(50)).background(if (sel) Pal.Accent else Pal.SurfaceHi)
                        .clickable { onSelect(o) }.padding(horizontal = 14.dp, vertical = 8.dp)
                ) {
                    Text(
                        if (o < 60) "${o}s" else "${o / 60} min",
                        color = if (sel) Pal.Bg else Pal.Text, fontSize = 13.sp, fontWeight = FontWeight.Medium,
                    )
                }
            }
        }
    }
}

@Composable
private fun SwitchRow(title: String, sub: String, checked: Boolean, onChange: (Boolean) -> Unit) {
    Row(
        Modifier.fillMaxWidth().padding(vertical = 4.dp).clip(RoundedCornerShape(18.dp)).background(Pal.Surface)
            .clickable { onChange(!checked) }.padding(14.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(Modifier.weight(1f)) {
            Text(title, color = Pal.Text, fontSize = 14.sp, fontWeight = FontWeight.Medium)
            Text(sub, color = Pal.Muted, fontSize = 12.sp, lineHeight = 16.sp)
        }
        Spacer(Modifier.width(10.dp))
        Switch(
            checked, onChange,
            colors = SwitchDefaults.colors(checkedTrackColor = Pal.Accent, checkedThumbColor = Pal.Bg, uncheckedTrackColor = Pal.SurfaceHi, uncheckedBorderColor = Pal.Outline),
        )
    }
}

@Composable
private fun ActionRow(title: String, onClick: () -> Unit) {
    Row(
        Modifier.fillMaxWidth().padding(vertical = 4.dp).clip(RoundedCornerShape(18.dp)).background(Pal.Surface)
            .clickable(onClick = onClick).padding(14.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) { Text(title, color = Pal.Text, fontSize = 14.sp, fontWeight = FontWeight.Medium) }
}

@Composable
private fun Tip(t: String) =
    Text("•  $t", color = Pal.Muted, fontSize = 12.5.sp, lineHeight = 18.sp, modifier = Modifier.padding(vertical = 3.dp))

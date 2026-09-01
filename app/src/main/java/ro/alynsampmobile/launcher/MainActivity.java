package ro.alynsampmobile.launcher;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.StrictMode;
import android.util.Log;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;

import com.applovin.mediation.MaxAd;
import com.applovin.mediation.MaxAdListener;
import com.applovin.mediation.MaxAdViewAdListener;
import com.applovin.mediation.MaxError;
import com.applovin.mediation.ads.MaxInterstitialAd;
import com.google.android.material.button.MaterialButton;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textview.MaterialTextView;
import com.joom.paranoid.Obfuscate;

import java.util.concurrent.TimeUnit;

import ro.alynsampmobile.launcher.ui.fragment.SettingsPageFragment;
import ro.alynsampmobile.launcher.utils.SampQuery;
import ro.alynsampmobile.launcher.utils.Utils;

/**
 * Tela principal (novo visual: uma unica tela com imagem de fundo, sem abas
 * embaixo). Tem os botoes "Escolher Servidor" e "Configuracoes" no topo, os
 * links sociais na lateral esquerda, e o botao de conectar com o contador de
 * players embaixo. As Configuracoes abrem como um painel por cima dessa tela.
 */
@Obfuscate
public class MainActivity extends AppCompatActivity implements MaxAdListener, MaxAdViewAdListener {
    private MaxInterstitialAd interstitialAd = null;

    private int retryAttempt;

    // TODO: troque pelo IP/porta definitivos do seu servidor quando tiver.
    private static final String SERVER1_HOST = "172.96.140.62";
    private static final String SERVER1_PORT = "3255";
    private static final String SERVER1_NAME = "Nativo RPG";

    // TODO: preencha com os links reais das redes sociais.
    private static final String DISCORD_URL = "";
    private static final String YOUTUBE_URL = "";
    private static final String INSTAGRAM_URL = "";

    private final Handler mainHandler = new Handler(Looper.getMainLooper());
    private final Handler playersRefreshHandler = new Handler(Looper.getMainLooper());
    private static final int PLAYERS_REFRESH_INTERVAL_MS = 15000;

    private View settingsContainer;
    private ImageButton btnCloseSettings;
    private TextView playersOnlineText;
    private boolean settingsOpen = false;

    private final Runnable playersRefreshRunnable = new Runnable() {
        @Override
        public void run() {
            refreshPlayerCount();
            playersRefreshHandler.postDelayed(this, PLAYERS_REFRESH_INTERVAL_MS);
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder().permitAll().build());
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);

        if (!getIntent().getStringExtra("extra_check").equals("alynsampmobile1337")) {
            Log.e("MainActivity", "Not joined from launcher!");
            finish();
            return;
        }

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        hideSystemUI();

        try {
            if (interstitialAd == null && !Utils.isTester(this)) {
                createInterstitialAd();
            }
        } catch (Exception e) {
            Log.e("MainActivity", "Error creating interstitialAd: " + e.getMessage());
        }

        ((MaterialTextView) findViewById(R.id.ahahaha)).setText(Utils.copyright);

        if (Utils.isTester(this)) {
            new AlertDialog.Builder(this).setTitle("Tester")
                    .setMessage("You are a tester! You have access to the latest features and updates.")
                    .setPositiveButton("Ok", null).setCancelable(false).show();
        }

        // garante que nenhuma instalacao antiga fique presa no modo offline (removido)
        getSharedPreferences("samp_settings", Context.MODE_PRIVATE).edit()
                .putBoolean("offline_mode", false).apply();

        settingsContainer = findViewById(R.id.settingsContainer);
        btnCloseSettings = findViewById(R.id.btnCloseSettings);
        playersOnlineText = findViewById(R.id.playersOnlineText);

        ImageButton btnDiscord = findViewById(R.id.btnDiscord);
        ImageButton btnYoutube = findViewById(R.id.btnYoutube);
        ImageButton btnInstagram = findViewById(R.id.btnInstagram);
        MaterialButton btnChooseServer = findViewById(R.id.btnChooseServer);
        MaterialButton btnSettings = findViewById(R.id.btnSettings);
        MaterialButton btnConnect = findViewById(R.id.btnConnect);

        btnDiscord.setOnClickListener(v -> openLink(DISCORD_URL));
        btnYoutube.setOnClickListener(v -> openLink(YOUTUBE_URL));
        btnInstagram.setOnClickListener(v -> openLink(INSTAGRAM_URL));

        btnChooseServer.setOnClickListener(v -> showChooseServerDialog());
        btnSettings.setOnClickListener(v -> openSettingsPanel());
        btnCloseSettings.setOnClickListener(v -> closeSettingsPanel());
        btnConnect.setOnClickListener(v -> showConnectDialog());
    }

    @Override
    protected void onResume() {
        super.onResume();
        playersRefreshHandler.post(playersRefreshRunnable);
    }

    @Override
    protected void onPause() {
        super.onPause();
        playersRefreshHandler.removeCallbacks(playersRefreshRunnable);
    }

    private void openLink(String url) {
        if (url == null || url.trim().isEmpty()) {
            Toast.makeText(this, R.string.coming_soon_message, Toast.LENGTH_LONG).show();
            return;
        }
        try {
            startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Consulta o servidor 1 em segundo plano e atualiza o contador de players
     * que fica acima do botao de conectar (ex: "10 / 200 online").
     */
    private void refreshPlayerCount() {
        new Thread(() -> {
            try {
                SampQuery query = new SampQuery(SERVER1_HOST, Integer.parseInt(SERVER1_PORT));
                if (query.isOnline()) {
                    String[] info = query.getInfo();
                    if (info != null) {
                        String text = getString(R.string.players_online_format, info[1], info[2]);
                        mainHandler.post(() -> playersOnlineText.setText(text));
                        return;
                    }
                }
                mainHandler.post(() -> playersOnlineText.setText(R.string.players_online_unknown));
            } catch (Exception e) {
                e.printStackTrace();
                mainHandler.post(() -> playersOnlineText.setText(R.string.players_online_unknown));
            }
        }).start();
    }

    /**
     * Painel pra trocar entre os 3 servidores (mesmos cards de antes). Servidor
     * 1 e' o unico funcional; 2 e 3 mostram "em breve".
     */
    private void showChooseServerDialog() {
        View dialogView = getLayoutInflater().inflate(R.layout.dialog_choose_server, null);

        AlertDialog dialog = new AlertDialog.Builder(this).setView(dialogView).create();

        TextView server1Name = dialogView.findViewById(R.id.server1_name);
        server1Name.setText(SERVER1_NAME);

        View server1Card = dialogView.findViewById(R.id.server1_card);
        View server2Card = dialogView.findViewById(R.id.server2_card);
        View server3Card = dialogView.findViewById(R.id.server3_card);

        // servidor 1 e' o unico ativo no momento, entao so fecha o painel
        server1Card.setOnClickListener(v -> dialog.dismiss());

        View.OnClickListener comingSoonListener = v -> Toast.makeText(
                this, R.string.coming_soon_message, Toast.LENGTH_LONG).show();
        server2Card.setOnClickListener(comingSoonListener);
        server3Card.setOnClickListener(comingSoonListener);

        dialog.show();
    }

    /**
     * Pede o nickname e conecta direto no servidor 1 (sem tela de transicao).
     */
    private void showConnectDialog() {
        View dialogView = getLayoutInflater().inflate(R.layout.layout_server1_connect, null);

        AlertDialog dialog = new AlertDialog.Builder(this).setView(dialogView).create();

        TextView serverPlayers = dialogView.findViewById(R.id.server_players);
        TextView serverGamemode = dialogView.findViewById(R.id.server_gamemode);
        TextInputEditText nicknameInput = dialogView.findViewById(R.id.nickname_input_text);
        MaterialButton connectButton = dialogView.findViewById(R.id.connect_button);

        String savedNick = getSharedPreferences("samp_settings", Context.MODE_PRIVATE).getString("nick_name", "");
        nicknameInput.setText(savedNick);
        nicknameInput.setOnEditorActionListener((textView, i, keyEvent) -> i == EditorInfo.IME_ACTION_DONE || i == EditorInfo.IME_ACTION_NEXT || i == EditorInfo.IME_ACTION_UNSPECIFIED);

        serverPlayers.setText("Players: --");
        serverGamemode.setText("Gamemode: --");

        dialog.show();

        new Thread(() -> {
            try {
                SampQuery query = new SampQuery(SERVER1_HOST, Integer.parseInt(SERVER1_PORT));
                if (query.isOnline()) {
                    String[] info = query.getInfo();
                    if (info != null) {
                        String players = "Players: " + info[1] + " / " + info[2];
                        String gamemode = "Gamemode: " + info[4];
                        mainHandler.post(() -> {
                            serverPlayers.setText(players);
                            serverGamemode.setText(gamemode);
                        });
                        return;
                    }
                }
                mainHandler.post(() -> serverPlayers.setText(getString(R.string.server_offline_msg)));
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();

        connectButton.setOnClickListener(v -> {
            if (!Utils.isOnline(this)) {
                Toast.makeText(this, "No internet connection!", Toast.LENGTH_LONG).show();
                dialog.dismiss();
                return;
            }

            String nickname = nicknameInput.getText() != null ? nicknameInput.getText().toString() : "";

            if (nickname.isEmpty()) {
                Toast.makeText(this, "Please set nickname.", Toast.LENGTH_LONG).show();
                return;
            }

            if (nickname.length() > 24) {
                Toast.makeText(this, "Nickname can't be longer than 24 characters.", Toast.LENGTH_LONG).show();
                return;
            }

            if (Utils.isServerBanned(SERVER1_HOST, SERVER1_PORT)) {
                Toast.makeText(this, "This server is banned on this launcher.", Toast.LENGTH_LONG).show();
                dialog.dismiss();
                return;
            }

            dialog.dismiss();

            getSharedPreferences("samp_settings", Context.MODE_PRIVATE).edit()
                    .putString("nick_name", nickname).apply();

            SharedPreferences.Editor edit = getSharedPreferences("samp_server", Context.MODE_PRIVATE).edit();
            edit.putString("host", SERVER1_HOST);
            edit.putString("port", SERVER1_PORT);
            edit.putString("password", "");
            edit.apply();

            Utils.saveSettings(this);

            try {
                Intent intent = new Intent(this, ro.alynsampmobile.game.SAMP.class);
                intent.putExtra("extra_check", "alynsampmobile1337");
                startActivity(intent);
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
    }

    private void openSettingsPanel() {
        getSupportFragmentManager().beginTransaction()
                .setReorderingAllowed(true)
                .replace(R.id.settingsContainer, SettingsPageFragment.class, null)
                .commit();
        settingsContainer.setVisibility(View.VISIBLE);
        btnCloseSettings.setVisibility(View.VISIBLE);
        settingsOpen = true;
    }

    private void closeSettingsPanel() {
        settingsContainer.setVisibility(View.GONE);
        btnCloseSettings.setVisibility(View.GONE);
        settingsOpen = false;
    }

    /**
     * Esconde a barra de status e a barra de navegacao do Android, deixando a
     * tela cheia (igual ao que ja acontece quando o jogo abre). Precisa ser
     * chamado de novo no onWindowFocusChanged porque o Android tende a
     * restaurar as barras quando o usuario troca de app e volta.
     */
    private void hideSystemUI() {
        View decorView = getWindow().getDecorView();
        decorView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideSystemUI();
        }
    }

    void createInterstitialAd() {
        interstitialAd = new MaxInterstitialAd("YOUR_AD_UNIT_ID", this);
        interstitialAd.setListener(this);

        // Load the first ad
        interstitialAd.loadAd();
    }

    // MAX Ad Listener
    @Override
    public void onAdLoaded(final MaxAd maxAd) {
        // Interstitial ad is ready to be shown. interstitialAd.isReady() will now return 'true'

        // Reset retry attempt
        retryAttempt = 0;
    }

    @Override
    public void onAdLoadFailed(final String adUnitId, final MaxError error) {
        // Interstitial ad failed to load
        // AppLovin recommends that you retry with exponentially higher delays up to a maximum delay (in this case 64 seconds)

        retryAttempt++;
        long delayMillis = TimeUnit.SECONDS.toMillis((long) Math.pow(2, Math.min(6, retryAttempt)));

        new Handler().postDelayed(() -> interstitialAd.loadAd(), delayMillis);
    }

    @Override
    public void onAdDisplayFailed(final MaxAd maxAd, final MaxError error) {
        // Interstitial ad failed to display. AppLovin recommends that you load the next ad.
        interstitialAd.loadAd();
    }

    @Override
    public void onAdDisplayed(final MaxAd maxAd) {
    }

    @Override
    public void onAdClicked(final MaxAd maxAd) {
    }

    @Override
    public void onAdHidden(final MaxAd maxAd) {
        // Interstitial ad is hidden. Pre-load the next ad
        interstitialAd.loadAd();
    }

    @Override
    public void onAdExpanded(final MaxAd maxAd) {
    }

    @Override
    public void onAdCollapsed(final MaxAd maxAd) {
    }

    @Override
    public void onBackPressed() {
        if (settingsOpen) {
            closeSettingsPanel();
            return;
        }

        super.onBackPressed();
        try {
            if (interstitialAd.isReady()) {
                interstitialAd.showAd();
            } else {
                Log.e("interstitialAd", "interstitialAd not ready!");
            }
        } catch (Exception e) {
            Log.e("MainActivity", "Error showing interstitialAd: " + e.getMessage());
        }

        quitApp();
    }

    private long quit_time = 0;

    public void quitApp() {
        if ((System.currentTimeMillis() - quit_time) > 2000) {
            Toast.makeText(this, "Press again to quit.", Toast.LENGTH_LONG).show();
            quit_time = System.currentTimeMillis();
        } else {
            finish();
        }
    }
}

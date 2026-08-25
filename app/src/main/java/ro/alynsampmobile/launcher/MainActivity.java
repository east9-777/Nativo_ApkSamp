package ro.alynsampmobile.launcher;

import android.os.Bundle;
import android.os.Handler;
import android.os.StrictMode;
import android.util.Log;
import android.view.View;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;

import com.applovin.mediation.MaxAd;
import com.applovin.mediation.MaxAdListener;
import com.applovin.mediation.MaxAdViewAdListener;
import com.applovin.mediation.MaxError;
import com.applovin.mediation.ads.MaxInterstitialAd;
import com.google.android.material.textview.MaterialTextView;
import com.joom.paranoid.Obfuscate;

import java.util.concurrent.TimeUnit;

import ro.alynsampmobile.launcher.ui.fragment.OnlineMenuFragment;
import ro.alynsampmobile.launcher.ui.fragment.SettingsPageFragment;
import ro.alynsampmobile.launcher.ui.fragment.StoryPageFragment;
import ro.alynsampmobile.launcher.utils.Utils;

@Obfuscate
public class MainActivity extends AppCompatActivity implements MaxAdListener, MaxAdViewAdListener {
    private MaxInterstitialAd interstitialAd = null;

    private int retryAttempt;
    private int selectedTab = 1;

    private MaterialTextView tabOnline, tabStory, tabSettings;

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

        tabOnline = findViewById(R.id.tabOnline);
        tabStory = findViewById(R.id.tabStory);
        tabSettings = findViewById(R.id.tabSettings);

        // aba Online e' a padrao ao abrir o app
        getSupportFragmentManager().beginTransaction()
                .setReorderingAllowed(true)
                .replace(R.id.fragmentContainer, OnlineMenuFragment.class, null)
                .commit();
        selectTab(1);

        tabOnline.setOnClickListener(v -> {
            if (selectedTab != 1) {
                getSupportFragmentManager().beginTransaction()
                        .setReorderingAllowed(true)
                        .replace(R.id.fragmentContainer, OnlineMenuFragment.class, null)
                        .commit();
                selectTab(1);
            }
        });

        tabStory.setOnClickListener(v -> {
            if (selectedTab != 2) {
                getSupportFragmentManager().beginTransaction()
                        .setReorderingAllowed(true)
                        .replace(R.id.fragmentContainer, StoryPageFragment.class, null)
                        .commit();
                selectTab(2);
            }
        });

        tabSettings.setOnClickListener(v -> {
            if (selectedTab != 3) {
                getSupportFragmentManager().beginTransaction()
                        .setReorderingAllowed(true)
                        .replace(R.id.fragmentContainer, SettingsPageFragment.class, null)
                        .commit();
                selectTab(3);
            }
        });
    }

    /**
     * Marca visualmente qual aba esta selecionada (fundo claro atras do texto,
     * igual ao seletor do GTA V) e desmarca as outras.
     */
    private void selectTab(int tab) {
        tabOnline.setBackgroundResource(tab == 1 ? R.drawable.tab_active_bg : android.R.color.transparent);
        tabOnline.setTextColor(getResources().getColor(tab == 1 ? android.R.color.black : android.R.color.white));

        tabStory.setBackgroundResource(tab == 2 ? R.drawable.tab_active_bg : android.R.color.transparent);
        tabStory.setTextColor(getResources().getColor(tab == 2 ? android.R.color.black : android.R.color.white));

        tabSettings.setBackgroundResource(tab == 3 ? R.drawable.tab_active_bg : android.R.color.transparent);
        tabSettings.setTextColor(getResources().getColor(tab == 3 ? android.R.color.black : android.R.color.white));

        selectedTab = tab;
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

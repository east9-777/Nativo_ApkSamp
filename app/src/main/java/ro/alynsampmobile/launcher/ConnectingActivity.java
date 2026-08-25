package ro.alynsampmobile.launcher;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.textview.MaterialTextView;
import com.joom.paranoid.Obfuscate;

import ro.alynsampmobile.launcher.utils.Utils;

/**
 * Tela de transicao mostrada entre o jogador apertar "Connect" num servidor e o
 * jogo (SAMP) realmente abrir. Fundo troca entre 5 imagens (tela_car_1 a
 * tela_car_5) - a mesma ideia da tela de loading do GTA V.
 *
 * Nao faz nenhuma verificacao/logica de conexao em si (isso e' tudo feito pelo
 * motor nativo do jogo depois que a activity SAMP abre) - e so uma transicao
 * visual de alguns segundos.
 */
@Obfuscate
public class ConnectingActivity extends AppCompatActivity {

    private static final int IMAGE_INTERVAL_MS = 2500;   // troca de imagem a cada 2.5s
    private static final int TOTAL_DURATION_MS = 5000;   // tempo total nessa tela antes de abrir o jogo

    private final int[] backgroundImages = {
            R.drawable.tela_car_1,
            R.drawable.tela_car_2,
            R.drawable.tela_car_3,
            R.drawable.tela_car_4,
            R.drawable.tela_car_5
    };
    private int backgroundImageIndex = 0;
    private final Handler handler = new Handler();

    private final Runnable backgroundSwapper = new Runnable() {
        @Override
        public void run() {
            ImageView backgroundImage = findViewById(R.id.background_image);
            if (backgroundImage != null) {
                backgroundImageIndex = (backgroundImageIndex + 1) % backgroundImages.length;
                backgroundImage.setImageResource(backgroundImages[backgroundImageIndex]);
            }
            handler.postDelayed(this, IMAGE_INTERVAL_MS);
        }
    };

    private final Runnable launchGame = this::startGame;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_connecting);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        hideSystemUI();

        ((MaterialTextView) findViewById(R.id.ahahaha)).setText(Utils.copyright);

        handler.postDelayed(backgroundSwapper, IMAGE_INTERVAL_MS);
        handler.postDelayed(launchGame, TOTAL_DURATION_MS);
    }

    private void startGame() {
        try {
            Intent intent = new Intent(this, ro.alynsampmobile.game.SAMP.class);
            intent.putExtra("extra_check", "alynsampmobile1337");
            startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
        }
        finish();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        handler.removeCallbacks(backgroundSwapper);
        handler.removeCallbacks(launchGame);
    }

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
}

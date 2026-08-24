package ro.alynsampmobile.launcher;

import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.WindowManager;
import android.widget.ImageView;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.joom.paranoid.Obfuscate;

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
}

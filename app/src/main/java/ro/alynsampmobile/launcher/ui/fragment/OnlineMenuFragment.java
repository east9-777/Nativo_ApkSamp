package ro.alynsampmobile.launcher.ui.fragment;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.inputmethod.EditorInfo;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;

import com.google.android.material.button.MaterialButton;
import com.google.android.material.textfield.TextInputEditText;
import com.joom.paranoid.Obfuscate;

import ro.alynsampmobile.launcher.R;
import ro.alynsampmobile.launcher.utils.SampQuery;
import ro.alynsampmobile.launcher.utils.Utils;

/**
 * Aba ONLINE, estilo GTA V: um card grande (servidor 1, fixo e funcional) +
 * dois cards menores empilhados (servidor 2 e 3, "em breve", sem IP definido ainda).
 *
 * O IP/porta do servidor 1 fica fixo em codigo (SERVER1_HOST / SERVER1_PORT), o
 * jogador nao escolhe/edita servidor - diferente do antigo ServersPageFragment.
 */
@Obfuscate
public class OnlineMenuFragment extends Fragment {

    // TODO: troque pelo IP/porta definitivos do seu servidor quando tiver.
    private static final String SERVER1_HOST = "172.96.140.62";
    private static final String SERVER1_PORT = "3255";
    private static final String SERVER1_NAME = "Nativo RPG";

    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_online_page, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        View server1Card = view.findViewById(R.id.server1_card);
        View server2Card = view.findViewById(R.id.server2_card);
        View server3Card = view.findViewById(R.id.server3_card);

        TextView server1Name = view.findViewById(R.id.server1_name);
        server1Name.setText(SERVER1_NAME);

        server1Card.setOnClickListener(v -> onServer1Clicked());

        View.OnClickListener comingSoonListener = v -> Toast.makeText(
                requireContext(),
                R.string.coming_soon_message,
                Toast.LENGTH_LONG
        ).show();

        server2Card.setOnClickListener(comingSoonListener);
        server3Card.setOnClickListener(comingSoonListener);

        // garante que o app nao esta preso no modo offline vindo de uma sessao anterior
        requireActivity().getSharedPreferences("samp_settings", Context.MODE_PRIVATE)
                .edit().putBoolean("offline_mode", false).apply();
    }

    private void onServer1Clicked() {
        View dialogView = getLayoutInflater().inflate(R.layout.layout_server1_connect, null);

        AlertDialog.Builder dialogBuilder = new AlertDialog.Builder(requireContext()).setView(dialogView);
        AlertDialog dialog = dialogBuilder.create();

        TextView serverPlayers = dialogView.findViewById(R.id.server_players);
        TextView serverGamemode = dialogView.findViewById(R.id.server_gamemode);
        TextInputEditText nicknameInput = dialogView.findViewById(R.id.nickname_input_text);
        MaterialButton connectButton = dialogView.findViewById(R.id.connect_button);

        String savedNick = requireActivity().getSharedPreferences("samp_settings", Context.MODE_PRIVATE).getString("nick_name", "");
        nicknameInput.setText(savedNick);
        nicknameInput.setOnEditorActionListener((textView, i, keyEvent) -> i == EditorInfo.IME_ACTION_DONE || i == EditorInfo.IME_ACTION_NEXT || i == EditorInfo.IME_ACTION_UNSPECIFIED);

        serverPlayers.setText("Players: --");
        serverGamemode.setText("Gamemode: --");

        dialog.show();

        // consulta o servidor em segundo plano pra mostrar jogadores/gamemode reais
        new Thread(() -> {
            try {
                SampQuery query = new SampQuery(SERVER1_HOST, Integer.parseInt(SERVER1_PORT));
                if (query.isOnline()) {
                    String[] info = query.getInfo();
                    if (info != null) {
                        String players = "Players: " + info[1] + " / " + info[2];
                        String gamemode = "Gamemode: " + info[4];
                        mainHandler.post(() -> {
                            if (isAdded()) {
                                serverPlayers.setText(players);
                                serverGamemode.setText(gamemode);
                            }
                        });
                        return;
                    }
                }
                mainHandler.post(() -> {
                    if (isAdded()) {
                        serverPlayers.setText(getString(R.string.server_offline_msg));
                    }
                });
            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();

        connectButton.setOnClickListener(v -> {
            if (!Utils.isOnline(requireActivity())) {
                Toast.makeText(requireContext(), "No internet connection!", Toast.LENGTH_LONG).show();
                dialog.dismiss();
                return;
            }

            String nickname = nicknameInput.getText() != null ? nicknameInput.getText().toString() : "";

            if (nickname.isEmpty()) {
                Toast.makeText(requireContext(), "Please set nickname.", Toast.LENGTH_LONG).show();
                return;
            }

            if (nickname.length() > 24) {
                Toast.makeText(requireContext(), "Nickname can't be longer than 24 characters.", Toast.LENGTH_LONG).show();
                return;
            }

            if (Utils.isServerBanned(SERVER1_HOST, SERVER1_PORT)) {
                Toast.makeText(requireContext(), "This server is banned on this launcher.", Toast.LENGTH_LONG).show();
                dialog.dismiss();
                return;
            }

            dialog.dismiss();

            requireActivity().getSharedPreferences("samp_settings", Context.MODE_PRIVATE).edit()
                    .putString("nick_name", nickname).apply();

            SharedPreferences.Editor edit = requireActivity().getSharedPreferences("samp_server", Context.MODE_PRIVATE).edit();
            edit.putString("host", SERVER1_HOST);
            edit.putString("port", SERVER1_PORT);
            edit.putString("password", "");
            edit.apply();

            Utils.saveSettings(requireActivity());

            try {
                Intent intent = new Intent(requireActivity(), ro.alynsampmobile.launcher.ConnectingActivity.class);
                requireActivity().startActivity(intent);
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
    }
}

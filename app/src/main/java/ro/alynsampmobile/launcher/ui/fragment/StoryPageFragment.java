package ro.alynsampmobile.launcher.ui.fragment;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.fragment.app.Fragment;

import com.joom.paranoid.Obfuscate;

import ro.alynsampmobile.launcher.ConnectingActivity;
import ro.alynsampmobile.launcher.R;

/**
 * Aba STORY: modo offline (jogar sozinho, sem servidor). Reaproveita a mesma
 * flag "offline_mode" que ja existia no codigo original (antes usada dentro
 * da lista de servidores) - so muda a tela, a logica por baixo e a mesma.
 */
@Obfuscate
public class StoryPageFragment extends Fragment {

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_story_page, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);

        View playButton = view.findViewById(R.id.play_offline_button);
        playButton.setOnClickListener(v -> new AlertDialog.Builder(requireContext())
                .setTitle(R.string.offline_mode)
                .setMessage(R.string.offline_mode_confirm)
                .setPositiveButton("Yes", (dialog, which) -> startOfflineGame())
                .setNegativeButton("No", null)
                .show());
    }

    private void startOfflineGame() {
        requireActivity().getSharedPreferences("samp_settings", Context.MODE_PRIVATE)
                .edit().putBoolean("offline_mode", true).apply();

        try {
            Intent intent = new Intent(requireActivity(), ConnectingActivity.class);
            requireActivity().startActivity(intent);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}

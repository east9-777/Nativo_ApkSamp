package ro.alynsampmobile.launcher.utils;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

import com.joom.paranoid.Obfuscate;

/**
 * Representa um pacote .zip que deve ser baixado inteiro e descompactado
 * dentro da pasta "files" do app (diferente de FileData, que é 1 arquivo solto).
 *
 * Usado para casos como a texdb, que tem centenas de arquivos pequenos e é
 * mais prático hospedar como um único .zip numa GitHub Release do que subir
 * arquivo por arquivo.
 */
@Obfuscate
public class ArchiveData {
    private final String name;      // nome de exibição, ex: "texdb"
    private final String path;      // pasta de destino relativa a getExternalFilesDir(null), ex: "texdb"
    private final long size;        // tamanho do .zip em bytes (usado para saber se precisa baixar de novo)
    private final String url;       // URL de download do .zip (ex: link de asset de uma GitHub Release)

    public ArchiveData(String name, String path, long size, String url) {
        this.name = name;
        this.path = path;
        this.size = size;
        this.url = url;
    }

    public String getName() {
        return name;
    }

    public String getPath() {
        return path;
    }

    public long getSize() {
        return size;
    }

    public String getUrl() {
        return url;
    }

    public static ArrayList<ArchiveData> getListByJson(JSONObject json) throws JSONException {
        ArrayList<ArchiveData> list = new ArrayList<>();
        if (!json.has("archives")) {
            return list; // arquivo antigo sem essa chave, ou nenhum archive definido
        }
        JSONArray arr = json.getJSONArray("archives");
        for (int i = 0; i < arr.length(); i++) {
            JSONObject item = arr.getJSONObject(i);
            list.add(new ArchiveData(
                    item.getString("name"),
                    item.getString("path"),
                    item.getLong("size"),
                    item.getString("url")));
        }
        return list;
    }
}

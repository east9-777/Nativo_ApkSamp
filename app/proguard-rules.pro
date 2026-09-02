-keep class com.nvidia.devtech.* { *; }
-keep class com.rockstargames.gtasa.* { *; }
-keep class com.wardrumstudios.utils.* { *; }

-keep class com.nativo.rpg.game.* { *; }
-keep class com.nativo.rpg.game.ui.* { *; }
-keep class com.nativo.rpg.game.ui.widgets.* { *; }
-keep class com.nativo.rpg.game.ui.widgets.adapter.* { *; }

-keep class com.nativo.rpg.br.utils.SignatureChecker { *; }

# for minify
-dontwarn javax.servlet.**
-dontwarn org.conscrypt.**
-dontwarn org.bouncycastle.**
-dontwarn org.openjsse.**
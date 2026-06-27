package com.goldrp.game.ui.hud;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Handler;
import android.os.Looper;
import android.text.Html;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.joom.paranoid.Obfuscate;
import com.goldrp.game.R;
import com.goldrp.game.core.Samp;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Locale;
import java.util.Timer;
import java.util.TimerTask;

@Obfuscate
public class Hud {
    private Activity activity;
    public boolean isShow;
    native void HudInit();
    private boolean d = false;
    public DecimalFormat formatter;
    private int current_real_money;
    private int current_visual_money;
    Thread thread_update_money;
    boolean shop_action = false;
    Timer money_timer;
    private boolean isHudSetPos = false;
    native void alt();
    native void N();
    native void onWeaponChanged();
    native void SetRadarBgPos(float x1, float y1, float x2, float y2);
    native void SetRadarPos(float x1, float y1, float size);
    private ArrayList<ImageView> hud_wanted;
    public ViewGroup viewGroup = null;

    private int lastHealth = -1;
    private int lastArmour = -1;

    private String playerName = "EGOR KUZN";
    private int playerId = 0;

    private ConstraintLayout mainHud;
    private ConstraintLayout hudInfoLayout;
    private ConstraintLayout healthLayout;
    private TextView healthText;
    private ConstraintLayout armourLayout;
    private TextView armourText;
    private ConstraintLayout eatLayout;
    private TextView eatText;
    private TextView hudMoney;
    private ConstraintLayout sampButtonsLayout;
    private ImageView hudButtonMenu;
    private ImageView hudButtonStar;
    private ImageView hudButtonInv;
    private ImageView hudButtonShop;
    private ImageView hudButtonHelp;
    private ImageView radarZone;
    private ConstraintLayout radarLayout;
    private TextView hudAmmo;
    private ConstraintLayout selectWeapon;
    private ImageView weaponButton;
    private ImageView star1;
    private ImageView star2;
    private ImageView star3;
    private ImageView star4;
    private ImageView star5;
    private ConstraintLayout hudLogoLayout;
    private ImageView hudLogoImg;
    private TextView hudLogoText;

    private static final int BUTTON_MENU = 0;
    private static final int BUTTON_STAR = 1;
    private static final int BUTTON_INV = 2;
    private static final int BUTTON_SHOP = 3;
    private static final int BUTTON_HELP = 4;

    private Handler handler = new Handler(Looper.getMainLooper());

    native void nativeClick(int buttonId);

    private boolean isTablet() {
        return (activity.getResources().getConfiguration().screenLayout
                & Configuration.SCREENLAYOUT_SIZE_MASK)
                >= Configuration.SCREENLAYOUT_SIZE_LARGE;
    }

    @SuppressLint("ClickableViewAccessibility")
    public Hud(Activity activity) {
        this.activity = activity;

        viewGroup = (ViewGroup) LayoutInflater.from(activity).inflate(R.layout.hud, null);

        initViews();
        HudInit();

        loadPlayerNameFromSettings();

        updateLogoText();

        weaponButton.setOnClickListener(v -> onWeaponChanged());

        setupButtonWithAnimation(hudButtonMenu, BUTTON_MENU);
        setupButtonWithAnimation(hudButtonStar, BUTTON_STAR);
        setupButtonWithAnimation(hudButtonInv, BUTTON_INV);
        setupButtonWithAnimation(hudButtonShop, BUTTON_SHOP);
        setupButtonWithAnimation(hudButtonHelp, BUTTON_HELP);

        formatter = new DecimalFormat();
        DecimalFormatSymbols symbols = DecimalFormatSymbols.getInstance();
        symbols.setGroupingSeparator(',');
        formatter.setDecimalFormatSymbols(symbols);

        hud_wanted = new ArrayList<>();
        hud_wanted.add(star1);
        hud_wanted.add(star2);
        hud_wanted.add(star3);
        hud_wanted.add(star4);
        hud_wanted.add(star5);
        for (int i = 0; i < hud_wanted.size(); i++) {
            hud_wanted.get(i).setVisibility(View.VISIBLE);
        }

        if (eatText != null) {
            eatText.setText("100%");
        }

        isShow = false;
        viewVisible(viewGroup, View.GONE);
    }

    private void setupButtonWithAnimation(ImageView button, int buttonId) {
        button.setOnClickListener(v -> {
            animateButtonPress(button);
            nativeClick(buttonId);
        });
    }

    private void animateButtonPress(ImageView button) {
        button.animate()
                .scaleX(0.8f)
                .scaleY(0.8f)
                .setDuration(100)
                .withEndAction(() -> {
                    button.animate()
                            .scaleX(1.0f)
                            .scaleY(1.0f)
                            .setDuration(100)
                            .start();
                })
                .start();

        button.animate()
                .alpha(0.5f)
                .setDuration(100)
                .withEndAction(() -> {
                    button.animate()
                            .alpha(1.0f)
                            .setDuration(100)
                            .start();
                })
                .start();
    }

    private void setupButtonWithHighlight(ImageView button, int buttonId) {
        button.setOnClickListener(v -> {
            float originalAlpha = button.getAlpha();

            button.setColorFilter(0x88FFFFFF, android.graphics.PorterDuff.Mode.SRC_ATOP);
            button.animate()
                    .scaleX(0.9f)
                    .scaleY(0.9f)
                    .setDuration(80)
                    .withEndAction(() -> {
                        handler.postDelayed(() -> {
                            button.clearColorFilter();
                            button.animate()
                                    .scaleX(1.0f)
                                    .scaleY(1.0f)
                                    .setDuration(80)
                                    .start();
                        }, 50);
                    })
                    .start();

            nativeClick(buttonId);
        });
    }

    public void attachToContainer(FrameLayout container) {
        if (viewGroup != null && container != null) {
            container.addView(viewGroup, -1, -1);
        }
    }

    private void initViews() {
        mainHud = viewGroup.findViewById(R.id.main_hud);
        hudInfoLayout = viewGroup.findViewById(R.id.hud_info_layout);
        healthLayout = viewGroup.findViewById(R.id.health_layout);
        healthText = viewGroup.findViewById(R.id.health_text);
        armourLayout = viewGroup.findViewById(R.id.armour_layout);
        armourText = viewGroup.findViewById(R.id.armour_text);
        eatLayout = viewGroup.findViewById(R.id.eat_layout);
        eatText = viewGroup.findViewById(R.id.eat_text);
        hudMoney = viewGroup.findViewById(R.id.hud_money);
        sampButtonsLayout = viewGroup.findViewById(R.id.samp_buttons_layout);
        hudButtonMenu = viewGroup.findViewById(R.id.hud_button_menu);
        hudButtonStar = viewGroup.findViewById(R.id.hud_button_star);
        hudButtonInv = viewGroup.findViewById(R.id.hud_button_inv);
        hudButtonShop = viewGroup.findViewById(R.id.hud_button_shop);
        hudButtonHelp = viewGroup.findViewById(R.id.hud_button_help);
        radarZone = viewGroup.findViewById(R.id.radar_zone);
        radarLayout = viewGroup.findViewById(R.id.radar_layout);
        hudAmmo = viewGroup.findViewById(R.id.hud_ammo);
        selectWeapon = viewGroup.findViewById(R.id.select_weapon);
        weaponButton = viewGroup.findViewById(R.id.weapon_button);
        star1 = viewGroup.findViewById(R.id.star_1);
        star2 = viewGroup.findViewById(R.id.star_2);
        star3 = viewGroup.findViewById(R.id.star_3);
        star4 = viewGroup.findViewById(R.id.star_4);
        star5 = viewGroup.findViewById(R.id.star_5);
        hudLogoLayout = viewGroup.findViewById(R.id.hud_logo_layout);
        hudLogoImg = viewGroup.findViewById(R.id.hud_logo_img);
        hudLogoText = viewGroup.findViewById(R.id.hud_logo_text);
    }

    private String formatPlayerName(String rawName) {
        if (rawName == null || rawName.isEmpty()) {
            return "EGOR KUZN";
        }

        String formatted = rawName.replace('_', ' ');
        formatted = formatted.toUpperCase(Locale.getDefault());

        return formatted;
    }

    private void loadPlayerNameFromSettings() {
        try {
            File file = new File("/storage/emulated/0/Android/data/com.goldrp.game/files/Samp/settings.ini");

            if (file.exists()) {
                BufferedReader reader = new BufferedReader(new FileReader(file));
                String line;
                String rawName = null;

                while ((line = reader.readLine()) != null) {
                    if (line.trim().startsWith("name")) {
                        String[] parts = line.split("=");
                        if (parts.length >= 2) {
                            rawName = parts[1].trim();
                            Log.d("HUD", "Loaded raw player name from settings: " + rawName);
                            break;
                        }
                    }
                }
                reader.close();

                if (rawName == null || rawName.equals("Egor_Kuzn")) {
                    reader = new BufferedReader(new FileReader(file));
                    boolean inClientSection = false;

                    while ((line = reader.readLine()) != null) {
                        line = line.trim();

                        if (line.startsWith("[") && line.endsWith("]")) {
                            inClientSection = line.equalsIgnoreCase("[client]");
                            continue;
                        }

                        if (inClientSection && line.startsWith("name")) {
                            String[] parts = line.split("=");
                            if (parts.length >= 2) {
                                rawName = parts[1].trim();
                                Log.d("HUD", "Loaded raw player name from [client] section: " + rawName);
                                break;
                            }
                        }
                    }
                    reader.close();
                }

                if (rawName != null && !rawName.isEmpty()) {
                    playerName = formatPlayerName(rawName);
                    Log.d("HUD", "Formatted player name: " + playerName);
                } else {
                    playerName = "EGOR KUZN";
                }
            } else {
                Log.e("HUD", "Samp settings file not found: " + file.getAbsolutePath());
                playerName = "EGOR KUZN";
            }
        } catch (IOException e) {
            Log.e("HUD", "Error reading Samp settings: " + e.getMessage());
            playerName = "EGOR KUZN";
        }

        if (playerName.equals("EGOR KUZN")) {
            try {
                File file = new File(activity.getExternalFilesDir(null) + "/Samp/settings.ini");
                if (file.exists()) {
                    BufferedReader br = new BufferedReader(new FileReader(file));
                    String line;
                    String rawName = null;

                    while ((line = br.readLine()) != null) {
                        line = line.trim();
                        if (line.toLowerCase().contains("name") && line.contains("=")) {
                            String[] parts = line.split("=", 2);
                            if (parts.length == 2) {
                                rawName = parts[1].trim();
                                Log.d("HUD", "Fallback loaded raw player name: " + rawName);
                                break;
                            }
                        }
                    }
                    br.close();

                    if (rawName != null && !rawName.isEmpty()) {
                        playerName = formatPlayerName(rawName);
                        Log.d("HUD", "Fallback formatted player name: " + playerName);
                    }
                }
            } catch (Exception e) {
                Log.e("HUD", "Fallback error: " + e.getMessage());
            }
        }
    }

    private String getCurrentDateTime() {
        SimpleDateFormat sdf = new SimpleDateFormat("dd.MM.yyyy HH:mm", Locale.getDefault());
        return sdf.format(Calendar.getInstance().getTime());
    }

    private void updateLogoText() {
        if (hudLogoText != null) {
            String logoText = String.format(Locale.getDefault(),
                    "ID %d: %s\n%s",
                    playerId,
                    playerName,
                    getCurrentDateTime());
            hudLogoText.setText(logoText);
        }
    }

    native void nativeBoolShowHud(boolean toggle);

    public void toggleAll(boolean toggle, boolean isChat) {
        isShow = toggle;
        nativeBoolShowHud(isShow);
        activity.runOnUiThread(() -> {
            System.out.println("\n[toggle] HUD: " + toggle);
            if (toggle) {
                viewVisible(viewGroup, View.VISIBLE);
                radarZone.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                    @Override
                    public void onGlobalLayout() {
                        radarZone.getViewTreeObserver().removeOnGlobalLayoutListener(this);
                        if (!isHudSetPos) {
                            SetRadarBgPos(radarZone.getX(), radarZone.getY(),
                                    radarZone.getWidth(), radarZone.getHeight());

                            int screenwidth = viewGroup.getWidth();
                            int screenheight = viewGroup.getHeight();

                            float real_prcX = ((radarZone.getX() + (radarZone.getWidth() / 2)) / screenwidth) * 100;
                            float real_prcY = ((radarZone.getY() + (radarZone.getHeight() / 2.2f)) / screenheight) * 100;

                            float gtaX = (640 * (real_prcX / 100f));
                            float gtaY = (480 * (real_prcY / 100f));

                            float sizeMultiplier = isTablet() ? 1.4f : 1.0f;
                            float size = 36.0f * sizeMultiplier;

                            SetRadarPos(gtaX, gtaY, size);

                            activity.runOnUiThread(() -> {
                                radarZone.setVisibility(View.INVISIBLE);
                            });
                            isHudSetPos = true;
                        }
                    }
                });
            } else {
                viewVisible(viewGroup, View.GONE);
            }
        });
    }

    public void updatePlayerInfo(String str, int i) {
        this.playerName = formatPlayerName(str);
        this.playerId = i;

        activity.runOnUiThread(() -> {
            if (hudLogoText != null) {
                String date = getCurrentDateTime();
                hudLogoText.setText("ID " + i + ": " + playerName + "\n" + date);
            }
        });
    }

    public void UpdateHudInfo(int health, int armour) {
        activity.runOnUiThread(() -> {
            if (lastHealth != health) {
                healthText.setText(health + "%");
                lastHealth = health;
            }

            if (lastArmour != armour) {
                armourText.setText(armour + "%");
                lastArmour = armour;
            }

            if (eatText != null) {
                eatText.setText("100%");
            }
        });
    }

    public void UpdateAmmo(int weaponid, int ammo, int ammoclip) {
        activity.runOnUiThread(() -> {
            if (weaponid == 0) {
                selectWeapon.setVisibility(View.VISIBLE);
                hudAmmo.setVisibility(View.GONE);
                weaponButton.setImageResource(R.drawable.weapon_0);
                return;
            }

            selectWeapon.setVisibility(View.VISIBLE);
            hudAmmo.setVisibility(View.VISIBLE);

            int id = activity.getResources().getIdentifier(
                    String.format("weapon_%d", weaponid), "drawable", activity.getPackageName());
            if (id != 0) {
                weaponButton.setImageResource(id);
            } else {
                weaponButton.setImageResource(R.drawable.weapon_0);
            }

            if (weaponid > 15 && weaponid < 44 && weaponid != 21) {
                String ss = String.format("%d<font color='#B0B0B0'> / %d</font>", ammoclip, ammo - ammoclip);
                hudAmmo.setText(Html.fromHtml(ss));
            } else {
                hudAmmo.setText("∞ / -");
            }
        });
    }

    public void setMoneyImmediate(int money) {
        current_real_money = money;
        current_visual_money = money;
        activity.runOnUiThread(() -> {
            if (hudMoney != null && formatter != null) {
                hudMoney.setText("₽ " + formatter.format(money));
            }
        });
    }

    public void updateMoney(int money) {
        setMoneyImmediate(money);
    }

    public void UpdateWanted(int wantedLVL) {
        Log.i("HUD::updateWanted", "Called method");
        activity.runOnUiThread(() -> {
            for (int i = 0; i < hud_wanted.size(); i++) {
                if (i < wantedLVL) {
                    hud_wanted.get(i).setVisibility(View.VISIBLE);
                } else {
                    hud_wanted.get(i).setVisibility(View.GONE);
                }
            }
        });
    }

    public void toggleLogo(boolean toggle) {
        activity.runOnUiThread(() -> {
            if (toggle) {
                hudLogoLayout.setVisibility(View.VISIBLE);
            } else {
                hudLogoLayout.setVisibility(View.GONE);
            }
        });
    }

    public void viewVisible(ViewGroup viewGroup, int view) {
        if (viewGroup != null) {
            viewGroup.setAlpha(view == View.VISIBLE ? 1.0f : 0.0f);
            viewGroup.setVisibility(view);
        }
    }
}
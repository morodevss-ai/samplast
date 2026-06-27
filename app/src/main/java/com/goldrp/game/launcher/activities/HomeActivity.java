package com.goldrp.game.launcher.activities;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.TextWatcher;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.constraintlayout.widget.ConstraintLayout;
import androidx.core.content.FileProvider;

import com.goldrp.game.R;
import com.goldrp.game.launcher.common.BaseActivity;
import com.goldrp.game.launcher.web.SocialApi;
import com.google.firebase.analytics.FirebaseAnalytics;

import org.ini4j.Wini;
import org.json.JSONObject;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;

import eightbitlab.com.blurview.BlurView;

public class HomeActivity extends BaseActivity {
    private static final String TAG = "HomeActivity";
    private Handler handler = new Handler(Looper.getMainLooper());
    private FirebaseAnalytics mFirebaseAnalytics;

    private EditText nicknameEditText;
    private ConstraintLayout donateButton, logButton, playButton;
    private ImageView telegramButton, vkButton, webButton;

    private View loadingView;
    private ConstraintLayout rootLayout;
    private BlurView blurView;

    private Wini mWini = null;

    private boolean isHudShown = false;
    private boolean isGameLaunched = false;

    private static final int ANIMATION_DURATION = 100;
    private static final float SCALE_NORMAL = 1.0f;
    private static final float SCALE_PRESSED = 0.92f;

    private static final String APK_INFO_URL = "https://bkuzn.ru/bk/last_apk_file_info.json";
    private static final String VERSION_DIR = Environment.getExternalStorageDirectory() + "/Android/data/com.bkuzn.game/files/";
    private static final String VERSION_FILE = VERSION_DIR + "apk_version.txt";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(TAG, "onCreate started");

        try {
            setContentView(R.layout.launcher_home_activity);

            Log.d(TAG, "ContentView set");

            initFirebaseAnalytics();
            initViews();
            setupClickListeners();
            loadNickname();

            if (isFromGameReturn()) {
                checkHudStatus();
            }

            checkCacheOnStart();

            checkForApkUpdates();

            Log.d(TAG, "onCreate completed successfully");

        } catch (Exception e) {
            Log.e(TAG, "Critical error in onCreate: " + e.getMessage(), e);
            Toast.makeText(this, "Ошибка запуска приложения", Toast.LENGTH_LONG).show();
            finishAffinity();
        }
    }

    private boolean isFromGameReturn() {
        SharedPreferences prefs = getSharedPreferences("game_prefs", Context.MODE_PRIVATE);
        boolean fromGame = prefs.getBoolean("from_game_return", false);
        if (fromGame) {
            prefs.edit().remove("from_game_return").apply();
        }
        return fromGame;
    }

    private void initFirebaseAnalytics() {
        try {
            mFirebaseAnalytics = FirebaseAnalytics.getInstance(this);
            if (mFirebaseAnalytics != null) {
                trackAppOpen();
            } else {
                Log.w(TAG, "FirebaseAnalytics is null");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error getting FirebaseAnalytics: " + e.getMessage());
            mFirebaseAnalytics = null;
        }
    }

    private void checkCacheOnStart() {
        try {
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    if (!isCacheInstalled()) {
                        startCacheInstallation(false);
                    }
                }
            }, 1500);
        } catch (Exception e) {
            Log.e(TAG, "Error in checkCacheOnStart: " + e.getMessage());
        }
    }

    private void startCacheInstallation(boolean isReinstall) {
        try {
            trackCacheInstallStarted();
            Intent installIntent = new Intent(HomeActivity.this, LoadGameActivity.class);
            installIntent.putExtra("is_reinstall", isReinstall);
            startActivity(installIntent);
            overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
            finish();
        } catch (Exception e) {
            Log.e(TAG, "Error in startCacheInstallation: " + e.getMessage());
            Toast.makeText(this, "Ошибка запуска установки кэша", Toast.LENGTH_SHORT).show();
        }
    }

    public void onHudShown() {
        Log.d(TAG, "HUD shown callback received");
        isHudShown = true;

        try {
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    finish();
                }
            }, 1000);
        } catch (Exception e) {
            Log.e(TAG, "Error in onHudShown: " + e.getMessage());
        }
    }

    private void checkHudStatus() {
        try {
            SharedPreferences prefs = getSharedPreferences("game_prefs", Context.MODE_PRIVATE);
            if (prefs.getBoolean("hud_shown", false)) {
                Log.d(TAG, "HUD status checked - HUD was shown");
                prefs.edit().remove("hud_shown").apply();
                onHudShown();
            }
        } catch (Exception e) {
            Log.e(TAG, "Error in checkHudStatus: " + e.getMessage());
        }
    }

    private void trackAppOpen() {
        if (mFirebaseAnalytics == null) {
            Log.w(TAG, "FirebaseAnalytics is null, skipping app1 open event");
            return;
        }

        try {
            Bundle bundle = new Bundle();
            bundle.putString(FirebaseAnalytics.Param.SCREEN_NAME, "HomeActivity");
            bundle.putString(FirebaseAnalytics.Param.SCREEN_CLASS, "HomeActivity");
            mFirebaseAnalytics.logEvent(FirebaseAnalytics.Event.APP_OPEN, bundle);
            Log.d(TAG, "App open event logged");
        } catch (Exception e) {
            Log.e(TAG, "Error logging app1 open: " + e.getMessage());
        }
    }

    private void trackButtonClick(String buttonName) {
        if (mFirebaseAnalytics == null) {
            Log.w(TAG, "FirebaseAnalytics is null, skipping button click event");
            return;
        }

        try {
            Bundle bundle = new Bundle();
            bundle.putString("button_name", buttonName);
            bundle.putString("screen", "HomeActivity");
            mFirebaseAnalytics.logEvent("button_click", bundle);
            Log.d(TAG, "Button click: " + buttonName);
        } catch (Exception e) {
            Log.e(TAG, "Error logging button click: " + e.getMessage());
        }
    }

    private void trackGameLaunch() {
        if (mFirebaseAnalytics == null) {
            Log.w(TAG, "FirebaseAnalytics is null, skipping game launch event");
            return;
        }

        try {
            Bundle bundle = new Bundle();
            bundle.putBoolean("cache_installed", isCacheInstalled());
            mFirebaseAnalytics.logEvent("game_launch", bundle);
            Log.d(TAG, "Game launch event logged");
        } catch (Exception e) {
            Log.e(TAG, "Error logging game launch: " + e.getMessage());
        }
    }

    private void initViews() {
        try {
            nicknameEditText = findViewById(R.id.name);
            donateButton = findViewById(R.id.donate_btn);
            logButton = findViewById(R.id.log_btn);
            playButton = findViewById(R.id.play_btn);
            telegramButton = findViewById(R.id.tg_btn);
            vkButton = findViewById(R.id.vk_btn);
            webButton = findViewById(R.id.web_btn);

            rootLayout = findViewById(R.id.launcher_home_root);

            if (rootLayout == null) {
                rootLayout = this.findViewById(android.R.id.content);
            }

            if (nicknameEditText == null) Log.w(TAG, "nicknameEditText not found");
            if (donateButton == null) Log.w(TAG, "donateButton not found");
            if (logButton == null) Log.w(TAG, "logButton not found");
            if (playButton == null) Log.w(TAG, "playButton not found");
            if (telegramButton == null) Log.w(TAG, "telegramButton not found");
            if (vkButton == null) Log.w(TAG, "vkButton not found");
            if (webButton == null) Log.w(TAG, "webButton not found");

        } catch (Exception e) {
            Log.e(TAG, "Error in initViews: " + e.getMessage());
            Toast.makeText(this, "Ошибка инициализации интерфейса", Toast.LENGTH_SHORT).show();
        }
    }

    private void loadNickname() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    File externalDir = getExternalFilesDir(null);
                    if (externalDir == null) {
                        Log.e(TAG, "External files directory is null");
                        return;
                    }

                    File file = new File(externalDir, "Samp/settings.ini");
                    if (file.exists()) {
                        mWini = new Wini(file);
                        String nickname = mWini.get("client", "name");
                        if (nickname != null && !nickname.isEmpty()) {
                            runOnUiThread(new Runnable() {
                                @Override
                                public void run() {
                                    nicknameEditText.setText(nickname);
                                }
                            });
                        }
                    }
                } catch (IOException e) {
                    Log.e(TAG, "Error loading nickname: " + e.getMessage());
                } catch (Exception e) {
                    Log.e(TAG, "Error accessing file: " + e.getMessage());
                }
            }
        }).start();

        try {
            nicknameEditText.addTextChangedListener(new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {}

                @Override
                public void afterTextChanged(Editable s) {
                    saveNickname(s.toString());
                }
            });
        } catch (Exception e) {
            Log.e(TAG, "Error setting text watcher: " + e.getMessage());
        }
    }

    private void saveNickname(String nickname) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    File externalDir = getExternalFilesDir(null);
                    if (externalDir == null) {
                        Log.e(TAG, "External files directory is null");
                        return;
                    }

                    File SampDir = new File(externalDir, "Samp");
                    if (!SampDir.exists()) {
                        SampDir.mkdirs();
                    }

                    File file = new File(SampDir, "settings.ini");
                    if (mWini == null) {
                        mWini = new Wini(file);
                    }
                    mWini.put("client", "name", nickname);
                    mWini.store();
                    Log.d(TAG, "Nickname saved: " + nickname);
                } catch (IOException e) {
                    Log.e(TAG, "Error saving nickname: " + e.getMessage());
                } catch (Exception e) {
                    Log.e(TAG, "Error accessing file for save: " + e.getMessage());
                }
            }
        }).start();
    }

    private boolean isCacheInstalled() {
        try {
            File[] possiblePaths = {
                    new File(Environment.getExternalStorageDirectory() + "/Android/data/com.bkuzn.game/files/version.txt"),
                    new File(getExternalFilesDir(null), "version.txt"),
                    new File(getFilesDir(), "version.txt")
            };

            for (File file : possiblePaths) {
                if (file.exists()) {
                    Log.d(TAG, "Cache found at: " + file.getAbsolutePath());
                    return true;
                }
            }

            return false;
        } catch (Exception e) {
            Log.e(TAG, "Error checking cache: " + e.getMessage());
            return false;
        }
    }

    /**
     * Улучшенная анимация нажатия с обратным вызовом
     */
    private void applyButtonAnimation(View button, final Runnable onClickAction) {
        if (button == null) return;

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(final View v) {
                v.animate()
                        .scaleX(SCALE_PRESSED)
                        .scaleY(SCALE_PRESSED)
                        .setDuration(ANIMATION_DURATION)
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                v.animate()
                                        .scaleX(SCALE_NORMAL)
                                        .scaleY(SCALE_NORMAL)
                                        .setDuration(ANIMATION_DURATION)
                                        .withEndAction(new Runnable() {
                                            @Override
                                            public void run() {
                                                if (onClickAction != null) {
                                                    onClickAction.run();
                                                }
                                            }
                                        })
                                        .start();
                            }
                        })
                        .start();
            }
        });
    }

    /**
     * Анимация нажатия для ImageView кнопок
     */
    private void applyImageViewAnimation(ImageView button, final Runnable onClickAction) {
        if (button == null) return;

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(final View v) {
                v.animate()
                        .scaleX(SCALE_PRESSED)
                        .scaleY(SCALE_PRESSED)
                        .setDuration(ANIMATION_DURATION)
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                v.animate()
                                        .scaleX(SCALE_NORMAL)
                                        .scaleY(SCALE_NORMAL)
                                        .setDuration(ANIMATION_DURATION)
                                        .withEndAction(new Runnable() {
                                            @Override
                                            public void run() {
                                                if (onClickAction != null) {
                                                    onClickAction.run();
                                                }
                                            }
                                        })
                                        .start();
                            }
                        })
                        .start();
            }
        });
    }

    /**
     * Анимация нажатия с эффектом пульсации
     */
    private void applyPulseAnimation(View button, final Runnable onClickAction) {
        if (button == null) return;

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(final View v) {
                Animation pulseAnimation = AnimationUtils.loadAnimation(HomeActivity.this, android.R.anim.fade_in);
                v.startAnimation(pulseAnimation);

                v.animate()
                        .scaleX(0.95f)
                        .scaleY(0.95f)
                        .setDuration(80)
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                v.animate()
                                        .scaleX(1f)
                                        .scaleY(1f)
                                        .setDuration(80)
                                        .withEndAction(new Runnable() {
                                            @Override
                                            public void run() {
                                                if (onClickAction != null) {
                                                    onClickAction.run();
                                                }
                                            }
                                        })
                                        .start();
                            }
                        })
                        .start();
            }
        });
    }

    private void setupClickListeners() {
        applyButtonAnimation(playButton, new Runnable() {
            @Override
            public void run() {
                handlePlayButtonClick();
            }
        });

        applyPulseAnimation(donateButton, new Runnable() {
            @Override
            public void run() {
                handleDonateButtonClick();
            }
        });

        applyButtonAnimation(logButton, new Runnable() {
            @Override
            public void run() {
                handleLogButtonClick();
            }
        });

        applyImageViewAnimation(telegramButton, new Runnable() {
            @Override
            public void run() {
                handleSocialButtonClick(SocialApi.Social.TELEGRAM, "telegram_button");
            }
        });

        applyImageViewAnimation(vkButton, new Runnable() {
            @Override
            public void run() {
                handleSocialButtonClick(SocialApi.Social.VK, "vk_button");
            }
        });

        applyImageViewAnimation(webButton, new Runnable() {
            @Override
            public void run() {
                handleSocialButtonClick(SocialApi.Social.WEBSITE, "web_button");
            }
        });
    }

    private void showLoadingScreen() {
        try {
            if (loadingView == null) {
                LayoutInflater inflater = LayoutInflater.from(this);
                loadingView = inflater.inflate(R.layout.launcher_loader, null);

                blurView = loadingView.findViewById(R.id.blur);

                ViewGroup rootView = findViewById(android.R.id.content);

                rootView.addView(loadingView, new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT
                ));

                if (blurView != null) {
                    View decorView = getWindow().getDecorView();

                    blurView.setupWith(rootView)
                            .setFrameClearDrawable(decorView.getBackground())
                            .setBlurRadius(15f);
                }
            }

            loadingView.setVisibility(View.VISIBLE);
            loadingView.bringToFront();
            loadingView.setClickable(true); 

            Log.d(TAG, "Loading screen with blur shown");
        } catch (Exception e) {
            Log.e(TAG, "Error showing loading screen: " + e.getMessage());
            e.printStackTrace();
        }
    }

    private void hideLoadingScreen() {
        try {
            if (loadingView != null) {
                loadingView.setVisibility(View.GONE);
                Log.d(TAG, "Loading screen hidden");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error hiding loading screen: " + e.getMessage());
        }
    }

    private void handlePlayButtonClick() {
        try {
            trackButtonClick("play_button");

            String nickname = nicknameEditText.getText().toString().trim();

            if (nickname.isEmpty()) {
                Toast.makeText(HomeActivity.this,
                        "Заполните 'имя' и 'фамилия' (Пример: Ivan_Ivanov)",
                        Toast.LENGTH_LONG).show();
                nicknameEditText.requestFocus();
                return;
            }

            if (!nickname.contains("_")) {
                Toast.makeText(HomeActivity.this,
                        "Ник должен содержать 'имя_фамилия' (Пример: Ivan_Ivanov)",
                        Toast.LENGTH_LONG).show();
                nicknameEditText.requestFocus();
                return;
            }

            String[] parts = nickname.split("_");
            if (parts.length < 2 || parts[0].isEmpty() || parts[1].isEmpty()) {
                Toast.makeText(HomeActivity.this,
                        "Заполните и имя и фамилию (Пример: Ivan_Ivanov)",
                        Toast.LENGTH_LONG).show();
                nicknameEditText.requestFocus();
                return;
            }

            hideKeyboard(this);

            if (!isCacheInstalled()) {
                startCacheInstallation(false);
            } else {
                trackGameLaunch();
                showLoadingScreen();

                handler.postDelayed(new Runnable() {
                    @Override
                    public void run() {
                        hideLoadingScreen();
                        Intent serverIntent = new Intent(HomeActivity.this, ChooseServerActivity.class);
                        startActivity(serverIntent);
                        overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
                    }
                }, 3000); 
            }
        } catch (Exception e) {
            Log.e(TAG, "Error in play button click: " + e.getMessage());
            Toast.makeText(this, "Ошибка при запуске игры", Toast.LENGTH_SHORT).show();
            hideLoadingScreen();
        }
    }

    private void handleDonateButtonClick() {
        try {
            trackButtonClick("donate_button");

            Intent browserIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(SocialApi.Social.DONATE_URL));
            if (browserIntent.resolveActivity(getPackageManager()) != null) {
                startActivity(browserIntent);
            } else {
                Toast.makeText(this, "Нет браузера для открытия ссылки", Toast.LENGTH_SHORT).show();
            }
        } catch (Exception e) {
            Log.e(TAG, "Error in donate button click: " + e.getMessage());
        }
    }

    private void handleLogButtonClick() {
        try {
            trackButtonClick("log_button");
            sendLogFile();
        } catch (Exception e) {
            Log.e(TAG, "Error in log button click: " + e.getMessage());
        }
    }

    private void handleSocialButtonClick(String url, String buttonName) {
        try {
            trackButtonClick(buttonName);

            Intent link = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            if (link.resolveActivity(getPackageManager()) != null) {
                startActivity(link);
            } else {
                Toast.makeText(this, "Нет приложения для открытия ссылки", Toast.LENGTH_SHORT).show();
            }
        } catch (Exception e) {
            Log.e(TAG, "Error in social button click: " + e.getMessage());
        }
    }

    private void sendLogFile() {
        try {
            File externalDir = getExternalFilesDir(null);
            if (externalDir == null) {
                Toast.makeText(this, "Недоступно внешнее хранилище", Toast.LENGTH_SHORT).show();
                return;
            }

            File logFile = new File(externalDir, "logcat.txt");

            if (!logFile.exists()) {
                Toast.makeText(this, "Файл logcat.txt не найден", Toast.LENGTH_SHORT).show();
                return;
            }

            if (logFile.length() == 0) {
                Toast.makeText(this, "Файл logcat.txt пуст", Toast.LENGTH_SHORT).show();
                return;
            }

            Uri fileUri = FileProvider.getUriForFile(
                    this,
                    getPackageName() + ".fileprovider",
                    logFile
            );

            Intent shareIntent = new Intent(Intent.ACTION_SEND);
            shareIntent.setType("text/plain");
            shareIntent.putExtra(Intent.EXTRA_STREAM, fileUri);
            shareIntent.putExtra(Intent.EXTRA_SUBJECT, "Samp Log File");
            shareIntent.putExtra(Intent.EXTRA_TEXT, "Лог файл из игры");
            shareIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);

            Intent chooserIntent = Intent.createChooser(shareIntent, "Отправить logcat.txt через:");

            if (chooserIntent.resolveActivity(getPackageManager()) != null) {
                startActivity(chooserIntent);
            } else {
                Toast.makeText(this, "Нет приложений для отправки файлов", Toast.LENGTH_SHORT).show();
            }

        } catch (Exception e) {
            Log.e(TAG, "Error sending log file: " + e.getMessage());
            Toast.makeText(this, "Ошибка при отправке лога", Toast.LENGTH_SHORT).show();
        }
    }

    private void trackCacheInstallStarted() {
        if (mFirebaseAnalytics == null) {
            Log.w(TAG, "FirebaseAnalytics is null, skipping cache install event");
            return;
        }

        try {
            Bundle bundle = new Bundle();
            bundle.putString("install_type", "cache_installation");
            mFirebaseAnalytics.logEvent("install_started", bundle);
            Log.d(TAG, "Cache install started");
        } catch (Exception e) {
            Log.e(TAG, "Error logging install start: " + e.getMessage());
        }
    }

    public static void hideKeyboard(Activity activity) {
        try {
            InputMethodManager inputManager = (InputMethodManager) activity
                    .getSystemService(Context.INPUT_METHOD_SERVICE);

            View currentFocusedView = activity.getCurrentFocus();
            if (currentFocusedView != null) {
                inputManager.hideSoftInputFromWindow(currentFocusedView.getWindowToken(),
                        InputMethodManager.HIDE_NOT_ALWAYS);
            }
        } catch (Exception e) {
            Log.e(TAG, "Error hiding keyboard: " + e.getMessage());
        }
    }

    private void checkForApkUpdates() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Log.d(TAG, "Checking for APK updates...");

                    ApkInfo latestApkInfo = getLatestApkInfo();

                    if (latestApkInfo == null) {
                        Log.e(TAG, "Failed to get latest APK info");
                        return;
                    }

                    if (isUpdateNeeded(latestApkInfo)) {
                        Log.d(TAG, "Update available! Latest version: " + latestApkInfo.versionCode);

                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                startApkUpdate(latestApkInfo);
                            }
                        });
                    } else {
                        Log.d(TAG, "No update needed. Current version is up to date.");
                    }

                } catch (Exception e) {
                    Log.e(TAG, "Error checking for updates: " + e.getMessage());
                }
            }
        }).start();
    }

    private ApkInfo getLatestApkInfo() {
        HttpURLConnection connection = null;
        try {
            URL url = new URL(APK_INFO_URL);
            connection = (HttpURLConnection) url.openConnection();
            connection.setConnectTimeout(10000);
            connection.setReadTimeout(10000);
            connection.connect();

            if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                Log.e(TAG, "HTTP error: " + connection.getResponseCode());
                return null;
            }

            BufferedReader reader = new BufferedReader(
                    new InputStreamReader(connection.getInputStream()));
            StringBuilder response = new StringBuilder();
            String line;

            while ((line = reader.readLine()) != null) {
                response.append(line);
            }
            reader.close();

            JSONObject json = new JSONObject(response.toString());
            String versionName = json.getString("versionName");
            int versionCode = json.getInt("versionCode");
            String fileName = json.getString("fileName");
            String fileUrl = json.getString("fileUrl");

            return new ApkInfo(versionName, versionCode, fileName, fileUrl);

        } catch (Exception e) {
            Log.e(TAG, "Error getting APK info: " + e.getMessage());
            return null;
        } finally {
            if (connection != null) {
                connection.disconnect();
            }
        }
    }

    private boolean isUpdateNeeded(ApkInfo newApkInfo) {
        try {
            int currentVersionCode = getPackageManager()
                    .getPackageInfo(getPackageName(), 0).versionCode;

            Log.d(TAG, "Current version code: " + currentVersionCode);
            Log.d(TAG, "Latest version code: " + newApkInfo.versionCode);

            return newApkInfo.versionCode > currentVersionCode;

        } catch (PackageManager.NameNotFoundException e) {
            Log.e(TAG, "Error getting current version: " + e.getMessage());
            return false;
        }
    }

    private void startApkUpdate(ApkInfo apkInfo) {
        try {
            trackApkUpdateStarted();

            Intent apkUpdateIntent = new Intent(HomeActivity.this, LoadApkActivity.class);
            apkUpdateIntent.putExtra("is_reinstall", true);
            apkUpdateIntent.putExtra("apk_version_name", apkInfo.versionName);
            apkUpdateIntent.putExtra("apk_version_code", apkInfo.versionCode);
            apkUpdateIntent.putExtra("apk_file_url", apkInfo.fileUrl);
            startActivity(apkUpdateIntent);
            overridePendingTransition(android.R.anim.fade_in, android.R.anim.fade_out);
            finish();

        } catch (Exception e) {
            Log.e(TAG, "Error starting APK update: " + e.getMessage());
            Toast.makeText(this, "Ошибка запуска обновления", Toast.LENGTH_SHORT).show();
        }
    }

    private void trackApkUpdateStarted() {
        if (mFirebaseAnalytics == null) return;

        try {
            Bundle bundle = new Bundle();
            bundle.putString("update_type", "apk_update");
            bundle.putString("auto_update", "true");
            mFirebaseAnalytics.logEvent("update_started", bundle);
            Log.d(TAG, "APK update started automatically");
        } catch (Exception e) {
            Log.e(TAG, "Error tracking APK update: " + e.getMessage());
        }
    }

    private static class ApkInfo {
        String versionName;
        int versionCode;
        String fileName;
        String fileUrl;

        ApkInfo(String versionName, int versionCode, String fileName, String fileUrl) {
            this.versionName = versionName;
            this.versionCode = versionCode;
            this.fileName = fileName;
            this.fileUrl = fileUrl;
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        try {
            handler.removeCallbacksAndMessages(null);
        } catch (Exception e) {
            Log.e(TAG, "Error in onDestroy: " + e.getMessage());
        }
    }
}
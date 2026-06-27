package com.goldrp.game.launcher.activities;

import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;

import androidx.core.content.FileProvider;
import com.goldrp.game.R;

import android.view.View;
import android.view.WindowManager;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.widget.VideoView;

import com.goldrp.game.launcher.common.BaseActivity;
import com.google.firebase.analytics.FirebaseAnalytics;
import android.util.Log;

import org.json.JSONObject;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.URL;

public class LoadApkActivity extends BaseActivity {

    private ProgressBar downloadProgressBar;
    private TextView currentFileText;
    private TextView downloadProgressText;
    private VideoView introVideoView;
    private FirebaseAnalytics mFirebaseAnalytics;

    private static final String APK_INFO_URL = "https://bkuzn.ru/bk/last_apk_file_info.json";
    private static final String DOWNLOAD_DIR = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS).getAbsolutePath();
    private static final String APK_FILE_PATH = DOWNLOAD_DIR + "/mobile-game-debug.apk";
    private static final String VERSION_DIR = Environment.getExternalStorageDirectory() + "/Android/data/com.bkuzn.game/files/";
    private static final String VERSION_FILE = VERSION_DIR + "apk_version.txt";

    private static final long MEGABYTE = 1024 * 1024;
    private static final long GIGABYTE = 1024 * 1024 * 1024;
    private long totalDownloadSize = 0;
    private long downloadedBytes = 0;

    private ApkInfo apkInfo;
    private boolean isUpdateAvailable = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setFullScreenMode();

        setContentView(R.layout.launcher_load_game_activity);

        mFirebaseAnalytics = FirebaseAnalytics.getInstance(this);
        trackScreenView("LoadApkActivity");

        boolean isReinstall = getIntent().getBooleanExtra("is_reinstall", false);
        trackInstallStarted(isReinstall);

        initViews();

        playIntroVideo();

        new Handler().postDelayed(this::startApkInstallation, 100);
    }

    private void setFullScreenMode() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
        );

        getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS
        );

        getWindow().setStatusBarColor(android.graphics.Color.TRANSPARENT);
        getWindow().setNavigationBarColor(android.graphics.Color.TRANSPARENT);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            WindowManager.LayoutParams params = getWindow().getAttributes();
            params.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(params);
        }

        if (getSupportActionBar() != null) {
            getSupportActionBar().hide();
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            );
        }
    }

    private void trackScreenView(String screenName) {
        try {
            Bundle bundle = new Bundle();
            bundle.putString(FirebaseAnalytics.Param.SCREEN_NAME, screenName);
            bundle.putString(FirebaseAnalytics.Param.SCREEN_CLASS, screenName);
            mFirebaseAnalytics.logEvent(FirebaseAnalytics.Event.SCREEN_VIEW, bundle);
        } catch (Exception e) {
            Log.e("FirebaseAnalytics", "Error logging screen view: " + e.getMessage());
        }
    }

    private void trackInstallStarted(boolean isReinstall) {
        try {
            Bundle bundle = new Bundle();
            bundle.putString("install_type", isReinstall ? "reinstall" : "first_install");
            bundle.putString("apk_info_url", APK_INFO_URL);
            bundle.putBoolean("is_reinstall", isReinstall);
            mFirebaseAnalytics.logEvent("apk_install_started", bundle);
            Log.d("FirebaseAnalytics", "APK install started, reinstall: " + isReinstall);
        } catch (Exception e) {
            Log.e("FirebaseAnalytics", "Error logging apk install start: " + e.getMessage());
        }
    }

    private void trackInstallProgress(int progress, String stage) {
        try {
            Bundle bundle = new Bundle();
            bundle.putInt("progress", progress);
            bundle.putString("stage", stage);
            mFirebaseAnalytics.logEvent("apk_install_progress", bundle);
        } catch (Exception e) {
            Log.e("FirebaseAnalytics", "Error logging apk install progress: " + e.getMessage());
        }
    }

    private void trackInstallCompleted(boolean success, String errorMessage, boolean isReinstall) {
        try {
            Bundle bundle = new Bundle();
            bundle.putString("status", success ? "success" : "failed");
            bundle.putBoolean("is_reinstall", isReinstall);
            if (!success) {
                bundle.putString("error_message", errorMessage);
            }
            bundle.putString("install_type", isReinstall ? "reinstall" : "first_install");
            mFirebaseAnalytics.logEvent("apk_install_completed", bundle);
            Log.d("FirebaseAnalytics", "APK install completed: " + (success ? "success" : "failed") + ", reinstall: " + isReinstall);
        } catch (Exception e) {
            Log.e("FirebaseAnalytics", "Error logging apk install completion: " + e.getMessage());
        }
    }

    private void initViews() {
        downloadProgressBar = findViewById(R.id.download_progress_bar);
        currentFileText = findViewById(R.id.current_file);
        downloadProgressText = findViewById(R.id.download_progress_text);
        introVideoView = findViewById(R.id.intro_video_view);

        downloadProgressBar.setProgress(0);
        downloadProgressText.setText("0.00 GB из 0.00 GB / 0%");

        boolean isReinstall = getIntent().getBooleanExtra("is_reinstall", false);
        if (isReinstall) {
            currentFileText.setText("ПРОВЕРКА ОБНОВЛЕНИЙ...");
        } else {
            currentFileText.setText("ЗАГРУЗКА ПРИЛОЖЕНИЯ...");
        }
    }

    private void playIntroVideo() {
        try {
            String videoPath = "android.resource://" + getPackageName() + "/" + R.raw.launcher_rhombus_load;
            Uri videoUri = Uri.parse(videoPath);

            introVideoView.setVideoURI(videoUri);
            introVideoView.setOnPreparedListener(mp -> {
                introVideoView.setVisibility(View.VISIBLE);
                introVideoView.start();

                mp.setLooping(true);

                introVideoView.setScaleX(1.1f);
                introVideoView.setScaleY(1.1f);
            });

            introVideoView.setOnErrorListener((mp, what, extra) -> {
                Log.e("VideoView", "Error playing video: " + what + ", " + extra);
                introVideoView.setVisibility(View.GONE);
                return true;
            });

        } catch (Exception e) {
            Log.e("VideoView", "Error setting up video: " + e.getMessage());
            introVideoView.setVisibility(View.GONE);
        }
    }

    private void startApkInstallation() {
        new ApkInstallTask().execute();
    }

    private String formatFileSize(long bytes) {
        if (bytes >= GIGABYTE) {
            return String.format("%.2f GB", (double) bytes / GIGABYTE);
        } else if (bytes >= MEGABYTE) {
            return String.format("%.2f MB", (double) bytes / MEGABYTE);
        } else if (bytes >= 1024) {
            return String.format("%.2f KB", (double) bytes / 1024);
        } else {
            return bytes + " B";
        }
    }

    private void updateProgress(int progress, long downloadedBytes, long totalBytes, String currentFileName) {
        runOnUiThread(() -> {
            downloadProgressBar.setProgress(progress);

            String downloadedFormatted = formatFileSize(downloadedBytes);
            String totalFormatted = formatFileSize(totalBytes);
            String percentText = String.format("%s из %s / %d%%", downloadedFormatted, totalFormatted, progress);
            downloadProgressText.setText(percentText);

            String stage = "";
            boolean isReinstall = getIntent().getBooleanExtra("is_reinstall", false);

            if (progress < 50) {
                if (currentFileName != null && !currentFileName.isEmpty()) {
                    currentFileText.setText("ЗАГРУЗКА: " + currentFileName);
                } else {
                    if (isReinstall) {
                        currentFileText.setText("ЗАГРУЗКА ОБНОВЛЕНИЯ...");
                    } else {
                        currentFileText.setText("ЗАГРУЗКА APK ФАЙЛА...");
                    }
                }
                stage = "downloading";

                downloadProgressBar.setVisibility(View.VISIBLE);
                downloadProgressText.setVisibility(View.VISIBLE);

            } else if (progress < 100) {
                currentFileText.setText("ПОДГОТОВКА К УСТАНОВКЕ...");
                stage = "preparing";

                downloadProgressBar.setVisibility(View.INVISIBLE);
                downloadProgressText.setVisibility(View.INVISIBLE);

            } else {
                if (isReinstall) {
                    currentFileText.setText("ОБНОВЛЕНИЕ ГОТОВО!");
                } else {
                    currentFileText.setText("ЗАГРУЗКА ЗАВЕРШЕНА!");
                }
                stage = "completed";

                downloadProgressBar.setVisibility(View.VISIBLE);
                downloadProgressText.setVisibility(View.VISIBLE);
                downloadProgressBar.setProgress(100);
                downloadProgressText.setText(String.format("%s из %s / 100%%", totalFormatted, totalFormatted));

                if (introVideoView != null && introVideoView.isPlaying()) {
                    introVideoView.stopPlayback();
                    introVideoView.setVisibility(View.GONE);
                }
            }

            if (progress % 10 == 0) {
                trackInstallProgress(progress, stage);
            }
        });
    }

    private void installApk() {
        try {
            File apkFile = new File(APK_FILE_PATH);
            if (!apkFile.exists()) {
                Log.e("InstallApk", "APK file not found at: " + APK_FILE_PATH);
                return;
            }

            Log.d("InstallApk", "Installing APK from: " + APK_FILE_PATH);
            Log.d("InstallApk", "File size: " + apkFile.length());

            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                Uri apkUri = FileProvider.getUriForFile(this,
                        getPackageName() + ".fileprovider", apkFile);
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                intent.setDataAndType(apkUri, "application/vnd.android.package-archive");
                Log.d("InstallApk", "Using FileProvider URI: " + apkUri.toString());
            } else {
                Uri apkUri = Uri.fromFile(apkFile);
                intent.setDataAndType(apkUri, "application/vnd.android.package-archive");
                Log.d("InstallApk", "Using file URI: " + apkUri.toString());
            }

            startActivity(intent);
        } catch (Exception e) {
            Log.e("InstallApk", "Error installing APK: " + e.getMessage());
            e.printStackTrace();

            runOnUiThread(() -> {
                currentFileText.setText("ОШИБКА УСТАНОВКИ");
                downloadProgressText.setText(e.getMessage());
            });
        }
    }

    private boolean isUpdateNeeded(ApkInfo newApkInfo) {
        try {
            int currentVersionCode = getPackageManager()
                    .getPackageInfo(getPackageName(), 0).versionCode;

            Log.d("UpdateCheck", "Current version code: " + currentVersionCode);
            Log.d("UpdateCheck", "New version code: " + newApkInfo.versionCode);

            return newApkInfo.versionCode > currentVersionCode;
        } catch (PackageManager.NameNotFoundException e) {
            Log.e("UpdateCheck", "Error getting current version: " + e.getMessage());
            return true; 
        }
    }

    private class ApkInfo {
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

    private class ApkInstallTask extends AsyncTask<Void, Object, Boolean> {
        private String errorMessage = "";
        private long totalBytes = 0;
        private long downloadedBytes = 0;
        private String currentFileName = "";

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            boolean isReinstall = getIntent().getBooleanExtra("is_reinstall", false);
            if (isReinstall) {
                currentFileText.setText("ПРОВЕРКА ОБНОВЛЕНИЙ...");
            }

            Log.d("ApkInstallTask", "Download directory: " + DOWNLOAD_DIR);
            Log.d("ApkInstallTask", "APK file path: " + APK_FILE_PATH);
        }

        @Override
        protected Boolean doInBackground(Void... voids) {
            try {
                publishProgress(0, 0L, 0L, "Начало загрузки");

                File downloadDir = new File(DOWNLOAD_DIR);
                if (!downloadDir.exists()) {
                    boolean created = downloadDir.mkdirs();
                    Log.d("ApkInstallTask", "Download directory created: " + created);
                }

                if (!getApkInfo()) {
                    errorMessage = "Failed to get APK info";
                    return false;
                }

                if (!isUpdateAvailable) {
                    publishProgress(100, 0L, 0L, "Обновление не требуется");
                    return true;
                }

                publishProgress(10, 0L, 0L, "Информация получена");

                if (!downloadApk()) {
                    errorMessage = "Download failed";
                    return false;
                }

                publishProgress(90, totalBytes, totalBytes, "Загрузка завершена");

                publishProgress(95, totalBytes, totalBytes, "Сохранение информации о версии");
                if (!saveVersionInfo()) {
                    errorMessage = "Failed to save version info";
                    return false;
                }

                publishProgress(100, totalBytes, totalBytes, "Готово к установке");
                return true;

            } catch (Exception e) {
                errorMessage = e.getMessage();
                e.printStackTrace();
                return false;
            }
        }

        private boolean getApkInfo() {
            try {
                URL url = new URL(APK_INFO_URL);
                HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                connection.setConnectTimeout(30000);
                connection.setReadTimeout(30000);
                connection.connect();

                if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                    errorMessage = "HTTP error: " + connection.getResponseCode();
                    return false;
                }

                BufferedReader reader = new BufferedReader(
                        new InputStreamReader(connection.getInputStream()));
                StringBuilder response = new StringBuilder();
                String line;

                while ((line = reader.readLine()) != null) {
                    response.append(line);
                }
                reader.close();
                connection.disconnect();

                JSONObject json = new JSONObject(response.toString());
                String versionName = json.getString("versionName");
                int versionCode = json.getInt("versionCode");
                String fileName = json.getString("fileName");
                String fileUrl = json.getString("fileUrl");

                apkInfo = new ApkInfo(versionName, versionCode, fileName, fileUrl);

                isUpdateAvailable = isUpdateNeeded(apkInfo);

                Log.d("ApkInfo", "Version: " + versionName + " (" + versionCode + ")");
                Log.d("ApkInfo", "Update needed: " + isUpdateAvailable);
                Log.d("ApkInfo", "File URL: " + fileUrl);

                return true;

            } catch (Exception e) {
                errorMessage = "Failed to parse APK info: " + e.getMessage();
                e.printStackTrace();
                return false;
            }
        }

        private boolean downloadApk() {
            try {
                URL url = new URL(apkInfo.fileUrl);
                HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                connection.setConnectTimeout(30000);
                connection.setReadTimeout(30000);
                connection.connect();

                if (connection.getResponseCode() != HttpURLConnection.HTTP_OK) {
                    errorMessage = "HTTP error: " + connection.getResponseCode();
                    return false;
                }

                int fileLength = connection.getContentLength();
                totalBytes = fileLength;

                downloadedBytes = 0;

                String urlPath = apkInfo.fileUrl;
                currentFileName = urlPath.substring(urlPath.lastIndexOf('/') + 1);
                if (currentFileName.isEmpty()) {
                    currentFileName = "mobile-game-release.apk";
                }

                publishProgress(10, 0L, totalBytes, currentFileName);

                File oldFile = new File(APK_FILE_PATH);
                if (oldFile.exists()) {
                    boolean deleted = oldFile.delete();
                    Log.d("DownloadApk", "Old file deleted: " + deleted);
                }

                InputStream input = connection.getInputStream();
                OutputStream output = new FileOutputStream(APK_FILE_PATH);

                byte[] data = new byte[4096];
                long total = 0;
                int count;
                while ((count = input.read(data)) != -1) {
                    total += count;
                    downloadedBytes = total;

                    if (fileLength > 0) {
                        int progress = 10 + (int) (total * 80 / fileLength);
                        publishProgress(progress, downloadedBytes, totalBytes, currentFileName);
                    }
                    output.write(data, 0, count);
                }

                output.flush();
                output.close();
                input.close();
                connection.disconnect();

                Log.d("DownloadApk", "Download completed. File saved to: " + APK_FILE_PATH);
                Log.d("DownloadApk", "File size: " + new File(APK_FILE_PATH).length());

                return true;

            } catch (Exception e) {
                errorMessage = "Download exception: " + e.getMessage();
                e.printStackTrace();
                return false;
            }
        }

        private boolean saveVersionInfo() {
            try {
                File versionDir = new File(VERSION_DIR);
                if (!versionDir.exists()) {
                    versionDir.mkdirs();
                }

                File versionFile = new File(VERSION_FILE);
                FileWriter writer = new FileWriter(versionFile);
                writer.write("versionName=" + apkInfo.versionName + "\n");
                writer.write("versionCode=" + apkInfo.versionCode + "\n");
                writer.write("fileName=" + apkInfo.fileName + "\n");
                writer.write("fileUrl=" + apkInfo.fileUrl + "\n");
                writer.write("downloadPath=" + APK_FILE_PATH + "\n");
                writer.flush();
                writer.close();

                Log.d("SaveVersion", "Version info saved to: " + VERSION_FILE);
                return true;
            } catch (Exception e) {
                errorMessage = "Version file exception: " + e.getMessage();
                e.printStackTrace();
                return false;
            }
        }

        @Override
        protected void onProgressUpdate(Object... values) {
            if (values.length >= 4 && values[0] instanceof Integer && values[3] instanceof String) {
                int progress = (Integer) values[0];
                long downloaded = values[1] instanceof Long ? (Long) values[1] :
                        values[1] instanceof Integer ? ((Integer) values[1]).longValue() : 0;
                long total = values[2] instanceof Long ? (Long) values[2] :
                        values[2] instanceof Integer ? ((Integer) values[2]).longValue() : 0;
                String fileName = (String) values[3];

                updateProgress(progress, downloaded, total, fileName);
            }
        }

        @Override
        protected void onPostExecute(Boolean success) {
            boolean isReinstall = getIntent().getBooleanExtra("is_reinstall", false);

            if (success) {
                if (isUpdateAvailable) {
                    String totalFormatted = formatFileSize(totalBytes);
                    downloadProgressText.setText(String.format("%s из %s / 100%%", totalFormatted, totalFormatted));

                    runOnUiThread(() -> {
                        if (isReinstall) {
                            currentFileText.setText("УСТАНОВКА...");
                        } else {
                            currentFileText.setText("УСТАНОВКА...");
                        }
                    });

                    trackInstallCompleted(true, "", isReinstall);

                    new Handler().postDelayed(() -> {
                        installApk();
                        finish();
                    }, 1500);
                } else {
                    runOnUiThread(() -> {
                        if (isReinstall) {
                            currentFileText.setText("ОБНОВЛЕНИЙ НЕ НАЙДЕНО");
                        } else {
                            currentFileText.setText("УСТАНОВКА НЕ ТРЕБУЕТСЯ");
                        }
                        downloadProgressText.setText("ВЕРСИЯ АКТУАЛЬНА");
                    });

                    trackInstallCompleted(true, "No update needed", isReinstall);

                    new Handler().postDelayed(() -> {
                        startActivity(new Intent(LoadApkActivity.this, HomeActivity.class));
                        finish();
                    }, 2000);
                }
            } else {
                if (isReinstall) {
                    currentFileText.setText("ОШИБКА ОБНОВЛЕНИЯ");
                } else {
                    currentFileText.setText("ОШИБКА ЗАГРУЗКИ");
                }
                downloadProgressText.setText(errorMessage);
                downloadProgressBar.setVisibility(View.VISIBLE);
                downloadProgressText.setVisibility(View.VISIBLE);

                if (introVideoView != null && introVideoView.isPlaying()) {
                    introVideoView.stopPlayback();
                    introVideoView.setVisibility(View.GONE);
                }

                trackInstallCompleted(false, errorMessage, isReinstall);

                Log.e("ApkInstallTask", "Installation failed: " + errorMessage);
            }
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (introVideoView != null && introVideoView.isPlaying()) {
            introVideoView.pause();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (introVideoView != null && !introVideoView.isPlaying()) {
            introVideoView.start();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (introVideoView != null) {
            introVideoView.stopPlayback();
        }
    }
}